#ifndef FILEWATCHER_H
#define FILEWATCHER_H

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>
#endif // WIN32

#if __unix__
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/inotify.h>
#endif // __unix__

#include <functional>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <utility>
#include <vector>
#include <array>
#include <map>
#include <system_error>
#include <string>
namespace filewatch {
	enum class ChangeType {
		added,
		removed,
		modified,
		renamed_old,
		renamed_new
	};

	template<class T>
	class FileWatch
	{
	public:
		FileWatch(T path, std::function<void(const T& file, const ChangeType change_type)> callback) :
			_path(path),
			_callback(callback),
			_directory(get_directory()),
#ifdef _WIN32
			_close_event(CreateEvent(NULL, TRUE, FALSE, NULL))
#endif // WIN32
		{
			if (!_close_event) {
				throw std::system_error(GetLastError(), std::system_category());
			}
			_callback_thread = std::move(std::thread([this]() { callback_thread(); }));
			_watch_thread = std::move(std::thread([this]() { monitor_directory(); }));
		}
		~FileWatch() {
			_destory = true;
			SetEvent(_close_event);
			cv.notify_all();
			_watch_thread.join();
			_callback_thread.join();
			CloseHandle(_directory);
		}

	private:
		T _path;
		std::atomic<bool> _destory = { false };
		std::function<void(const T& file, const ChangeType change_type)> _callback;

		std::thread _watch_thread;

		std::condition_variable cv;
		std::mutex _callback_mutex;
		std::vector<std::pair<T, ChangeType>> _callback_information;
		std::thread _callback_thread;
#ifdef _WIN32
		HANDLE _directory = { nullptr };
		HANDLE _close_event = { nullptr };

		const DWORD _filters =
			FILE_NOTIFY_CHANGE_SECURITY |
			FILE_NOTIFY_CHANGE_CREATION |
			FILE_NOTIFY_CHANGE_LAST_ACCESS |
			FILE_NOTIFY_CHANGE_LAST_WRITE |
			FILE_NOTIFY_CHANGE_SIZE |
			FILE_NOTIFY_CHANGE_ATTRIBUTES |
			FILE_NOTIFY_CHANGE_DIR_NAME |
			FILE_NOTIFY_CHANGE_FILE_NAME;

		const std::map<DWORD, ChangeType> _change_type_mapping = {
			{ FILE_ACTION_ADDED, ChangeType::added },
			{ FILE_ACTION_REMOVED, ChangeType::removed },
			{ FILE_ACTION_MODIFIED, ChangeType::modified },
			{ FILE_ACTION_RENAMED_OLD_NAME, ChangeType::renamed_old },
			{ FILE_ACTION_RENAMED_NEW_NAME, ChangeType::renamed_new }
		};
#endif // WIN32

#ifdef _WIN32
		HANDLE get_directory() {
			HANDLE directory = ::CreateFile(
				_path.c_str(),           // pointer to the file name
				FILE_LIST_DIRECTORY,    // access (read/write) mode
				FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, // share mode
				NULL, // security descriptor
				OPEN_EXISTING,         // how to create
				FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, // file attributes
				NULL);                 // file with attributes to copy

			if (directory == INVALID_HANDLE_VALUE)
			{
				throw std::system_error(GetLastError(), std::system_category());
			}
			return directory;
		}
		void monitor_directory() {
			std::vector<BYTE> buffer(1024 * 256);
			DWORD bytes_returned = 0;
			OVERLAPPED overlapped_buffer{ 0 };

			overlapped_buffer.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
			if (!overlapped_buffer.hEvent) {
				std::cerr << "Error creating monitor event" << std::endl;
			}

			std::array<HANDLE, 2> handles{ overlapped_buffer.hEvent, _close_event };

			auto async_pending = false;
			do {
				std::vector<std::pair<T, ChangeType>> parsed_information;
				ReadDirectoryChangesW(
					_directory,
					buffer.data(), buffer.size(),
					TRUE,
					_filters,
					&bytes_returned,
					&overlapped_buffer, NULL);
				async_pending = true;
				switch (WaitForMultipleObjects(2, handles.data(), FALSE, INFINITE))
				{
				case WAIT_OBJECT_0:
				{
					if (!GetOverlappedResult(_directory, &overlapped_buffer, &bytes_returned, TRUE)) {

					}
					async_pending = false;

					if (bytes_returned == 0) {
						break;
					}

					FILE_NOTIFY_INFORMATION *file_information = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(&buffer[0]);
					do
					{
						std::basic_string<T::value_type> filename{ file_information->FileName, file_information->FileNameLength / 2 };
						parsed_information.emplace_back(T{ filename }, _change_type_mapping.at(file_information->Action));

						if (file_information->NextEntryOffset == 0) {
							break;
						}

						file_information = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(reinterpret_cast<BYTE*>(file_information) + file_information->NextEntryOffset);
					} while (true);
					break;
				}
				case WAIT_OBJECT_0 + 1:
					// quit
					break;
				case WAIT_FAILED:
					break;
				}
				//dispatch callbacks
				{
					std::lock_guard<std::mutex> lock(_callback_mutex);
					_callback_information.insert(_callback_information.end(), parsed_information.begin(), parsed_information.end());
				}
				cv.notify_all();
			} while (_destory == false);


			if (async_pending)
			{
				//clean up running async io
				CancelIo(_directory);
				GetOverlappedResult(_directory, &overlapped_buffer, &bytes_returned, TRUE);
			}
		}
#endif // WIN32

		void callback_thread()
		{
			while (_destory == false) {
				std::unique_lock<std::mutex> lock(_callback_mutex);
				if (_callback_information.empty() && _destory == false) {
					cv.wait(lock, [this] { return _callback_information.size() > 0 || _destory; });
				}
				const auto callback_information = std::exchange(_callback_information, {});
				lock.unlock();

				for (const auto& file : callback_information) {
					if (_callback) {
						try
						{
							_callback(file.first, file.second);
						}
						catch (const std::exception&)
						{
						}
					}
				}
			}
		}
	};
}
#endif
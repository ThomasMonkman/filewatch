//	MIT License
//
//	Copyright(c) 2017 Thomas Monkman
//	Copyright(c) 2025 Xichen Zhou
//	
//	Permission is hereby granted, free of charge, to any person obtaining a copy
//	of this software and associated documentation files(the "Software"), to deal
//	in the Software without restriction, including without limitation the rights
//	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//	copies of the Software, and to permit persons to whom the Software is
//	furnished to do so, subject to the following conditions :
//	
//	The above copyright notice and this permission notice shall be included in all
//	copies or substantial portions of the Software.
//	
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//	SOFTWARE.

#ifndef FILEWATCHER_H
#define FILEWATCHER_H

#include <cstdio>
#include <fstream>

#ifdef _WIN32
#define FILEWATCH_PLATFORM_WIN
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <Pathcch.h>
#include <shlwapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

#endif

#if defined(FILEWATCH_PLATFORM_LINUX)
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#if defined(FILEWATCH_PLATFORM_MAC)
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
#include <fcntl.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <unordered_map>
#include <unordered_set>
#endif

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <condition_variable>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <filesystem>
#include <functional>
#include <future>
#include <mutex>
#include <regex>
#include <string>
#include <system_error>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace filewatch {
	namespace fs = std::filesystem;
	enum class Event {
		added,
		removed,
		modified,
		renamed_old,
		renamed_new
	};
      
      template<typename Fn, typename... Args>
      struct Invokable {
            static Fn make() {
                  return (Fn*)0;
            }

            template<typename T>
            static T defaultValue() {
                  return *(T*)0;
            }

            static void call(int) {
                  make()(defaultValue<Args...>());
            }

            static int call(long value);

            static constexpr bool value = std::is_same<decltype(call(0)), int>::value;
      };

#define _FILEWATCH_TO_STRING(x) #x
#define FILEWATCH_TO_STRING(x) _FILEWATCH_TO_STRING(x)

      [[maybe_unused]] static const char* event_to_string(Event event) {
            switch (event) {
            case Event::added:
                  return FILEWATCH_TO_STRING(Event::added);
            case Event::removed:
                  return FILEWATCH_TO_STRING(Event::removed);
            case Event::modified:
                  return FILEWATCH_TO_STRING(Event::modified);
            case Event::renamed_old:
                  return FILEWATCH_TO_STRING(Event:renamed_old);
            case Event::renamed_new:
                  return FILEWATCH_TO_STRING(Event::renamed_new);
            }
            assert(false);
      }

      /**
       * \class FileWatchBase
       *
       * \brief Watches a folder or file, and will notify of changes via function callback.
       *
       * \author Thomas Monkman
       *
       */
      template <class SubClass>
      class FileWatchBase {
      public:
	      ~FileWatchBase()
	      {
		      destroy();
	      }

	      FileWatchBase &operator=(const FileWatchBase &other)
	      {
		      if (this == &other) {
			      return *this;
		      }

		      destroy();
		      _path = other._path;
		      _callback = other._callback;
		      init();
		      return *this;
	      }

	      // Const memeber varibles don't let me implent moves nicely, if moves are really wanted std::unique_ptr
	      // should be used and move that.
	      FileWatchBase(FileWatchBase &&) = delete;
	      FileWatchBase &operator=(FileWatchBase &&) & = delete;

      protected:
	      FileWatchBase(const fs::path &path, const std::regex &pattern,
		            std::function<void(const fs::path &file, const Event event_type)> callback)
		  : _path(fs::canonical(path)), _pattern(pattern), _callback(callback)
	      {
		      if (!fs::exists(path)) {
			      throw fs::filesystem_error("no such file exists", path, std::error_code());
		      }
		      // init();
	      }

      protected:
	      static constexpr char _regex_all[] = {'.', '*', '\0'};
	      static constexpr char _this_directory[] = {'.', '/', '\0'};

	      struct PathParts {
		      fs::path directory;
		      fs::path filename;
	      };
	      // the path to watch, either a single file or directory
	      fs::path _path;
	      // additional filters
	      std::regex _pattern;
	      // only used if watch a single file
	      fs::path _filename;

	      std::function<void(const fs::path &file, const Event event_type)> _callback;

	      std::thread _watch_thread;

	      std::condition_variable _cv;
	      std::mutex _callback_mutex;
	      std::vector<std::pair<fs::path, Event>> _callback_information;
	      std::thread _callback_thread;

	      std::promise<void> _running;
	      std::atomic<bool> _destory = {false};
	      bool _watching_single_file = {false};

#if FILEWATCH_PLATFORM_MAC

#endif // FILEWATCH_PLATFORM_MAC

		void init()
		{
			static_cast<SubClass *>(this)->get_directory(_path);

			_callback_thread = std::thread([this]() {
				try {
					callback_thread();
				} catch (...) {
					try {
						_running.set_exception(std::current_exception());
					}
					catch (...) {} // set_exception() may throw too
				}
			});

			_watch_thread = std::thread(
			    [this]()
			    {
				    try {
					    static_cast<SubClass *>(this)->monitor_directory();
				    }
				    catch (...) {
					    try {
						    _running.set_exception(std::current_exception());
					    }
					    catch (...) {
					    } // set_exception() may throw too
				    }
			    });

			std::future<void> future = _running.get_future();
			future.get(); //block until the monitor_directory is up and running
		}

		void destroy()
		{
			_destory = true;
			_running = std::promise<void>();

			static_cast<SubClass *>(this)->close();

			_cv.notify_all();
			_watch_thread.join();
			_callback_thread.join();

			static_cast<SubClass *>(this)->destroy();
		}

		const PathParts split_directory_and_file(const fs::path &path) const
		{
			fs::path parent_path = path.parent_path();
			// deal with empty parent path case such as "text.txt"
			if (parent_path.empty()) {
				parent_path = fs::current_path();
			}
			parent_path = fs::canonical(parent_path);

			return PathParts{
			    parent_path,
			    path.filename(),
			};
		}

		bool pass_filter(const fs::path &file_path)
		{
			return (_watching_single_file) ? file_path.filename() == _filename
			                               : std::regex_match(file_path.u8string(), _pattern);
		}

		void callback_thread()
		{
			while (_destory == false) {
				std::unique_lock<std::mutex> lock(_callback_mutex);
				if (_callback_information.empty() && _destory == false) {
					_cv.wait(lock, [this] { return _callback_information.size() > 0 || _destory; });
				}
				decltype(_callback_information) callback_information = {};
				std::swap(callback_information, _callback_information);
				lock.unlock();

				for (const auto &file : callback_information) {
					if (_callback) {
						try {
							_callback(file.first, file.second);
						}
						catch (const std::exception &) {
						}
					}
				}
			}
		}
      };
} // namespace filewatch

#if defined(FILEWATCH_PLATFORM_MAC)

namespace filewatch {
	class _FileWatch : public FileWatchBase<_FileWatch> {
		friend FileWatchBase<_FileWatch>;

	protected:
		_FileWatch(fs::path const &path, std::regex const &pattern,
		           std::function<void(const fs::path &file, const Event event_type)> callback)
		    : FileWatchBase<_FileWatch>(path, pattern, callback)
		{
			// it has to go here because base class cannot
			// initialize subclass
			this->init();
		}

	private:
		struct FileState {
			ino_t inode;
			uint32_t nlink;
			time_t last_modification;
		};
		struct DirectorySnapShot {
			using states_t = std::unordered_map<fs::path, FileState>;
			using paths_t = std::unordered_map<ino_t, fs::path>;

			states_t states;
			paths_t paths;

			typename states_t::const_iterator find(fs::path const &path) const
			{
				return states.find(path);
			}

			typename paths_t::const_iterator find(ino_t node) const
			{
				return paths.find(node);
			}

			void insert(fs::path const &path, FileState const &state)
			{
				if (paths.find(state.inode) != paths.end())
					paths.erase(paths.find(state.inode));
				states[path] = state;
				paths[state.inode] = path;
			}

			void erase(fs::path const &path)
			{
				if (states.find(path) != states.end()) {
					FileState &state = states.at(path);
					paths.erase(state.inode);
				}
				states.erase(path);
			}

		} _directory_snapshot;

		bool _previous_event_is_rename = false;
		dispatch_queue_t _queue = nullptr;
		struct timespec _last_modification_time = {};
		FSEventStreamRef _directory;
		// fd for single file

	private:
		FileState makeFileState(fs::path const &path)
		{
			struct stat st{};

			if (!path.is_absolute()) {
				::stat(fs::canonical(path).c_str(), &st);
			}
			else {
				::stat(path.c_str(), &st);
			}
			return FileState{st.st_ino, st.st_nlink, st.st_mtimespec.tv_sec};
		}

		void notify(const fs::path &path, const FSEventStreamEventFlags flags, ino_t inode)
		{
			std::vector<std::pair<fs::path, Event>> callbacks;
			bool regex_matched = std::regex_match(path.filename().u8string(), this->_pattern);
			fs::path parentPath = fs::is_directory(this->_path) ? this->_path : this->_path.parent_path();
			fs::path relPath = fs::relative(path, parentPath);
			if (relPath.empty()) {
				return;
			}

			if (flags & kFSEventStreamEventFlagItemRenamed) {
				const auto prevPath = _directory_snapshot.find(inode);
				if (prevPath != _directory_snapshot.paths.end()) {
					fs::path prevRelPath = fs::relative(prevPath->second, parentPath);
					callbacks.emplace_back(std::make_pair(prevRelPath, Event::renamed_old));
					callbacks.emplace_back(std::make_pair(relPath, Event::renamed_new));
				}
				_directory_snapshot.insert(path, makeFileState(path));
			}
			else if ((flags & kFSEventStreamEventFlagItemModified)) {
				if (_directory_snapshot.find(path) != _directory_snapshot.states.end()) {
					callbacks.emplace_back(std::make_pair(relPath, Event::modified));
				}
			}
			else if ((flags & kFSEventStreamEventFlagItemCreated) &&
			         regex_matched &&
			         !this->_watching_single_file)
			{
				_directory_snapshot.insert(path, makeFileState(path));
				callbacks.emplace_back(std::make_pair(relPath, Event::added));
			}
			else if (flags & kFSEventStreamEventFlagItemRemoved) {
				if (_directory_snapshot.find(path) != _directory_snapshot.states.end()) {
					_directory_snapshot.erase(path);
					callbacks.emplace_back(std::make_pair(relPath, Event::removed));
				}
			}
			{
				std::lock_guard<std::mutex> lock(this->_callback_mutex);
				this->_callback_information.insert(
				    std::end(this->_callback_information), callbacks.begin(), callbacks.end());
			}
			this->_cv.notify_all();
		}

		static CFStringRef CFStringRefFromPath(const fs::path &path)
		{
			auto u8string = path.u8string(); // native format
			return CFStringCreateWithBytes(kCFAllocatorDefault,
			                               (const UInt8 *)u8string.data(),
			                               u8string.size(),
			                               kCFStringEncodingUTF8,
			                               false);
		}

		static fs::path CFStringRefToPath(CFStringRef const &stringRef)
		{
			CFIndex length = CFStringGetLength(stringRef);
			std::vector<char> buffer(length + 1);
			CFStringGetCString(stringRef, buffer.data(), length + 1, kCFStringEncodingUTF8);
			return fs::path(buffer.data());
		}

		static void handleFsEvent(__attribute__((unused)) ConstFSEventStreamRef streamFef,
		                          void *clientCallBackInfo, size_t numEvents, CFArrayRef eventPaths,
		                          const FSEventStreamEventFlags *eventFlags,
		                          __attribute__((unused)) const FSEventStreamEventId *eventIds)
		{
			auto *self = (_FileWatch *)clientCallBackInfo;

			for (size_t i = 0; i < numEvents; i++) {
				FSEventStreamEventFlags flag = eventFlags[i];
				// external data gives us the inode
				auto pathInfoDict =
				    static_cast<CFDictionaryRef>(CFArrayGetValueAtIndex((CFArrayRef)eventPaths, i));
				auto pathRef = static_cast<CFStringRef>(
				    CFDictionaryGetValue(pathInfoDict, kFSEventStreamEventExtendedDataPathKey));
				auto cfInode = static_cast<CFNumberRef>(
				    CFDictionaryGetValue(pathInfoDict, kFSEventStreamEventExtendedFileIDKey));
				unsigned long inode = 0;

				if (cfInode) {
					CFNumberGetValue(cfInode, kCFNumberLongType, &inode);
				}
				if (flag & (kFSEventStreamEventFlagMustScanSubDirs |
				            kFSEventStreamEventFlagUserDropped |
				            kFSEventStreamEventFlagKernelDropped))
				{
					continue; // ignore invalid events.
				}
				self->notify(CFStringRefToPath(pathRef), flag, inode);
			}
		}

		FSEventStreamRef openStream(const fs::path &directory)
		{
			// Note that FSEventStreamCreate can only handle directories
			CFStringRef path = CFStringRefFromPath(directory);
			CFArrayRef paths = CFArrayCreate(kCFAllocatorDefault, (const void **)&path, 1, nullptr);
			FSEventStreamContext context{.info = (void *)this};
			FSEventStreamRef event = FSEventStreamCreate(kCFAllocatorDefault,
			                                             (FSEventStreamCallback)handleFsEvent,
			                                             &context,
			                                             paths,
			                                             kFSEventStreamEventIdSinceNow,
			                                             0,
			                                             (kFSEventStreamCreateFlagNoDefer |
			                                              kFSEventStreamCreateFlagFileEvents |
			                                              kFSEventStreamCreateFlagUseExtendedData |
			                                              kFSEventStreamCreateFlagUseCFTypes));

			CFRelease(path);
			CFRelease(paths);
			return event;
		}

		FSEventStreamRef openStreamForDirectory(const fs::path &directory)
		{
			FSEventStreamRef stream = openStream(directory);
			// note that we could come from openStreamForFile.
			if (fs::is_directory(directory) && !this->_watching_single_file) {
				for (const auto &dir_entry : fs::directory_iterator{directory}) {
					const auto &path = dir_entry.path();
					if (std::regex_match(path.u8string(), this->_pattern)) {
						_directory_snapshot.insert(path, makeFileState(path));
					}
				}
			}
			return stream;
		}

		FSEventStreamRef openStreamForFile(const fs::path &file)
		{
			auto split = this->split_directory_and_file(file);

			this->_watching_single_file = true;
			this->_filename = std::move(split.filename);
			_directory_snapshot.insert(file, makeFileState(file));
			return openStreamForDirectory(split.directory);
		}

		void get_directory(const fs::path &path)
		{
			if (fs::exists(path)) {
				if (fs::is_directory(path)) {
					_directory = openStreamForDirectory(path);
				}
				else if (fs::is_regular_file(path)) {
					_directory = openStreamForFile(path);
				}
				else {
					throw fs::filesystem_error(
					    "file is not directory or regular file", path, std::error_code());
				}
			}
			else {
				throw fs::filesystem_error("file does not exist", path, std::error_code());
			}
		}

		void monitor_directory()
		{
			_queue = dispatch_queue_create("DirectoryWatcher", DISPATCH_QUEUE_SERIAL);
			FSEventStreamSetDispatchQueue(_directory, _queue);
			FSEventStreamStart(_directory);
			this->_running.set_value();
		}

		void destroy()
		{
			FSEventStreamStop(_directory);
			FSEventStreamInvalidate(_directory);
			FSEventStreamRelease(_directory);
			_directory = nullptr;
			if (_queue) {
				dispatch_release(_queue);
			}
		}
	};

} // namespace filewatch

#elif defined(FILEWATCH_PLATFORM_LINUX)

namespace filewatch {
	class _FileWatch : public FileWatchBase<_FileWatch> {
		friend FileWatchBase<_FileWatch>;

	public:
		_FileWatch(fs::path const &path, std::regex const &pattern,
		           std::function<void(const fs::path &file, const Event event_type)> callback)
		    : FileWatchBase<_FileWatch>(path, pattern, callback)
		{
			// it has to go here because base class cannot
			// initialize subclass
			this->init();
		}

	private:
		struct FolderInfo {
			int folder;
			int watch;
		};

		FolderInfo _directory;

		const std::uint32_t _listen_filters = IN_MODIFY | IN_CREATE | IN_DELETE;

		constexpr static size_t event_size = (sizeof(struct inotify_event));
		constexpr static size_t buf_size = 1024 * event_size + 16;

	private:
		void get_directory(const fs::path &path)
		{
			const auto folder = inotify_init();
			if (folder < 0) 
			{
				throw std::system_error(errno, std::system_category());
			}

			_watching_single_file = fs::is_regular_file(path);
			_filename = path.filename();

			const auto watch = inotify_add_watch(folder, path.c_str(), IN_MODIFY | IN_CREATE | IN_DELETE);
			if (watch < 0) {
				throw std::system_error(errno, std::system_category());
			}
			_directory = { folder, watch };
		}
		void close()
		{
			inotify_rm_watch(_directory.folder, _directory.watch);
		}
		void destroy()
		{
			::close(_directory.folder);
		}

		void process_inotify(inotify_event const *event,
		                     std::vector<std::pair<fs::path, Event>> &parsed_information)
		{
			// when watching a single file, the event->len could be 0
			if (event->len || _watching_single_file) {
				const fs::path changed_file{event->name};
				if (pass_filter(changed_file)) {
					if (event->mask & IN_CREATE) {
						parsed_information.emplace_back(changed_file, Event::added);
					}
					else if (event->mask & IN_DELETE) {
						parsed_information.emplace_back(changed_file, Event::removed);
					}
					else if (event->mask & IN_MODIFY) {
						parsed_information.emplace_back(changed_file, Event::modified);
					}
				}
			}
		}

		void monitor_directory()
		{
			char buffer[buf_size];
			const struct inotify_event *event = nullptr;
			_running.set_value();
			while (_destory == false) {
				const auto length = read(_directory.folder, static_cast<void *>(buffer), buf_size);
				if (length > 0) {
					std::vector<std::pair<fs::path, Event>> parsed_information;
					for (char *ptr = buffer; ptr < buffer + length;
					     ptr += sizeof(struct inotify_event) + event->len)
					{
						event = reinterpret_cast<struct inotify_event *>(ptr); // NOLINT
						process_inotify(event, parsed_information);
					}
					// dispatch callbacks
					{
						std::lock_guard<std::mutex> lock(_callback_mutex);
						_callback_information.insert(_callback_information.end(), parsed_information.begin(), parsed_information.end());
					}
					_cv.notify_all();
				}
			}
		}
	};
} // namespace filewatch

#elif defined(FILEWATCH_PLATFORM_WIN)

namespace filewatch {
	class _FileWatch : public FileWatchBase<_FileWatch> {
		friend FileWatchBase<_FileWatch>;

	protected:
		_FileWatch(fs::path const &path, std::regex const &pattern,
		           std::function<void(const fs::path &file, const Event event_type)> callback)
		    : FileWatchBase<_FileWatch>(path, pattern, callback)
		{
			// it has to go here because base class cannot
			// initialize subclass
			this->init();
		}

	private:
		HANDLE _directory = {nullptr};
		HANDLE _close_event = {nullptr};

		const DWORD _listen_filters = FILE_NOTIFY_CHANGE_SECURITY | FILE_NOTIFY_CHANGE_CREATION |
		                              FILE_NOTIFY_CHANGE_LAST_ACCESS | FILE_NOTIFY_CHANGE_LAST_WRITE |
		                              FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_ATTRIBUTES |
		                              FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_FILE_NAME;

		const std::unordered_map<DWORD, Event> _event_type_mapping = {
		    {FILE_ACTION_ADDED, Event::added},
		    {FILE_ACTION_REMOVED, Event::removed},
		    {FILE_ACTION_MODIFIED, Event::modified},
		    {FILE_ACTION_RENAMED_OLD_NAME, Event::renamed_old},
		    {FILE_ACTION_RENAMED_NEW_NAME, Event::renamed_new}};
		static constexpr std::size_t _buffer_size = {1024 * 256};

	private:
		template <typename... Args> DWORD GetFileAttributesX(const char *lpFileName, Args... args)
		{
			return GetFileAttributesA(lpFileName, args...);
		}
		template <typename... Args> DWORD GetFileAttributesX(const wchar_t *lpFileName, Args... args)
		{
			return GetFileAttributesW(lpFileName, args...);
		}

		template <typename... Args> HANDLE CreateFileX(const char *lpFileName, Args... args)
		{
			return CreateFileA(lpFileName, args...);
		}
		template <typename... Args> HANDLE CreateFileX(const wchar_t *lpFileName, Args... args)
		{
			return CreateFileW(lpFileName, args...);
		}

		void get_directory(const fs::path &path)
		{
			_close_event = CreateEvent(NULL, TRUE, FALSE, NULL);
			if (!_close_event) {
				throw std::system_error(GetLastError(), std::system_category());
			}

			auto file_info = GetFileAttributesX(path.c_str());

			if (file_info == INVALID_FILE_ATTRIBUTES) {
				throw std::system_error(GetLastError(), std::system_category());
			}
			this->_watching_single_file = (file_info & FILE_ATTRIBUTE_DIRECTORY) == false;

			const fs::path watch_path = [this, &path]()
			{
				if (_watching_single_file) {
					const auto parsed_path = split_directory_and_file(path);
					_filename = parsed_path.filename;
					return parsed_path.directory;
				}
				else {
					return path;
				}
			}();

			HANDLE directory =
			    CreateFileX(watch_path.c_str(),  // pointer to the file name
			                FILE_LIST_DIRECTORY, // access (read/write) mode
			                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, // share mode
			                nullptr,                                                // security descriptor
			                OPEN_EXISTING,                                          // how to create
			                FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,      // file attributes
			                HANDLE(0)); // file with attributes to copy

			if (directory == INVALID_HANDLE_VALUE) {
				throw std::system_error(GetLastError(), std::system_category());
			}
			_directory = directory;
		}

		void monitor_directory()
		{
			std::vector<BYTE> buffer(_buffer_size);
			DWORD bytes_returned = 0;
			OVERLAPPED overlapped_buffer{ 0 };

			overlapped_buffer.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
			if (!overlapped_buffer.hEvent) {
				// std::cerr << "Error creating monitor event" << std::endl;
			}

			std::array<HANDLE, 2> handles{ overlapped_buffer.hEvent, _close_event };

			auto async_pending = false;
			_running.set_value();
			do {
				std::vector<std::pair<fs::path, Event>> parsed_information;
				ReadDirectoryChangesW(_directory,
				                      buffer.data(),
				                      static_cast<DWORD>(buffer.size()),
				                      TRUE,
				                      _listen_filters,
				                      &bytes_returned,
				                      &overlapped_buffer,
				                      NULL);

				async_pending = true;

				switch (WaitForMultipleObjects(2, handles.data(), FALSE, INFINITE)) {
				case WAIT_OBJECT_0:
				{
					if (!GetOverlappedResult(_directory, &overlapped_buffer, &bytes_returned, TRUE)) {
						throw std::system_error(GetLastError(), std::system_category());
					}
					async_pending = false;

					if (bytes_returned == 0) {
						break;
					}

					FILE_NOTIFY_INFORMATION *file_information = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(&buffer[0]);
					do
					{
						std::wstring changed_file_w{file_information->FileName,
						                            file_information->FileNameLength /
						                                sizeof(file_information->FileName[0])};
						fs::path changed_file(changed_file_w);

						if (pass_filter(changed_file)) {
							parsed_information.emplace_back(changed_file, _event_type_mapping.at(file_information->Action));
						}

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
				_cv.notify_all();
			} while (_destory == false);

			if (async_pending)
			{
				//clean up running async io
				CancelIo(_directory);
				GetOverlappedResult(_directory, &overlapped_buffer, &bytes_returned, TRUE);
			}
		}

		void close()
		{
			SetEvent(_close_event);
		}

		void destroy()
		{
			CloseHandle(_directory);
		}
	};
} // namespace filewatch

#endif

namespace filewatch {
	class FileWatch : public _FileWatch {
	public:
		FileWatch(fs::path const &path, std::regex const &pattern,
		          std::function<void(const fs::path &file, const Event event_type)> callback)
		    : _FileWatch(path, pattern, callback)
		{
		}
		FileWatch(fs::path const &path,
		          std::function<void(const fs::path &file, const Event event_type)> callback)
		    : FileWatch(path, std::regex(this->_regex_all), callback)
		{
		}

		FileWatch(const FileWatch &other) : FileWatch(other._path, other._callback)
		{
		}
	};
} // namespace filewatch

#endif

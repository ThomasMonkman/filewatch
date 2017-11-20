# Filewatch
**Master**  
[![Master](https://travis-ci.org/ThomasMonkman/filewatch.svg?branch=master)](https://travis-ci.org/ThomasMonkman/filewatch)

**Develop**  
[![Develop](https://travis-ci.org/ThomasMonkman/filewatch.svg?branch=develop)](https://travis-ci.org/ThomasMonkman/filewatch)
Single header only file watcher in c++

#### Example:
```cpp
filewatch::FileWatch<std::wstring> watch(L"C:/Users/User/Desktop/Watch/Test"s, [](const std::wstring& path, const filewatch::ChangeType change_type) {
		std::wcout << path << L" : ";
		switch (change_type)
		{
		case filewatch::ChangeType::added:
			std::cout << "The file was added to the directory." << '\n';
			break;
		case filewatch::ChangeType::removed:
			std::cout << "The file was removed from the directory." << '\n';
			break;
		case filewatch::ChangeType::modified:
			std::cout << "The file was modified. This can be a change in the time stamp or attributes." << '\n';
			break;
		case filewatch::ChangeType::renamed_old:
			std::cout << "The file was renamed and this is the old name." << '\n';
			break;
		case filewatch::ChangeType::renamed_new:
			std::cout << "The file was renamed and this is the new name." << '\n';
			break;
		};
	});
```

or using std::filesystem

```cpp
	filewatch::FileWatch<std::filesystem::path> watch(L"C:/Users/User/Desktop/Watch/Test"s, [](const std::filesystem::path& path, const filewatch::ChangeType change_type) {
		std::wcout << path << L" : ";
		switch (change_type)
		{
		case filewatch::ChangeType::added:
			std::cout << "The file was added to the directory." << '\n';
			break;
		case filewatch::ChangeType::removed:
			std::cout << "The file was removed from the directory." << '\n';
			break;
		case filewatch::ChangeType::modified:
			std::cout << "The file was modified. This can be a change in the time stamp or attributes." << '\n';
			break;
		case filewatch::ChangeType::renamed_old:
			std::cout << "The file was renamed and this is the old name." << '\n';
			break;
		case filewatch::ChangeType::renamed_new:
			std::cout << "The file was renamed and this is the new name." << '\n';
			break;
		};
	});
```

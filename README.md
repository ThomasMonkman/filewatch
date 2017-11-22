# Filewatch

| Branch        | Status        |
| ------------- |:-------------:|
| **Master**    | [![Master](https://travis-ci.org/ThomasMonkman/filewatch.svg?branch=master)](https://travis-ci.org/ThomasMonkman/filewatch) |
| **Develop**   | [![Develop](https://travis-ci.org/ThomasMonkman/filewatch.svg?branch=develop)](https://travis-ci.org/ThomasMonkman/filewatch) |

Single header only folder watcher in C++ for windows and linux

#### Examples:
- [Simple](#1)
- [Change Type](#2)
- [Using std::filesystem](#3)
- [Works with relative paths](#4)

###### Simple: <a id="1"></a>
```cpp
filewatch::FileWatch<std::wstring> watch(
	L"C:/Users/User/Desktop/Watch/Test"s, 
	[](const std::wstring& path, const filewatch::ChangeType change_type) {
		std::wcout << path << L"\n";
	}
);
```

###### Change Type: <a id="2"></a>
```cpp
filewatch::FileWatch<std::wstring> watch(
	L"C:/Users/User/Desktop/Watch/Test"s, 
	[](const std::wstring& path, const filewatch::ChangeType change_type) {
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
	}
);
```

###### using std::filesystem: <a id="3"></a>
```cpp
filewatch::FileWatch<std::filesystem::path> watch(
	L"C:/Users/User/Desktop/Watch/Test"s, 
	[](const std::filesystem::path& path, const filewatch::ChangeType change_type) {
		std::wcout << std::filesystem::absolute(path) << L"\n";		
	}
);
```

###### works with relative paths: <a id="4"></a>
```cpp
filewatch::FileWatch<std::filesystem::path> watch(
	L"./"s, 
	[](const std::filesystem::path& path, const filewatch::ChangeType change_type) {
		std::wcout << std::filesystem::absolute(path) << L"\n";		
	}
);
```

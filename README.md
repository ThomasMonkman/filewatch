# Filewatch <a href="#"><img src="https://img.shields.io/badge/C++-11-blue.svg?style=flat-square"></a>

| Branch        | Status        |
| ------------- |:-------------:|
| **Master**    | ![Master](https://github.com/ThomasMonkman/filewatch/workflows/CMake/badge.svg?branch=master) |

Single header folder/file watcher in C++11 for windows and linux, with optional regex filtering
#### Install:
Drop [FileWatch.hpp](https://github.com/ThomasMonkman/filewatch/blob/master/FileWatch.hpp) in to your include path, and you should be good to go.

#### Compiler Support:

Works on:

- Clang 4 and higher    
- GCC 4.8 and higher    
- Visual Studio 2015 and higher should be supported, however only 2019 is on the ci and tested

#### Examples:
- [Simple](#1)
- [Change Type](#2)
- [Regex](#3)
- [Using std::filesystem](#4)
- [Works with relative paths](#5)
- [Single file watch](#6)

On linux or none unicode windows change std::wstring for std::string or std::filesystem (boost should work as well).

###### Simple: <a id="1"></a>
```cpp
filewatch::FileWatch<std::wstring> watch(
	L"C:/Users/User/Desktop/Watch/Test"s, 
	[](const std::wstring& path, const filewatch::Event change_type) {
		std::wcout << path << L"\n";
	}
);
```

###### Change Type: <a id="2"></a>
```cpp
filewatch::FileWatch<std::wstring> watch(
	L"C:/Users/User/Desktop/Watch/Test"s, 
	[](const std::wstring& path, const filewatch::Event change_type) {
		std::wcout << path << L" : ";
		switch (change_type)
		{
		case filewatch::Event::added:
			std::cout << "The file was added to the directory." << '\n';
			break;
		case filewatch::Event::removed:
			std::cout << "The file was removed from the directory." << '\n';
			break;
		case filewatch::Event::modified:
			std::cout << "The file was modified. This can be a change in the time stamp or attributes." << '\n';
			break;
		case filewatch::Event::renamed_old:
			std::cout << "The file was renamed and this is the old name." << '\n';
			break;
		case filewatch::ChangeType::renamed_new:
			std::cout << "The file was renamed and this is the new name." << '\n';
			break;
		};
	}
);
```

###### Regex: <a id="3"></a>

Using the standard regex libary you can filter the file paths that will trigger. When using wstring you will have to use `std::wregex`
```cpp
filewatch::FileWatch<std::wstring> watch(
	L"C:/Users/User/Desktop/Watch/Test"s,
	std::wregex(L"test.*"),
	[](const std::wstring& path, const filewatch::Event change_type) {
		std::wcout << path << L"\n";
	}
);
```

###### Using std::filesystem: <a id="4"></a>
```cpp
filewatch::FileWatch<std::filesystem::path> watch(
	L"C:/Users/User/Desktop/Watch/Test"s, 
	[](const std::filesystem::path& path, const filewatch::Event change_type) {
		std::wcout << std::filesystem::absolute(path) << L"\n";		
	}
);
```

###### Works with relative paths: <a id="5"></a>
```cpp
filewatch::FileWatch<std::filesystem::path> watch(
	L"./"s, 
	[](const std::filesystem::path& path, const filewatch::Event change_type) {
		std::wcout << std::filesystem::absolute(path) << L"\n";		
	}
);
```

###### Single file watch: <a id="6"></a>
```cpp
filewatch::FileWatch<std::wstring> watch(
	L"./test.txt"s, 
	[](const std::wstring& path, const filewatch::Event change_type) {
		std::wcout << path << L"\n";		
	}
);
```


Compile parameters

Framework Qt 5.15.2

Need install:

Microsoft Visual Studio 2019 (C++)

LLVM 12.0.0 from https://github.com/llvm/llvm-project/releases/tag/llvmorg-12.0.0

Add Qt Kits in Qt Creator preferences:

Name:  - anything

Compiler:
	C:   LLVM clang-cl
	C++: LLVM clang-cl

Debugger: System LLDB at C:\Program Files\LLVM\bin\lldb.exe

Qt version: Qt 5.15.2 MSVC2019 64bit

CMake Tools: CMake 3.27.7 (Qt)



Note:
If install "Microsoft Visual C++ 2015-2022 Redistributable (x64)" and "(x86)"
in folder "bin" can be done delete - msvcr120.dll, msvcp140_1.dll, vcruntime140_1.dll

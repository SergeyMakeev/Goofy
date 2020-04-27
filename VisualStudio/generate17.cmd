set builddir="vs2017"
del %builddir% /S /Q
mkdir %builddir%
cd %builddir%
cmake -G "Visual Studio 15 2017 Win64" ../../
cd ..
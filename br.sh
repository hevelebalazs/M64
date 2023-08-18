#Build and Run
gcc M64.c -o M64.exe
if [ $? != 0 ] ; then
	exit 1
fi

./M64.exe Example/$1.m64 Example/$1.m64.c

if [ $? != 0 ] ; then
	exit 1
fi

echo Compiling c code...
gcc Example/$1.m64.c -o Example/$1.exe

if [ $? != 0 ] ; then
	exit 1
fi

echo Running exe...
echo --------------
./Example/$1.exe


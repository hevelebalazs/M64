echo Building compiler...

gcc M64.c -o M64.exe
if [ $? != 0 ] ; then
	exit 1
fi

echo Compiling to x64...

./M64.exe Example/$1.m64 Example/$1.asm

if [ $? != 0 ] ; then
	exit 1
fi

echo Assembling...

nasm -f win64 Example/$1.asm -o Example/$1.obj

if [ $? != 0 ] ; then
	exit 1
fi

echo Linking...

gcc Example/$1.obj -o Example/$1.exe

if [ $? != 0 ] ; then
	exit 1
fi

echo Running exe...
echo --------------
./Example/$1.exe
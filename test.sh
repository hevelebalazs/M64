echo Building M64 compiler...

gcc M64.c -o M64.exe
if [ $? != 0 ] ; then
	exit 1
fi

echo Converting M64 to C...

./M64.exe Test/Code.m64 Test/Code.h
if [ $? != 0 ] ; then
	exit 1
fi

echo Compiling C...

gcc Test/Test.c -o Test.exe -lGdi32
if [ $? != 0 ] ; then
	exit 1
fi

echo Running exe...

./Test.exe
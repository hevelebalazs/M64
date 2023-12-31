gcc Test/Test.c -o Test.exe -lGdi32
if [ $? != 0 ] ; then
	exit 1
fi

./Test.exe
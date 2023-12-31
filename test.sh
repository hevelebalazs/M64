gcc Test/Test.c -o Test.exe
if [ $? != 0 ] ; then
	exit 1
fi

./Test.exe
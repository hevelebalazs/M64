#Build and Run
gcc M64.c -o M64.exe
if [ $? == 0 ] ; then
	./M64.exe Example/ToC.m64 Example/ToC.m64.c
fi
if [ $? == 0 ] ; then
	echo Compiling c code...
	gcc Example/ToC.m64.c -o Example/ToC.exe
fi
if [ $? == 0 ] ; then
	echo Running exe...
	echo --------------
	./Example/ToC.exe
fi

#Build and Run
gcc M64.c -o M64.exe
if [ $? == 0 ] ; then
		./M64.exe Example/ToC.m64 Example/ToC.m64.c
fi

Build steps-
edksetup.bat rebuild
build -p UefiPayloadPkg/Library/UplData/UnitTest/CborTest.dsc -a X64 -b NOOPT -t VS2019 -D UNIT_TESTING_DEBUG=1



To verify the unit test code-
Run CborUnitTest.exe generated in Build\CborHostTest\NOOPT_VS2019\X64
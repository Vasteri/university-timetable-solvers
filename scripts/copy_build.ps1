$my_path = $PSScriptRoot

mkdir $my_path\..\build\
rm $my_path\..\build\Desktop_Qt_6_9_3_MinGW_64_bit-Release\
rm $my_path\..\build\android-build-CrossUI-debug.apk
cp -R $my_path\..\src\CrossUI\build\Desktop_Qt_6_9_3_MinGW_64_bit-Release\ $my_path\..\build\
C:\Qt\6.9.3\mingw_64\bin\windeployqt.exe $my_path\..\build\Desktop_Qt_6_9_3_MinGW_64_bit-Release\CrossUI.exe
cp "$my_path\..\src\CrossUI\build\Android_Qt_6_9_3_Clang_arm64_v8a-Debug\android-build-CrossUI\build\outputs\apk\debug\android-build-CrossUI-debug.apk" $my_path\..\build\
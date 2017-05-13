@echo off
rmdir /s /q libyojimbo
mkdir libyojimbo
mkdir libyojimbo\tests
copy ..\*.h libyojimbo
copy ..\*.cpp libyojimbo
copy ..\tests\*.* libyojimbo\tests
copy ..\premake5.lua libyojimbo
robocopy /MIR /DCOPY:T ..\tlsf libyojimbo\tlsf
REM because robocopy sometimes sets non-zero error codes on successful operation. what the actual fuck windows
cmd /c "exit /b 0"

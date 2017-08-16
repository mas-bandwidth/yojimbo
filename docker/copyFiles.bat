@echo off
rmdir /s /q yojimbo
mkdir yojimbo
mkdir yojimbo\tests
copy ..\*.h yojimbo
copy ..\*.cpp yojimbo
copy ..\premake5.lua yojimbo
robocopy /MIR /DCOPY:T ..\tlsf yojimbo\tlsf
REM because robocopy sometimes sets non-zero error codes on successful operation. what the actual fuck windows
cmd /c "exit /b 0"

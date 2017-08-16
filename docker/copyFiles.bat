@echo off
rmdir /s /q yojimbo
mkdir yojimbo
mkdir yojimbo\tests
copy ..\*.h yojimbo
copy ..\*.cpp yojimbo
copy ..\premake5.lua yojimbo
robocopy /MIR /DCOPY:T ..\tlsf yojimbo\tlsf
robocopy /MIR /DCOPY:T ..\netcode.io yojimbo\netcode.io
robocopy /MIR /DCOPY:T ..\reliable.io yojimbo\reliable.io
REM because robocopy sometimes sets non-zero error codes on successful operation. what the actual fuck windows
cmd /c "exit /b 0"

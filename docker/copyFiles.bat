@echo off
rmdir /s /q libyojimbo
mkdir libyojimbo
copy ..\*.h libyojimbo
copy ..\*.cpp libyojimbo
copy ..\premake5.lua libyojimbo
robocopy /MIR /DCOPY:T ..\rapidjson libyojimbo\rapidjson

REM if not exist libyojimbo\rapidjson mkdir libyojimbo\rapidjson
REM copy ..\rapidjson\*.h libyojimbo\rapidjson\*.h

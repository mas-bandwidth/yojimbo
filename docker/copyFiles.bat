@echo off
rmdir /s /q libyojimbo
mkdir libyojimbo
mkdir libyojimbo\tests
copy ..\*.h libyojimbo
copy ..\*.cpp libyojimbo
copy ..\tests\*.* libyojimbo\tests
copy ..\premake5.lua libyojimbo
robocopy /MIR /DCOPY:T ..\rapidjson libyojimbo\rapidjson
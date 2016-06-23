@echo off
rmdir /s /q libyojimbo
mkdir libyojimbo
copy ..\*.h libyojimbo
copy ..\*.cpp libyojimbo
copy ..\premake5.lua libyojimbo

This directory contains a port of yojimbo to the vcpkg package manager. 
vcpkg Git Repository: https://github.com/microsoft/vcpkg
vcpkg Documentation:  https://vcpkg.readthedocs.io

Usage:
    
  1. git clone https://github.com/microsoft/vcpkg.git
  2. cd vcpkg
  3. Bootstrap:
     - bootstrap-vcpkg.bat (windows) 
     - bootstrap-vcpkg.sh (Linux)
  4. Install
     - vcpkg install yojimbo:x64-windows --overlay-ports=this/directory (64 bit windows)
     - vcpkg install yojimbo:x64-linux --overlay-ports=this/directory(64 bit linux)

Tested on Windows 10 and Windows WSL2 Linux

ToDo:
----------
- Add features
- vcpkgify netcode and reliable modules and use standard vcpkg semantics.
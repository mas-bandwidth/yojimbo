The directory /vcpkg contains files to build and install yojimbo and its dependencies with the vcpkg package manager.
Documentation: https://vcpkg.io

Files
---------------

``yojimbo/vcpkg/ports/yojimbo/..`` This directory contains the files you need to build Yojimbo with vcpkg.

A. Building Yojimbo: 
---------------------
Install and bootstrap vcpkg, change to its root directory and open a shell there.

On windows: 

``$: vcpkg.exe install yojimbo:x64-windows-static --overlay-ports C:\develop\yojimbo\vcpkg\ports``

where "C:\Develop\yojimbo\vcpkg\ports" must be the correct path to the overlay directory in the yojimbo git tree "/yojimbo/vcpkg/ports"

On linux: 

``$: ./vcpkg install yojimbo:x64-linux --overlay-ports /home/joeuser/develop/yojimbo/vcpkg/ports``

just the equivalent (tested on WSL only).


B. Integrate the port into your vcpkg installation (advanced)
----------------------------------------------------------------------------
1. Fork vcpkg and check out your fork.

2. Add the yojimbo port from this repo to the ports of your checked out port.

3. In your vcpkg fork with the yojimbo port added run: vcpkg x-add-version --all


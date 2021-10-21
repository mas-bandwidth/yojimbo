The directory /vcpkg contains files to build and install yojimbo and its dependencies with the vcpkg package manager.
Documentation: https://vcpkg.io

Files
---------------

1. ``yojimbo/vcpkg/ports/yojimbo/..`` This directory contains the core files you need to build Yojimbo with vcpkg.

2. ``yojimbo/vcpkg/versions/y-/yojimbo.json`` and ``vcpkg/versions/baseline.json``.
These two files are only needed if you want to integrate yojimbo completely into vcpkg. 
Do NOT just copy over ``baseline.json`` - patch the existing one.


Building Yojimbo: 
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
Please read and understand the vcpkg documentation for this.

Notes:

1. Make sure to adjust the "baseline" field for the "default-registry" in `vcpkg-configuration.cmake` to the commit hash of your vcpkg installation

`{

    "default-registry": {
        "baseline": "bd1ef2df46303989eeb048eb7aa9b816aa46365e", <=== This one
        "kind": "builtin"
    },

    "registries": [
        {
            "baseline": "default",
            "kind": "filesystem",
            "path": "../registry",
            "packages": [
                "mbedtls",
                "yojimbo"
            ]
        }
    ]
}`

2. `src/main.cpp` is just `#include`ing the yojimbo header, but not really using it.
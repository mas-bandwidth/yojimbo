# TODO

I wake up today and connect.cpp is failing on MacOSX. I'm pretty 100% sure it worked last night.

It still works today on Linux.

What is going on? 

The error is in mbedtls: 

    MBEDTLS_ERR_CTR_DRBG_ENTROPY_SOURCE_FAILED

But why is it suddenly failing on MacOSX and no other platforms? What is different?

I tried rebooting my machine. Tried reinstalling mbedtls... same error. But still, it works on Linux.

What is going on?!
Research
========

# Travis

Faster way to install libsodium on trusty

https://gist.github.com/jonathanpmartins/2510f38abee1e65c6d92

Is there a prebuilt version of mbedtls I can use as well?

Caching in travis may also help speed up builds: https://docs.travis-ci.com/user/caching/

# Allocators for yojimbo

https://github.com/mattconte/tlsf

create_mspace_with_base with dlmalloc?

# Multithreaded design for yojimbo. 

Start profiling it. Is this necessary? What would need to change to make multithreading easier?

Would yojimbo just multithread internally? What would a multithreaded interface to yojimbo look like in this case to the user?

It would be nice to be able to run the expensive part, which is the network interface read and write packets off main thread.

What is required to be able to do this?

# Linux Sockets

Linux zero copy stuff and kernel bypass looks interesting. Worth investigating for server-side?

http://www.pfq.io

https://www.youtube.com/watch?v=tM4YskS94b0

http://www.slideshare.net/garyachy/dpdk-44585840

http://yusufonlinux.blogspot.com/2010/11/data-link-access-and-zero-copy.html

https://blog.cloudflare.com/kernel-bypass/

https://blog.cloudflare.com/how-to-achieve-low-latency/

http://www.openonload.org

https://www.youtube.com/watch?v=1Y8hoznuuuM

http://www.openonload.org/openonload-google-talk.pdf

http://www.net.in.tum.de/fileadmin/bibtex/publications/papers/Network-Latency-Netgames-2014.pdf

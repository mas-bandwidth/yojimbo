FROM phusion/baseimage:0.11

CMD ["/sbin/my_init"]

WORKDIR /app

RUN apt-get -y update && apt-get install -y wget make g++ dh-autoreconf pkg-config cmake

RUN wget https://github.com/jedisct1/libsodium/releases/download/1.0.16/libsodium-1.0.16.tar.gz && \
    tar -zxvf libsodium-*.tar.gz && \
    cd libsodium-* && \
    ./configure && \
    make -j32 && make check && \
    make install && \
    cd .. && \
    rm -rf libsodium* && \
    ldconfig

RUN wget https://github.com/premake/premake-core/releases/download/v5.0.0-alpha13/premake-5.0.0-alpha13-linux.tar.gz && \ 
    tar -zxvf premake-*.tar.gz && \
    rm premake-*.tar.gz && \
    mv premake5 /usr/local/bin

RUN wget https://github.com/ARMmbed/mbedtls/archive/mbedtls-2.13.0.tar.gz && \
    tar -zxvf mbedtls-*.tar.gz && \
    cd mbedtls-mbedtls-* && \
    cmake . && \
    make -j32 && make install && \
    ldconfig

ADD yojimbo /app/yojimbo

RUN cd yojimbo && find . -exec touch {} \; && premake5 gmake && make -j32 test server config=release_x64 && ./bin/test && cp /app/yojimbo/bin/* /app && rm -rf yojimbo

ENTRYPOINT ./test && ./server

RUN apt-get clean && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

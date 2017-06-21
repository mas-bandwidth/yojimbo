FROM phusion/baseimage:0.9.18

CMD ["/sbin/my_init"]

WORKDIR /app

RUN apt-get -y update && apt-get install -y wget make g++ dh-autoreconf pkg-config cmake

RUN wget https://github.com/jedisct1/libsodium/releases/download/1.0.10/libsodium-1.0.10.tar.gz && \
    tar -zxvf libsodium-1.0.10.tar.gz && \
    cd libsodium-1.0.10 && \
    ./configure && \
    make && make check && \
    make install && \
    cd .. && \
    rm -rf libsodium* && \
    ldconfig

RUN wget https://github.com/premake/premake-core/releases/download/v5.0.0-alpha10/premake-5.0.0-alpha10-linux.tar.gz && \ 
    tar -zxvf premake-*.tar.gz && \
    rm premake-*.tar.gz && \
    mv premake5 /usr/local/bin

RUN wget https://github.com/ARMmbed/mbedtls/archive/mbedtls-2.1.5.tar.gz && \
    tar -zxvf mbedtls-2.1.5.tar.gz && \
    cd mbedtls-mbedtls-2.1.5 && \
    cmake . && \
    make && make install && \
    ldconfig

ADD yojimbo /app/yojimbo

RUN cd yojimbo && find . -exec touch {} \; && premake5 gmake && make -j32 test server config=release_x64 && ./bin/test && cp /app/yojimbo/bin/* /app && rm -rf yojimbo

ENTRYPOINT ./test && ./server

RUN apt-get clean && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

FROM phusion/baseimage:0.9.18

CMD ["/sbin/my_init"]

WORKDIR /home

RUN apt-get -y update && apt-get install -y wget make g++ dh-autoreconf pkg-config

RUN wget https://github.com/jedisct1/libsodium/releases/download/1.0.10/libsodium-1.0.10.tar.gz && \
    tar -zxvf libsodium-1.0.10.tar.gz && \
    cd libsodium-1.0.10 && \
    ./configure && \
    make && make check && \
    make install && \
    cd .. && \
    rm -rf libsodium* && \
    ldconfig

RUN wget https://github.com/vstakhov/libucl/archive/0.8.0.tar.gz && \
    tar -zxvf 0.8.0.tar.gz && \
    cd libucl-0.8.0 && \
    ./autogen.sh && \
    ./configure && \
    make && make install && \
    cd .. && \
    rm -rf libucl* && \
    rm 0.8.0.tar.gz && \
    ldconfig

RUN wget https://github.com/premake/premake-core/releases/download/v5.0.0-alpha9/premake-5.0.0-alpha9-linux.tar.gz && \ 
    tar -zxvf premake-5.0.0-alpha9-linux.tar.gz && \
    rm premake-5.0.0-alpha9-linux.tar.gz && \
    mv premake5 /usr/local/bin

ADD libyojimbo /home/libyojimbo

RUN cd libyojimbo && premake5 gmake && make all config=release_x64 && ./bin/test && cp /home/libyojimbo/bin/server /home && rm -rf libyojimbo

CMD ./server

RUN apt-get clean && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

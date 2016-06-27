FROM golang

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

ADD matcher.go matcher.go

RUN go get github.com/gorilla/mux

RUN go build -o /go/bin/matcher matcher.go

ENTRYPOINT /go/bin/matcher

EXPOSE 8080
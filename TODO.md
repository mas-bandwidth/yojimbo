# TODO

How to setup the golang web server to serve over https?

Start here, curl should still work, then once curl works over https, get the mbedtls working over ssl

Seems to be pretty easy to server HTTP over TLS in go: https://gist.github.com/denji/12b3a568f092ab951456

OK so curl is able to connect now, but I think I need to get a real certificate, because I need to run curl in insecure mode: 

    curl https://localhost:8080/match/12314/1 --insecure

Where can I get a free real certificate?

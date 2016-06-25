# TODO

Now implement the code to parse the connect token from JSON.

Add test for read/write connect token to JSON. Make sure it reads back identically.

Actually convert the client/server token generation to use the JSON serialization of the token,
instead of the C++ specific binary serialization (which is not compatible with web backends...)

Now switch to golang and write a simple webservice to provide a connect token on request, initially just to one server running on 127.0.0.1:50000

Now switch back to C++ and integrate a HTTPS library to talk to this web service and request a match.

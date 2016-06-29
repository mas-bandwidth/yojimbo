# TODO

Time to switch over to another JSON lib. No point writing more code in libucl when I'm going to replace it.

Remedy guys suggested rapidjson

https://github.com/miloyip/rapidjson

It's pretty good. Very quickly implemented the match reponse parse with it.

Also it is a header only library, so it can just be included in the source tree for windows with no hassle.

Now to remove the rest of the code using libucl and get rid of it.

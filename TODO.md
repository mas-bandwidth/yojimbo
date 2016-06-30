# DONE

...

# TODO

First start by adding a magic value in the base packet (debug build only) which is set only if the object is created through the packet factory. That way if somehow a packet is getting passed into free which has not been created via the packet factory, I will pick it up. Also, bad pointers and so on.

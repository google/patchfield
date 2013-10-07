tinyosc
=======

A minimal but complete implementation of Open Sound Control 1.0 (http://opensoundcontrol.org/spec-1_0), providing serialization and deserialization of OSC messages but no networking support.  This implementation is real-time safe; in particular, it does not perform any memory allocation. The functions for packing and unpacking OSC packets are supposed to be reminiscent of the venerable printf/scanf functions in plain C.

For documentation, see the header file ``src/tinyosc.h``. The tests in ``src/tinyosc_test.c`` also show some sample invocations of the tinyosc API.

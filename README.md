Patchbay
========

Patchbay is an audio infrastructure for Android that provides a simple,
callback-driven API for implementing audio modules (such as synthesizers and
effects), a graph-based API for connecting audio modules, plus support for
inter-app audio routing. It is inspired by [JACK](http://jackaudio.org "JACK"),
the JACK audio connection kit.

Patchbay consists of a remote or local service that acts as a virtual patchbay
that audio apps can connect to, plus a number of sample apps that illustrate
how to implement audio modules and how to manipulate the signal processing
graph. The implementation resides entirely in userland and works on many stock
consumer devices, such as Nexus 7 and 10.


Cloning and building Patchbay
-----------------------------

Make sure that ``ndk-build`` is on your search path, then clone the project and
build the native components like this:

```
git clone --recursive ...
cd patchbay
make
```

Now you can import all projects in the patchbay directory into your development
environment of choice.


Project layout
--------------

* ``Patchbay`` Library project, the core of Patchbay
* ``PatchbayService``
* ``PatchbayControl``
* ``PatchbayJavaSample``
* ``PatchbayLowpassSample``
* ``PatchbayPcmSample``
* ``PatchbayPd``
* ``PatchbayPdSample``

Major components
----------------


Client API
----------


Module API
----------


Latency and performance
-----------------------


Device compatibility
--------------------


Patchbay and Google
-------------------

Patchbay started as a 20% project and Google owns the copyright.  Still, this
project is not an official Google product.

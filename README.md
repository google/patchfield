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


Major components
----------------


Project layout
--------------

* ``Patchbay`` Library project, the core of Patchbay; contains the Patchbay service class and all APIs.
* ``PatchbayControl`` Control app and remote service launch config; you will need to install this app if you want to run Patchbay as a remote service.
* ``PatchbayJavaSample`` Sample app illustrating the use of the JavaModule class.
* ``PatchbayLowpassSample`` Sample app illustrating how to implement an audio module for Patchbay, complete with build files and lock-free concurrency.
* ``PatchbayPcmSample`` Sample app playing a wav file through Patchbay. Very crude but useful for tests and demos.
* ``PatchbayPd`` Library project providing an audio module implementation that uses Pure Data (Pd) for synthesis.
* ``PatchbayPdSample`` Sample app using PatchbayPd.


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

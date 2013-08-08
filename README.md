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

* ``Patchbay`` Library project, the core of Patchbay; contains the Patchbay service class and all APIs.
* ``PatchbayControl`` Sample control app and remote service configuration; you will need to install this app if you want to run Patchbay as a remote service.
* ``PatchbayJavaSample`` Sample app illustrating the use of the JavaModule class.
* ``PatchbayLowpassSample`` Sample app illustrating how to implement an audio module for Patchbay, complete with build files and lock-free concurrency.
* ``PatchbayPcmSample`` Sample app playing a wav file through Patchbay. Crude but useful for prototyping and demos.
* ``PatchbayPd`` Library project providing an audio module implementation that uses libpd for synthesis. This project is a not a toy; libpd is a heavy-duty signal processing library, and project fully integrates it into Patchbay.
* ``PatchbayPdSample`` Sample app using PatchbayPd.


Major components
----------------

Patchbay consists of a number of components. At its core is a Java class,
``Patchbay.java``, that manages the audio graph and handles audio input and
output with OpenSL ES. Patchbay.java is meant to be accessed through
``PatchbayService.java``, which can run as a remote or local service, i.e., you
can use Patchbay to route audio signals between different apps, or you can
build apps that just use Patchbay internally.

There are two APIs for interacting with the Patchbay service, a Java API for querying and manipulating the state of the signal processing graph, and a hybrid C/Java API for implementing new audio modules such as synthesizers and effects.


Client API
----------


Module API
----------


Latency and performance
-----------------------


Device compatibility
--------------------

Patchbay pushes the limits of the current Android audio stack, and so it works better on some devices than on others. It is also a very young project that has not yet been tested on a wide range of devices. Here's an incomplete list of devices where Patchbay is known to work (or not).

Works: Nexus 7 (both new and old), Nexus 10, Nexus S

Glitches: Galaxy Nexus


Patchbay and Google
-------------------

Patchbay started as a 20% project and Google owns the copyright.  Still, this
project is not an official Google product.

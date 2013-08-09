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

* ``Patchbay`` Library project, the core of Patchbay; contains the
  ``PatchbayService`` class and all APIs, as well as utilities.
* ``PatchbayControl`` Sample control app and remote service configuration; you
  will need to install this app if you want to run Patchbay as a remote
service.
* ``PatchbayJavaSample`` Sample app illustrating the use of the ``JavaModule``
  class.
* ``PatchbayLowpassSample`` Sample app illustrating how to implement an audio
  module for Patchbay, complete with build files and lock-free concurrency.
* ``PatchbayPcmSample`` Sample app playing a wav file through Patchbay. Crude
  but useful for prototyping and demos.
* ``PatchbayPd`` Library project providing an audio module implementation that
  uses libpd for synthesis. This project is a not a toy; libpd is a heavy-duty
signal processing library, and this project fully integrates it into Patchbay.
* ``PatchbayPdSample`` Sample app using ``PatchbayPd``.


Major components
----------------

Patchbay consists of a number of components. At its core is a Java class,
``Patchbay.java``, that manages the audio graph and handles audio input and
output with OpenSL ES. ``Patchbay.java`` is meant to be accessed through
``PatchbayService.java``, which can run as a remote or local service, i.e., you
can use Patchbay to route audio signals between different apps, or you can
build apps that just use Patchbay internally.

There are two APIs for interacting with the Patchbay service, a Java client API
for querying and manipulating the state of the signal processing graph, and a
hybrid C/Java module API for implementing new audio modules such as
synthesizers and effects.


Client API
----------

The Java client API is defined by two interfaces, ``IPatchbayService.aidl`` and
``IPatchbayClient.aidl``. In general, clients do not synthesize or process
audio but merely operate on the signal processing graph. The Patchbay service
implements the ``IPatchbayService`` interface. This interface provides a
collection of functions for querying and manipulating the signal processing
graph. Most clients will implement the ``IPatchbayClient`` interface, which
provides a collection of callbacks that the service will invoke to inform
clients of changes in the graph.

The sample app in ``PatchbayControl`` is a typical example of a client app that
does no audio processing of its own. Rather, it presents a simple user
interface that visually represents the graph and lets users connect or
disconnect audio modules. Through the ``IPatchbayClient`` interface, it keeps
its representation of the graph up to date.

Audio module API
----------------

On the Java side, the audio module API consists of one abstract base class,
``AudioModule.java``. Subclasses of ``AudioModule`` must implement a method
that registers the module's audio processing callback with the Patchbay
service, plus another method that releases the native resources held by the
module when they are no longer needed.

The audio processing callback will generally be implemented in C or C++, using
a small C API, defined in ``audio_module.h``, that only has two elements: A
function prototype that the signal processing callback must conform to, and a
function that registers the signal processing callback with the Patchbay
service.

The ``PatchbayLowpassSample`` project contains a typical audio module
implementation as well as a simple app that shows how to use an audio module.
It also illustrates how to set up the build system and how to interact with the
audio processing thread in a thread-safe yet lock-free manner.

Latency
-------

Patchbay incurs no latency on top of the systemic latency of the Android audio
stack; the audio processing callbacks of all active modules are invoked in one
buffer queue callback of OpenSL ES.

Device compatibility
--------------------

Patchbay pushes the limits of the current Android audio stack, and it works
better on some devices than on others. It is also a very young project that has
not yet been tested on a wide range of devices. It works well on Nexus 7 (both
new and old) as well as Nexus 10. On Galaxy Nexus, alas, it tends to glitch.
Further testing and evaluation is needed.


Patchbay and Google
-------------------

Patchbay started as a 20% project and Google owns the copyright.  Still, this
project is not an official Google product.

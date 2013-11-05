Patchfield
==========

Patchfield is an audio infrastructure for Android that provides a simple,
callback-driven API for implementing audio modules (such as synthesizers and
effects), a graph-based API for connecting audio modules, plus support for
inter-app audio routing. It is inspired by [JACK](http://jackaudio.org "JACK"),
the JACK audio connection kit.

Patchfield consists of a remote or local service that acts as a virtual
patchbay that audio apps can connect to, plus a number of sample apps that
illustrate how to implement audio modules and how to manipulate the signal
processing graph. The implementation resides entirely in userland and works on
many stock consumer devices, such as Nexus 7 and 10.

Forum
-----

If you have any questions or comments, don't hesitate to get in touch at
https://groups.google.com/d/forum/patchfield. If you implement a great project
on top of Patchbay, please tell us about it!

Device compatibility
--------------------

Patchfield requires SDK version 14 or later. It pushes the limits of the
current Android audio stack, and it works better on some devices than on
others. It is also a very young project that has not yet been tested on a wide
range of devices. Further testing and evaluation is needed.

Patchfield has been successfully tested on Nexus 10, Nexus 7 (both the original
version and the new one), Nexus S, and HTC-1. On Nexus 4, Patchfield works
well as long as the Patchfield service runs with foreground privileges.

Patchfield does not seem to work well on Galaxy Nexus (audio glitches when
switching between activities), and Galaxy S4 (frequent glitches). The glitches
on Galaxy Nexus seem to be unrelated to Patchfield; problems of this kind have
also been reported with other audio software.

Cloning and building Patchfield
-------------------------------

Make sure that you have Android NDK version r8e or later, and that
``ndk-build`` is on your search path, then clone the project and build the
native components like this:

```
git clone --recursive https://github.com/google/patchfield.git
cd patchfield
make
```

(Don't worry if ``javah`` reports errors on this initial build.) Now you can
import all projects in the patchfield directory into your development
environment of choice. If you build any native binaries after importing the
projects into Eclipse, make sure to refresh your Eclipse workspace or else
Eclipse may not see the latest version.

Project layout
--------------

* ``Patchfield`` Library project, the core of Patchfield; contains the
  ``PatchfieldService`` class and all APIs, as well as utilities.
* ``PatchfieldControl`` Sample control app and remote service configuration; you
  will need to install this app if you want to run Patchfield as a remote
service. (Note, however, that the Patchfield service does not depend on this
control app; they are merely being packaged together for convenience.)
* ``LowpassSample`` Sample app illustrating how to implement an audio module
  for Patchfield, complete with build files and lock-free concurrency.
* ``PcmSample`` Sample app playing a wav file through Patchfield.  Crude but
  useful for prototyping and demos.
* ``PatchfieldPd`` Library project providing an audio module implementation
  that uses libpd for synthesis. This project is a not a toy; libpd is a
heavy-duty signal processing library, and this project fully integrates it into
Patchfield.
* ``PdSample`` Sample app using ``PatchfieldPd``.

Further sample projects are available will be posted soon.

Major components
----------------

Patchfield consists of a number of components. At its core is a Java class,
``Patchfield.java``, that manages the audio graph and handles audio input and
output with OpenSL ES. ``Patchfield.java`` is meant to be accessed through
``PatchfieldService.java``, which can run as a remote or local service, i.e.,
you can use Patchfield to route audio signals between different apps, or you
can build apps that just use Patchfield internally.

There are two APIs for interacting with the Patchfield service, a Java client
API for querying and manipulating the state of the signal processing graph, and
a hybrid C/Java module API for implementing new audio modules such as
synthesizers and effects.

Patchfield introduces a new permission, USE_PATCHFIELD. Apps will only be
allowed to bind to ``PatchfieldService`` as a remote service if they have this
permission.

A note on deployment
--------------------

For the time being, the Patchfield service will not be available from Google
Play yet. The reason is that the API and protocol may change a few more times
before they are finalized, and so a published service would come with the risk
of drifting out of sync with client apps.

If you want to publish Patchfield-based apps on Google Play, you currently have
two options. If your app is self-contained, you can just package a copy of the
Patchfield service with your app. Or, if you want to support multiple apps that
exchange audio via Patchfield, you can publish your own copy of the Patchfield
service on Google Play and document the version of Patchbay that client apps
will have to use.

Right now the recommended way of using Patchfield is as a local service
packaged with your app. When the API and protocol have stabilized, the service
will be published on Google Play, and then inter-app audio routing via the
common remote service will be preferable.

Client API
----------

The Java client API is defined by two interfaces, ``IPatchfieldService.aidl``
and ``IPatchfieldClient.aidl``. In general, clients do not synthesize or
process audio but merely operate on the signal processing graph. The Patchfield
service implements the ``IPatchfieldService`` interface. This interface
provides a collection of functions for querying and manipulating the signal
processing graph. Most clients will implement the ``IPatchfieldClient``
interface, which provides a collection of callbacks that the service will
invoke to inform clients of changes in the graph.

The sample app in ``PatchfieldControl`` is a typical example of a client app
that does no audio processing of its own. Rather, it presents a simple user
interface that visually represents the graph and lets users connect or
disconnect audio modules. Through the ``IPatchfieldClient`` interface, it keeps
its representation of the graph up to date.

Audio module API
----------------

On the Java side, the audio module API consists of one abstract base class,
``AudioModule.java``. Subclasses of ``AudioModule`` must implement a method
that registers the module's audio processing callback with the Patchfield
service, plus another method that releases the native resources held by the
module when they are no longer needed.

The audio processing callback will generally be implemented in C or C++, using
a small C API, defined in ``audio_module.h``, that only has two elements: A
function prototype that the signal processing callback must conform to, and a
function that registers the signal processing callback with the Patchfield
service.

Apps that use audio modules also need to bind to a ``PatchfieldService``
instance because some of the audio module setup requires the
``IPatchfieldService`` interface.

The ``PatchfieldLowpassSample`` project contains a typical audio module
implementation as well as a simple app that shows how to use an audio module.
It also illustrates how to set up the build system and how to interact with the
audio processing thread in a thread-safe yet lock-free manner.

Sample format and buffer size
-----------------------------

Patchfield uses 32-bit float samples; buffers are non-interleaved.  The
Patchfield service operates at the native sample rate and buffer size of the
device. This means that audio modules must operate at the native sample rate
and buffer size as well. Native sample rates of 44100Hz and 48000Hz are common,
and so audio modules must support both. Moreover, audio modules must be
prepared to work with arbitrary buffer sizes.

In particular, apps cannot assume that the buffer size is a power of two.
Multiples of three, such as 144, 192, and 384, have been seen in the wild. The
Patchfield repository includes a utility library that performs buffer size
adaptation for audio modules that use synthesis techniques or audio libraries
that cannot operate at the native buffer size.

Navigation between Patchfield apps
----------------------------------

In order to navigate from the control app to an audio module, long-click on the
icon of the module in the control UI. In order to navigate from an audio module
to the control app, long-click on the back button. An abstract utility class,
``PatchfieldActivity.java``, implements this behavior and manages the
connection to the Patchfield service.

Apps that use Patchfield should launch with launch mode ``singleTask``. This
deviates from the usual recommendations for Android development, but it is
necessary for the navigation model of Patchfield.

Latency
-------

Patchfield incurs no latency on top of the systemic latency of the Android audio
stack; the audio processing callbacks of all active modules are invoked in one
buffer queue callback of OpenSL ES.

Patchfield and Google
---------------------

Patchfield started as a 20% project and Google owns the copyright.  Still, this
project is not an official Google product.

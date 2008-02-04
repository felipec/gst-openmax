= GstOpenMAX =

GstOpenMAX is a GStreamer plug-in that allows communication with OpenMAX IL
components.

OpenMAX IL is an industry standard that provides an abstraction layer for
computer graphics, video, and sound routines.

You'll need an OpenMAX IL implementation. Luckily STMicroelectronics has
provided an useful sample implementation in the Bellagio project which is Open
Source.

For more information check:
http://freedesktop.org/wiki/GstOpenMAX

== Why is it good? ==

The easiest way to see OpenMAX IL is to think about OpenGL. both standards are
controlled by the Khronos Group, both standards are open and royalty free.

In a similar way you can think of Bellagio/OpenMAX IL as Mesa/Open GL. They are
not official implementations, but they follow the standard.

There are many Open Source encoders/decoders, but they all have different
API's, so in order to use it you need to learn the API. In fact some projects
as FFmpeg constantly change their API, so it makes things difficult.

If you use Bellagio's omxil you don't have to worry about FFmpeg's API changes,
those are wrapped inside a standard API.

The standard has been reviewed by several parties including: Texas Instruments,
ARM, ATI Technologies, Beatnik, Broadcom, Emuzed, Fraunhofer, Freescale,
Infineon, Intel, Motorola, Nokia, NVIDIA, Philips, SKY MobileMedia, Samsung,
Sasken, Siemens, ST Microelectronics, and Symbian.

The gst-openmax project is a collaboration between Fluendo, Nokia,
STMicroelectronics, Texas Instruments and the Open Source community.

== What's bad? ==

Not all of the codecs are defined. For example Vorbis is defined, but not
Theora. Of course it could be added as an extension, but that API wouldn't be
set in stone

== How to build omxil ==

Download the source code from:

 * http://omxil.sourceforge.net/

Or directly from:

 * http://dl.sourceforge.net/sourceforge/omxil/libomxil-B-0.3.3.tar.gz

Install as usual:

 ./configure --prefix="$HOME/omx"
 make
 make install

Register the components:

 $HOME/omx/bin/omxregister -v

You'll get an output like this:

 Component OMX.st.volume.component registered
   Specific role OMX.st.volume.component registered
 Component OMX.st.audio_decoder.ogg.single registered
   Specific role OMX.st.audio_decoder.ogg.single registered
 Component OMX.st.alsa.alsasink registered
   Specific role OMX.st.alsa.alsasink registered
 Component OMX.st.audio_filereader registered
 Component OMX.st.audio_decoder registered
   Specific role OMX.st.audio_decoder.mp3 registered
   Specific role OMX.st.audio_decoder.ogg registered
   Specific role OMX.st.audio_decoder.aac registered
 Component OMX.st.video_decoder registered
   Specific role OMX.st.video_decoder.mpeg4 registered
   Specific role OMX.st.video_decoder.avc registered
 Component OMX.st.video_colorconv.ffmpeg registered
   Specific role OMX.st.video_colorconv.ffmpeg registered
 Component OMX.st.audio_decoder.mp3.mad registered
   Specific role OMX.st.audio_decoder.mp3.mad registered

  8 OpenMAX IL ST static components with 11 roles succesfully scanned

Make sure you have:

 * "OMX.st.audio_decoder.mp3.mad" for MP3 decoding
 * "OMX.st.audio_decoder.ogg.single" for Vorbis decoding
 * "OMX.st.video_decoder.mpeg4" for MPEG4 video decoding
 * "OMX.st.alsa.alsasink" for ALSA sound output

== How to build gst-openmax ==

If you are fetching it from CVS:

 ./autogen.sh --noconfigure

If you want to install it into your home directory:

 ./configure --prefix="$HOME/omx"

 make

Then as root (depending on your prefix):

 make install

== How to try it ==

 export GST_PLUGIN_PATH=$HOME/omx/lib/gstreamer-0.10
 export LD_LIBRARY_PATH=$HOME/omx/lib

You'll be able to play videos with MP3, Vorbis, MPEG-4, H.264 or H.263 content using Bellagio's components.

If you want to see what's happening:

 export GST_DEBUG=omx:4

== Links ==

 * http://omxil.sourceforge.net/
 * http://freedesktop.org/wiki/GstOpenMAX

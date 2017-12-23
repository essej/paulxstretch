PaulXStretch - Plugin for extreme time stretching and other spectral processing of audio

Copyright (C) 2006-2011 Nasca Octavian Paul, Tg. Mures, Romania

Copyright (C) 2017 Xenakios

Released under GNU General Public License v.2 license.

Source code at (currently missing GPL license for some of the source files, will be fixed later) :

https://bitbucket.org/xenakios/paulstretchplugin/overview

Requirements for building from source code :
    -C++17 compiler and C++17 standard library
    -JUCE 5.2 : https://github.com/WeAreROLI/JUCE
    -FFTW3
	
History :

12-15-2017 1.0.0 preview 1
	-Very early public release. Various issues present.
12-17-2017 1.0.0 preview 2
	-Report only 2 input channels to host
	-Fixes for using potentially invalid sample rates in internal initializations and calculations
	-File import dialog allows importing file formats supported by JUCE, not just .wav
12-17-2017 1.0.0 preview 2b
	-Fix buffer channel count issue, seems to make AU validation pass consistently
12-22-2017 1.0.0 preview 3
	-Added parameter to set audio input capture buffer length (up to 120 seconds)
	-Add button to show settings menu
	-Fix bug when offline rendering in host
	-Allow setting background prebuffering amount in settings, including none. 
	 (None is mostly useful in case the plugin doesn't detect the host is offline rendering. 
	 For real time playback none is likely only going to work with small FFT sizes.)
	-Slightly better GUI layout but still mostly just 2 columns of parameters
	-Remember last file import folder

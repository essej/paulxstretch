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

02-07-2018 1.0.0
	-Control/Command click on waveform seeks (if click within active play range)
	-Moved prebuffering amount menu to prebuffering meter (click to show)
02-02-2018 1.0.0 preview 5
	-Added buttons to enable/disable spectral processing modules
	-Restored ability to set capture buffer length (via the settings menu)
	-Seek to play range beginning when audio file imported
	-No longer seeks to beginning of play range when changing FFT size
	-Fixes to waveform display issues
	-Double click on waveform selects whole waveform
	-Double click on slider resets parameter to default value
	-Show prebuffering ready amount graphically instead of text
	-Highlight related parameters when spectral processing module clicked in the module chain
01-05-2018 1.0.0 preview 4
	-Added reset parameters (except main volume and input pass through) command to settings menu
	-Added option to settings menu to ignore loading imported audio file when recalling state
	-Added support for dropping audio files to GUI (available functionality depends on host, many will simply 
	 provide the file name of the source of audio file, so audio clip/event/item specific audio won't be imported)
	-Removed an unnecessary level of buffering (doesn't reduce latency but should help a bit with CPU usage)
	-Added About window
12-23-2017 1.0.0 preview 3
	-Added parameter to set audio input capture buffer length (up to 120 seconds)
	-Added parameter allow passing through audio from plugin input
	-Inverted the number of harmonics parameter active range
	-Added button to show settings menu
	-Fix bug when offline rendering in host
	-Allow setting background prebuffering amount in settings, including none. 
	 (None is mostly useful in case the plugin doesn't detect the host is offline rendering. 
	 For real time playback none is likely only going to work with small FFT sizes.)
	-Slightly better GUI layout but still mostly just 2 columns of sliders
	-Remember last file import folder
	-Added detection of invalid audio output sample values (infinities, NaN)
12-17-2017 1.0.0 preview 2b
	-Fix buffer channel count issue, seems to make AU validation pass consistently
12-17-2017 1.0.0 preview 2
	-Report only 2 input channels to host
	-Fixes for using potentially invalid sample rates in internal initializations and calculations
	-File import dialog allows importing file formats supported by JUCE, not just .wav
12-15-2017 1.0.0 preview 1
	-Very early public release. Various issues present.

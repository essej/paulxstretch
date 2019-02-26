PaulXStretch - Plugin for extreme time stretching and other spectral processing of audio

Copyright (C) 2006-2011 Nasca Octavian Paul, Tg. Mures, Romania

Copyright (C) 2017-2018 Xenakios

Released under GNU General Public License v.3 license.

History :
02-26-2019 1.2.4
	-Changed Import file button to show/hide an overlaid file browser for audio files. (This seems to be the only 
	 technically correct way to browse for the files.)
	-Fix the Capture parameter. (Recalling it when undoing and loading project should now be ignored, 
	 while still allowing automation and MIDI learn.)
01-17-2019 1.2.3
	-Captured audio buffers can optionally be saved as files on disk for later recall.
	-Added varispeed (resampling speed change) feature when spectral stretch engine is bypassed
	-Added shortcut key (may not work properly in all plugin formats and hosts) :
		"I" to open file import dialog
	-Attempt to prevent capture enabled state from being recalled when undoing in the host
07-09-2018 1.2.2
	-Add option to mute audio when capturing audio
	-Automatically adjust play range after capturing to captured length
	-Moved Free Filter parameters from the main parameters GUI to the Free Filter tab page
06-01-2018 1.2.1
	-Added looping enabled parameter
	-Added GUI button and parameter to rewind to beginning of selected play range
	-Flush old stretched audio faster when source audio is changed
	-Fix play range not being recalled properly when loading host project
05-07-2018 1.2.0
	-Changed "Octaves" module to "Ratios". The Ratios module has more shifters than the previous 
	 Octaves module and allows changing the pitch ratios (and the shifters mix) 
	 in a separate tabbed page in the GUI.
	-Spectral module enabled parameters changed to target particular modules instead of chain slots
	-Save and restore some additional settings
04-01-2018 1.1.2 
	-Rebuilt with latest JUCE to fix parameter automation issue for example in Ableton Live
	-Optimization in calculation of transformed free filter envelope
03-21-2018 1.1.1
	-Removed code that accidentally got included in 1.1.0
03-21-2018 1.1.0
	-Added free filter spectral processing module
	 *Yellow envelope line is the editable envelope, blue line is the envelope transformed with the 
	 free filter plugin parameters, used for the audio processing. (These are initially overlapped, 
	 tweak the free filter parameters to see them separated.)
	 *Click on envelope view to add point
	 *Alt-click on envelope point to remove point
	 *Alt-drag envelope line to change curvature
	 *Drag envelope line to move line up/down
	 *Right-click envelope view to show additional actions/options
	-Made spectral module on/off states plugin parameters
	-Fixed bug with the number of harmonics parameter not getting saved and recalled.
	-Fixed bugs with calculations involving samplerate
02-23-2018 1.0.2
	-Added stretch processing bypass parameter (to play the original sound looped like it is passed into the stretcher)
	-Show approximate stretched output duration in info label (only valid if the stretch amount is not automated in the host)
	-Waveform selection can be moved by dragging the selection top area
	-Smoothed playback with fades when changing waveform selection (doesn't work ideally, fixing later...)
	-Fixes for the waveform graphics disappearing unexpectedly (this probably still isn't entirely fixed, though)
02-16-2018 1.0.1
	-Increased maximum number of input channels to 8
	-Added zoom/scroll bar for waveform
	-GUI performance improvement/bug fix during capture mode
	-Shorter crossfade length when changing FFT size
02-09-2018 1.0.0
	-Control/Command click on waveform seeks (if click within active play range)
	-Moved prebuffering amount menu to prebuffering meter (click to show)
	-Added dummy parameter to tell the host the plugin state has changed when importing files etc.
	 (May not work properly for undo etc on all hosts.)
	-Removed the factory presets as they are not really that useful
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

Source code at :

https://bitbucket.org/xenakios/paulstretchplugin/overview

Requirements for building from source code :
    -A modern C++ compiler and standard library (C++14, some C++17 needs to be supported)
    -JUCE 5.3.2 : https://github.com/WeAreROLI/JUCE
    -FFTW3.3.6

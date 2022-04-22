### PaulXStretch - Plugin for extreme time stretching and other spectral processing of audio

This application/plugin is based on the PaulStretch algorithm. (Paul’s Extreme Time Stretch, originally developed by Nasca Octavian Paul), and specifically the PaulXStretch version from Xenakios. The UI has been updated and adapted for various screen sizes.

PaulXStretch is only suitable for radical transformation of sounds. It is not suitable at all for subtle time or pitch corrections and such. Ambient music and sound design are probably the most suitable use cases. It can turn any short audio into an hours long ambient soundscape without batting an eye!

Works as a standalone application on macOS, Windows, Linux, and iOS, and as an audio plugin (AU, VST3) on macOS, Windows and iOS (AUv3).


<img src="https://sonosaurus.com/paulxstretch/assets/images/paulxstretch_screenshot.png" width="938" />

# Installing

## Windows and Mac
There are binary releases for macOS and Windows available at [sonosaurus.com/paulxstretch](https://sonosaurus.com/paulxstretch) or in the releases of this repository on GitHub.

## Linux

For now, you can build it yourself following the [build instructions](#on-linux) below.


# Building

Source code is now maintained at:

https://github.com/essej/paulxstretch

To build from source on macOS and Windows, all of the dependencies are a part of this GIT repository, including prebuilt FFTW libraries (for Mac/Win only) 
The build now uses [CMake](https://cmake.org) 3.15 or above on macOS, Windows, and Linux platforms, see
details below.


### On macOS

Make sure you have [CMake](https://cmake.org) >= 3.15 and XCode. Then run:
```
./setupcmake.sh
./buildcmake.sh
``` 
The resulting application and plugins will end up under `build/PaulXStretch_artefacts/Release`
when the build completes. If you would rather have an Xcode project to look
at, use `./setupcmakexcode.sh` instead and use the Xcode project that gets
produced at `buildXcode/PaulXStretch.xcodeproj`.

### On Windows

You will need [CMake](https://cmake.org) >= 3.15, and  Visual Studio 2019
installed. You'll also need Cygwin installed if you want to use the scripts
below, but you can also use CMake in other ways if you prefer.

```
./setupcmakewin.sh
./buildcmake.sh
``` 
The resulting application and plugins will end up under `build/PaulXStretch_artefacts/Release`
when the build completes. The MSVC project/solution can be found in
build/PaulXStretch_artefacts as well after the cmake setup step.


### On Linux

The first thing to do in a terminal is go to the Linux directory:

    cd linux

And read the [BUILDING.md](linux/BUILDING.md) file for
further instructions.

  

# Contributors

Copyright (C) 2022 Jesse Chappell

Copyright (C) 2017-2018 Xenakios

Copyright (C) 2006-2011 Nasca Octavian Paul, Tg. Mures, Romania


# License and 3rd Party Software

Released under GNU General Public License v.3 license with App Store license
exception. The full license text is in the LICENSE and LICENSE_EXCEPTION files. Paul Nasca, Xenakios and Jesse Chappell all explicitly permitted the license exception clause.


It is built using JUCE 6 (slightly modified on a public fork), I'm using the very handy tool `git-subrepo` to include the source code for my forks of those software libraries in this repository.
FFTW is required, but statically built libraries are included in `deps` for easier building on Mac and Windows.

My github forks of these that are referenced via `git-subrepo` in this repository are:

> https://github.com/essej/JUCE  in the sono6good branch.

The version distributed on the iOS App Store by Sonosaurus is not built with FFTW, and the JUCE commercial license applies there.






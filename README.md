### PaulXStretch - Plugin for extreme time stretching and other spectral processing of audio

This application/plugin is based on the PaulStretch algorithm. (Paul’s Extreme Time Stretch, originally developed by Nasca Octavian Paul), and specifically the PaulXStretch version from Xenakios. The UI has been updated and adapted for various screen sizes.

PaulXStretch is only suitable for radical transformation of sounds. It is not suitable at all for subtle time or pitch corrections and such. Ambient music and sound design are probably the most suitable use cases. It can turn any short audio into an hours long ambient soundscape without batting an eye!

# Building

Source code is now maintained at:

https://github.com/essej/paulxstretch

Requirements for building from source code :
  - A modern C++ compiler and standard library (C++14, some C++17 needs to be supported)
  - JUCE 6.X is included in deps/juce and is a slightly modified branch with
    some improvements/augmentations. It is managed via the tool git-subrepo
and the original repository is: https://github.com/essej/JUCE in the
```sono6good``` branch
  - FFTW >= 3.3.6

# Contributors

Copyright (C) 2006-2011 Nasca Octavian Paul, Tg. Mures, Romania

Copyright (C) 2017-2018 Xenakios

Copyright (C) 2022 Jesse Chappell

# License
Released under GNU General Public License v.3 license with App Store license
exception.




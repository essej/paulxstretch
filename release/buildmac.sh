#!/bin/bash

cd ../Builds/MacOSX

if ! xcodebuild -configuration Release -scheme "PaulXStretch - All" ; then

  echo Error BUILDING
  exit 2
fi



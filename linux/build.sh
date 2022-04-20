#!/bin/bash

cd ..

if ./setupcmake.sh
then

   #make -C build clean

   ./buildcmake.sh
else
  echo 
  echo "ERROR setting up cmake, look for errors above and report them if asking for help."
  echo
fi




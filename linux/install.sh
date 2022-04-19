#!/bin/bash

PREFIX=/usr/local

if [ -n "$1" ] ; then
  PREFIX="$1"
fi

echo "Installing PaulXStretch to ${PREFIX} ... (specify destination as command line argument if you want it elsewhere)"

BUILDDIR=../build/PaulXStretch_artefacts/Release

mkdir -p ${PREFIX}/bin
if ! cp ${BUILDDIR}/Standalone/paulxstretch  ${PREFIX}/bin/paulxstretch ; then
  echo
  echo "Looks like you need to run this as 'sudo $0'"
  exit 2
fi

mkdir -p ${PREFIX}/share/applications
cp paulxstretch.desktop ${PREFIX}/share/applications/paulxstretch.desktop
chmod +x ${PREFIX}/share/applications/paulxstretch.desktop

mkdir -p ${PREFIX}/share/pixmaps
cp ../images/paulxstretch_logo@2x.png ${PREFIX}/share/pixmaps/paulxstretch.png

if [ -d ${BUILDDIR}/VST3/PaulXStretch.vst3 ] ; then
  mkdir -p ${PREFIX}/lib/vst3
  cp -a ${BUILDDIR}/VST3/PaulXStretch.vst3 ${PREFIX}/lib/vst3/

  echo "PaulXStretch VST3 plugin installed"
fi


echo "PaulXStretch application installed"


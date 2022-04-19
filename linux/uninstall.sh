#!/bin/bash

PREFIX=/usr/local

if [ -n "$1" ] ; then
  PREFIX="$1"
fi

echo "Un-Installing PaulXStretch from ${PREFIX} ... (specify destination as command line argument if you have it elsewhere)"


if [ -f ${PREFIX}/bin/paulxstretch ] ; then
  if ! rm -f ${PREFIX}/bin/paulxstretch ; then
    echo
    echo "Looks like you need to run this with 'sudo $0'"
    exit 2
  fi
fi

rm -f ${PREFIX}/share/applications/paulxstretch.desktop
rm -f ${PREFIX}/pixmaps/paulxstretch.png

rm -rf ${PREFIX}/lib/vst3/PaulXStretch.vst3

echo "PaulXStretch uninstalled"

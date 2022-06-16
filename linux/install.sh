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
cp ../images/paulxstretch_icon_1024_rounded.png ${PREFIX}/share/pixmaps/paulxstretch.png

if [ -d ${BUILDDIR}/VST3/PaulXStretch.vst3 ] ; then
  mkdir -p ${PREFIX}/lib/vst3
  cp -a ${BUILDDIR}/VST3/PaulXStretch.vst3 ${PREFIX}/lib/vst3/

  echo "PaulXStretch VST3 plugin installed"
fi

if [ -f ${BUILDDIR}/CLAP/PaulXStretch.clap ] ; then
  #mkdir -p ${PREFIX}/lib/clap
  if [ x"$SUDO_USER" != x ] ; then
        CLAPDIR=`eval echo ~$SUDO_USER/.clap`
	mkdir -p $CLAPDIR
       	chown $SUDO_USER:$SUDO_GROUP $CLAPDIR
       	cp -f ${BUILDDIR}/CLAP/PaulXStretch.clap $CLAPDIR/
  	chown $SUDO_USER:$SUDO_GROUP $CLAPDIR/PaulXStretch.clap
  else
  	CLAPDIR=$HOME/.clap
  	mkdir -p $CLAPDIR
  	cp -a ${BUILDDIR}/CLAP/PaulXStretch.clap $CLAPDIR/
  fi

  echo "PaulXStretch CLAP plugin installed in $CLAPDIR"
fi


echo "PaulXStretch application installed"


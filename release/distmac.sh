#!/bin/bash

if [ -z "$1" ] ; then
   echo "Usage: $0 <version>"
   exit 1
fi

VERSION=$1


MAINNAME="PaulXStretch"

#BUILDDIR=../Builds/MacOSX/build/Release
BUILDDIR=../build/${MAINNAME}_artefacts/Release
#INSTBUILDDIR=../build/${MAINNAME}Inst_artefacts/Release

rm -rf ${MAINNAME}

mkdir -p ${MAINNAME}


#cp ../doc/README_MAC.txt ${MAINNAME}/

cp -pLRv ${BUILDDIR}/Standalone/${MAINNAME}.app  ${MAINNAME}/
cp -pLRv ${BUILDDIR}/AU/${MAINNAME}.component  ${MAINNAME}/
cp -pLRv ${BUILDDIR}/VST3/${MAINNAME}.vst3 ${MAINNAME}/
cp -pRHv ${BUILDDIR}/AAX/${MAINNAME}.aaxplugin  ${MAINNAME}/

#cp -pLRv ${BUILDDIR}/${MAINNAME}.app  ${MAINNAME}/
#cp -pLRv ${BUILDDIR}/${MAINNAME}.component  ${MAINNAME}/
#cp -pLRv ${BUILDDIR}/${MAINNAME}.vst3 ${MAINNAME}/



# this codesigns and notarizes everything
if ! ./codesign.sh ; then
  echo
  echo Error codesign/notarizing, stopping
  echo
  exit 1
fi

# make installer package (and sign it)

rm -f macpkg/${MAINNAME}Temp.pkgproj

if ! ./update_package_version.py ${VERSION} macpkg/${MAINNAME}.pkgproj macpkg/${MAINNAME}Temp.pkgproj ; then
  echo
  echo Error updating package project versions
  echo
  exit 1
fi

if ! packagesbuild  macpkg/${MAINNAME}Temp.pkgproj ; then
  echo 
  echo Error building package
  echo
  exit 1
fi

mkdir -p ${MAINNAME}Pkg
rm -f ${MAINNAME}Pkg/*

if ! productsign --sign ${INSTSIGNID} --timestamp  macpkg/build/${MAINNAME}\ Installer.pkg ${MAINNAME}Pkg/${MAINNAME}\ Installer.pkg ; then
  echo 
  echo Error signing package
  echo
  exit 1
fi

# make dmg with package inside it

#exit

if ./makepkgdmg.sh $VERSION ; then

   ./notarizedmg.sh ${VERSION}/${MAINNAME}-${VERSION}-mac.dmg

   echo
   echo COMPLETED DMG READY === ${VERSION}/${MAINNAME}-${VERSION}-mac.dmg
   echo
   
fi

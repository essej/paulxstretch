#!/bin/bash

if [ -z "$1" ] ; then
  echo "Usage: $0 <version>"
  exit 1
fi

VERSION=$1
MAINNAME="PaulXStretch"
MAINNAMELC="paulxstretch"

rm -f ${MAINNAME}Pkg.dmg

# cp ${MAINNAME}/README_MAC.txt ${MAINNAME}Pkg/

if dropdmg --config-name=${MAINNAME}Pkg --layout-folder ${MAINNAME}PkgLayout --volume-name="${MAINNAME} v${VERSION}"  --APP_VERSION=v${VERSION}  --signing-identity=C7AF15C3BCF2AD2E5C102B9DB6502CFAE2C8CF3B ${MAINNAME}Pkg
then
  mkdir -p ${VERSION}
  mv -v ${MAINNAME}Pkg.dmg ${VERSION}/${MAINNAMELC}-${VERSION}-mac.dmg
else
  echo "Error making package DMG"
  exit 2
fi


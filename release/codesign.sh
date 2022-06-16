#!/bin/bash


MAINNAME="PaulXStretch"

# codesign them with developer ID cert

POPTS="--strict  --force --options=runtime --sign C7AF15C3BCF2AD2E5C102B9DB6502CFAE2C8CF3B --timestamp"
AOPTS="--strict  --force --options=runtime --sign C7AF15C3BCF2AD2E5C102B9DB6502CFAE2C8CF3B --timestamp"

codesign ${AOPTS} --entitlements ${MAINNAME}.entitlements ${MAINNAME}/${MAINNAME}.app
codesign ${POPTS} --entitlements ${MAINNAME}.entitlements  ${MAINNAME}/${MAINNAME}.component
codesign ${POPTS} --entitlements ${MAINNAME}.entitlements ${MAINNAME}/${MAINNAME}.vst3
codesign ${POPTS} --entitlements ${MAINNAME}.entitlements ${MAINNAME}/${MAINNAME}.clap


# AAX is special
if [ -n "${PSAAXSIGNCMD}" ]; then
   echo "Signing AAX plugin"
   ${PSAAXSIGNCMD}  --in ${MAINNAME}/${MAINNAME}.aaxplugin --out ${MAINNAME}/${MAINNAME}.aaxplugin
fi



if [ "x$1" = "xonly" ] ; then
  echo Code-signing only
  exit 0
fi


mkdir -p tmp

# notarize them in parallel
./notarize-app.sh --submit=tmp/sbapp.uuid  ${MAINNAME}/${MAINNAME}.app
./notarize-app.sh --submit=tmp/sbau.uuid ${MAINNAME}/${MAINNAME}.component
./notarize-app.sh --submit=tmp/sbvst3.uuid ${MAINNAME}/${MAINNAME}.vst3
./notarize-app.sh --submit=tmp/sbclap.uuid ${MAINNAME}/${MAINNAME}.clap
#./notarize-app.sh --submit=tmp/sbinstvst3.uuid SonoBus/SonoBusInstrument.vst3
#./notarize-app.sh --submit=tmp/sbvst2.uuid SonoBus/SonoBus.vst 

if ! ./notarize-app.sh --resume=tmp/sbapp.uuid ${MAINNAME}/${MAINNAME}.app ; then
  echo Notarization App failed
  exit 2
fi

if ! ./notarize-app.sh --resume=tmp/sbau.uuid ${MAINNAME}/${MAINNAME}.component ; then
  echo Notarization AU failed
  exit 2
fi

if ! ./notarize-app.sh --resume=tmp/sbvst3.uuid ${MAINNAME}/${MAINNAME}.vst3 ; then
  echo Notarization VST3 failed
  exit 2
fi

if ! ./notarize-app.sh --resume=tmp/sbclap.uuid ${MAINNAME}/${MAINNAME}.clap ; then
  echo Notarization CLAP failed
  exit 2
fi




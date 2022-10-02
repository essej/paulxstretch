# This generates the clap.icns file from the artwork PNG

IN=../../clap-libs/clap/artwork/clap-simple-logo-black.png

mkdir OUT
mkdir OUT/PNG
convert -geometry 512 $IN OUT/PNG/icon_512.png
convert -geometry 256 $IN OUT/PNG/icon_256.png
convert -geometry 128 $IN OUT/PNG/icon_128.png
convert -geometry 64 $IN OUT/PNG/icon_64.png


identify OUT/PNG/*png


mkdir OUT/SET.iconset
sips -z 512 512 OUT/PNG/icon_512.png --out OUT/SET.iconset/icon_512x512.png
sips -z 256 256 OUT/PNG/icon_512.png --out OUT/SET.iconset/icon_256x256.png
sips -z 128 128 OUT/PNG/icon_512.png --out OUT/SET.iconset/icon_128x128.png
sips -z 64 64 OUT/PNG/icon_512.png --out OUT/SET.iconset/icon_64x64.png

iconutil -c icns -o clap.icns OUT/SET.iconset

rm -rf OUT

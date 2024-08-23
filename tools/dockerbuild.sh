#!/bin/sh

WORKING_DIR=$(pwd)
BUILD_DIR=$(pwd)/build_docker
OUT_DIR=$(pwd)/out

mkdir -p "$BUILD_DIR"
rm -rf "$BUILD_DIR/*"
mkdir -p "$OUT_DIR"

cd "$BUILD_DIR"
CMAKE_OUTPUT=$(cmake -DMODXO_PINOUT=$MODXO_PINOUT -DCMAKE_BUILD_TYPE=$BUILD_TYPE .. 2>&1 | tee /dev/tty)
BOARD=$(echo "$CMAKE_OUTPUT" | sed -n 's/^-- Building for \([^ .]*\)\.$/\1/p')

if [ $CLEAN == 'y' ]; then
    make clean
fi

make -j
if [ $? -ne 0 ]; then
    echo "Build failed"
    exit 1
fi

if [ $? -ne 0 ]; then
    echo "Build failed"
    exit 1
fi

mv "${BUILD_DIR}/modxo_${BOARD}.bin" "${OUT_DIR}/" && \
mv "${BUILD_DIR}/modxo_${BOARD}.uf2" "${OUT_DIR}/" && \
mv "${BUILD_DIR}/modxo_${BOARD}.elf" "${OUT_DIR}/" && \
echo "Build successful. Files are in ${OUT_DIR}" || \
echo "Build failed"


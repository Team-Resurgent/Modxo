#!/bin/sh

# MODXO_PINOUT=${MODXO_PINOUT:-'modxo_official_pico'}
CLEAN=${CLEAN:-0}
# BUILD_TYPE=${BUILD_TYPE:-"Debug"} # Default build type
BIOS2UF2=${BIOS2UF2:-"bios.bin bios/*.bin"}

WORKING_DIR=$(pwd)
BUILD_DIR=$(pwd)/build_docker
OUT_DIR=$(pwd)/out
BIOS2UF2_TOOL=$(pwd)/tools/flashbin_to_uf2.py

mkdir -p "$BUILD_DIR"
rm -rf "$BUILD_DIR/*"
mkdir -p "$OUT_DIR"

cd "$BUILD_DIR"
CMAKE_OUTPUT=$(cmake -DMODXO_PINOUT=$MODXO_PINOUT -DCMAKE_BUILD_TYPE=$BUILD_TYPE .. 2>&1 | tee /dev/tty)
BOARD=$(echo "$CMAKE_OUTPUT" | sed -n 's/^-- Building for \([^ .]*\)\.$/\1/p')

if [ $CLEAN -eq 1 ]; then
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

if [ -n "$BIOS2UF2" ]; then
    FULL_PATHS=""

    for file in $BIOS2UF2; do
        FULL_PATHS="$FULL_PATHS $WORKING_DIR/$file"
    done

    # Trim leading space
    FULL_PATHS=$(echo "$FULL_PATHS" | sed 's/^ //')

    python $BIOS2UF2_TOOL --output-dir "${OUT_DIR}" $FULL_PATHS
fi

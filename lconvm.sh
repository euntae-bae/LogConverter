#!/bin/bash

./lconv1 sensor-ax.txt
./lconv1 sensor-ay.txt
./lconv1 sensor-az.txt
./lconv4 "$1"
outDir="lconv-out-$(date +%Y%m%d%H%M%S)"
mkdir $outDir
mv sensor-out.txt "./$outDir"
mv sensor-win.txt "./$outDir"
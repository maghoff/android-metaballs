#!/bin/bash

./build.sh && # For good measure :)
ant release &&
jarsigner -signedjar Metaballs-signed.apk bin/GL2JNIActivity-unsigned.apk mykey &&
~/Downloads/android-sdk-linux_86/tools/zipalign 4 Metaballs-signed.apk Metaballs.apk


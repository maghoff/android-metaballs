#!/bin/bash

# The NDK-bits
../../ndk-build &&

# The Java-bits:
~/Downloads/android-sdk-linux_86/tools/android update project -p . &&
ant compile &&
ant install


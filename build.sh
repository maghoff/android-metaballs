#!/bin/bash

~/Downloads/android-sdk-linux_86/tools/android update project -p .
ant compile
ant install


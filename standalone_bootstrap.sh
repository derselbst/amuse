#!/bin/sh

# MAKE SURE CLANG13-devel is installed - newer clang will NOT WORK!!!

git clone https://github.com/AxioDL/boo.git
(cd boo && git submodule update --recursive --init)

#git clone https://github.com/libAthena/athena.git
#(cd athena && git checkout f7c1cd8f59e522651b6cf1f8df011028940abee3~1 && git submodule update --recursive --init)

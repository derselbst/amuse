#!/bin/sh
git clone https://github.com/AxioDL/boo.git
(cd boo && git submodule update --recursive --init)

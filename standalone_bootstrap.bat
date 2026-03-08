@echo off
git clone https://github.com/AxioDL/boo.git
pushd boo
git submodule update --recursive --init
popd

git clone https://github.com/libAthena/athena.git
pushd athena
git checkout fa346ac52bfb83bf51f3f55248fa07a966b356f8
git submodule update --recursive --init
popd

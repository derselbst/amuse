@echo off
git clone https://github.com/AxioDL/boo.git
pushd boo
git submodule update --recursive --init
popd

git clone https://github.com/libAthena/athena.git
pushd athena
git checkout f7c1cd8f59e522651b6cf1f8df011028940abee3~1
git submodule update --recursive --init
popd

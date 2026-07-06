#!/bin/bash
set -e

scriptdir=${0%`basename "$0"`}
cd $scriptdir
scriptdir=`pwd`

. .common.inc.sh

# Pin ESP-IDF to a known version explicitly, so a from-scratch build is
# deterministic regardless of the recorded submodule gitlink.
git submodule update --init esp-idf
git -C esp-idf fetch --tags origin
git -C esp-idf checkout v5.5.4
git -C esp-idf submodule update --init --recursive

rm -rf esp-idf-tools
mkdir -p esp-idf-tools
esp-idf/install.sh
rm -rf esp-idf-tools/dist

esp-idf/tools/idf_tools.py install-python-env

ENV=$(echo "$scriptdir"/esp-idf-tools/python_env/idf*_py3*_env/bin/python)
$ENV -m pip install setuptools wheel

echo "init done"

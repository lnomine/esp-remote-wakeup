#!/bin/bash
set -e

scriptdir=${0%`basename "$0"`}
cd $scriptdir
scriptdir=`pwd`

. .common.inc.sh

reset_build_ts
idf_docker build

echo "build finished successfully"

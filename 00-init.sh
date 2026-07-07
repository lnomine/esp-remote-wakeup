#!/bin/bash
set -e

scriptdir=${0%`basename "$0"`}
cd $scriptdir
scriptdir=`pwd`

. .common.inc.sh

echo "pulling $IDF_IMAGE ..."
docker pull "$IDF_IMAGE"

echo "init done"
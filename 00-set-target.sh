#!/bin/bash
set -e

scriptdir=${0%`basename "$0"`}
cd $scriptdir
scriptdir=`pwd`

. .common.inc.sh

idf_docker set-target ${1:-$IDF_TARGET}

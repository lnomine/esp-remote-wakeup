#!/bin/bash
set -e

scriptdir=${0%`basename "$0"`}
cd $scriptdir
scriptdir=`pwd`

. .common.inc.sh
parse_args $@

idf_docker_with_device "$port_flash" --port "$port_flash" flash

echo "flash finished successfully"
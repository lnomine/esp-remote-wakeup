IDF_IMAGE="${IDF_IMAGE:-espressif/idf:release-v5.5}"
IDF_TARGET="${IDF_TARGET:-esp32s3}"

print_common_usage() {
	echo "usage: $0 -p [port]"
	echo "note: you can define default ports in the defport-monitor and defport-flash files"
	exit 1
}

parse_args() {
	while getopts "p:kf" o; do
	    case "${o}" in
			p)
				port=${OPTARG}
				;;
			f)
				force=1
				;;
			*)
				print_common_usage
				;;
	    esac
	done
	shift $((OPTIND-1))

	if [ -z "$port_monitor" ] && [ -e defport-monitor ]; then
		port_monitor=`cat defport-monitor`
	fi
	if [ -z "$port_flash" ] && [ -e defport-flash ]; then
		port_flash=`cat defport-flash`
	fi
	if [ -z "$port_monitor" ] || [ -z "$port_flash" ]; then
		print_common_usage
	fi
}

idf_docker() {
	docker run --rm -it \
		-v "$scriptdir":/project \
		-w /project \
		-u "$(id -u)" \
		-e HOME=/tmp \
		-e IDF_GIT_SAFE_DIR='*' \
		"$IDF_IMAGE" idf.py "$@"
}

idf_docker_with_device() {
	local device=$1
	shift
	docker run --rm -it \
		-v "$scriptdir":/project \
		-w /project \
		-u "$(id -u)" \
		-e HOME=/tmp \
		-e IDF_GIT_SAFE_DIR='*' \
		--device="$device" \
		--group-add dialout \
		"$IDF_IMAGE" idf.py "$@"
}

reset_build_ts() {
	rm -f build/esp-idf/app_update/CMakeFiles/__idf_app_update.dir/esp_app_desc.c.obj
}

do_clean() {
	idf_docker clean
	idf_docker fullclean
	rm -rf "$scriptdir/build"
}

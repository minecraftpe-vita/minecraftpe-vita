#!/bin/sh
#
# This is just a wrapper for `makesis` that also takes care of copying assets into EPOCROOT
#
set -xeu

_arch=armv6
_mode=urel

_groupdir=$(dirname -- "$0")

_pkg=${1:-${_groupdir}/minecraftpe.pkg}
_sis=${2:-$(basename -- "${_pkg%.*}.${_arch}.${_mode}.sis")}

_approot=/epoc32/release/${_arch}/${_mode}

if [ ! -d "${EPOCROOT}" ] ; then
	>&2 echo 'EPOCROOT does not point to a directory'
	exit 1
fi

test -f "${EPOCROOT}${_approot}/minecraftpe.exe" || \
	sbs -c "${_arch}_${_mode}" -b "${_groupdir}/bld.inf"

install -Dm755 -t "${EPOCROOT}/epoc32/data/z/sys/bin/" "${EPOCROOT}${_approot}/minecraftpe.exe"
mkdir -p "${EPOCROOT}/epoc32/data/z/private/e000c418"
cp -r "${_groupdir}/../../../data" "${EPOCROOT}/epoc32/data/z/private/e000c418/"

makesis -v -d"${EPOCROOT}/epoc32/data/z" "${_pkg}" "${_sis}"

_crt=${3:-}
_key=${4:-}
_pass=${5:-}

# Also generate self-signed SISX for non-defused environments

if [ -z "${_crt}" ] || [ -z "${_key}" ] ; then
	_key=snakeoil.key
	_crt=snakeoil.crt

	openssl req -x509 \
		-newkey rsa:2048 \
		-keyout "${_key}" \
		-out "${_crt}" \
		-sha1 -nodes -days 999 \
		-subj '/C=XX/ST=Meowland/L=New Meowningsburg/O=Communal Cat Tree/OU=Department of Meow/CN=meow'
fi

if [ -n "${_pass}" ] ; then
	signsis -v "${_sis}" "${_sis}x" "${_crt}" "${_key}" "${_pass}"
else
	signsis -v "${_sis}" "${_sis}x" "${_crt}" "${_key}"
fi

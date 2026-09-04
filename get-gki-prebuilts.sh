#!/bin/bash
#
# SPDX-License-Identifier: GPL-2.0-only
#
# Extracts prebuilts/gki from Google's kernel release tarball into the Kleaf
# workspace. See GKI-PREBUILTS.md.

set -e

SRC="$1"

if [ -z "${SRC}" ] || [ ! -f "${SRC}" ]; then
    echo "usage: $0 <kernel-15938155.tar.xz>" >&2
    exit 1
fi

# this script lives at private/ in the workspace
WORKSPACE="$(cd "$(dirname "$(realpath "${BASH_SOURCE[0]}")")/.." && pwd)"

if [ ! -e "${WORKSPACE}/common/BUILD.bazel" ]; then
    echo "${WORKSPACE} does not look like a synced kernel workspace" >&2
    exit 1
fi

echo "Extracting prebuilts/gki into ${WORKSPACE} ..."
tar -xJf "${SRC}" --strip-components=1 -C "${WORKSPACE}" 'kernel-15938155/prebuilts/gki'

for d in kernel_aarch64 kernel_aarch64_16k kernel_aarch64_fips140; do
    if [ ! -d "${WORKSPACE}/prebuilts/gki/${d}" ]; then
        echo "MISSING: prebuilts/gki/${d}" >&2
        exit 1
    fi
    echo "OK prebuilts/gki/${d}"
done

rel=$(sed -n 's/^kernel_release=//p' "${WORKSPACE}/prebuilts/gki/kernel_aarch64/gki-info.txt" 2>/dev/null)
echo "GKI kernel release: ${rel:-unknown}"

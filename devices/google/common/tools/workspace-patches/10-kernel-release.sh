#!/bin/bash
# Name the kernel release.
#
# Without stamping, kleaf writes the scmversion itself and settles on
# "maybe-dirty" - build/kernel/kleaf/impl/stamp.bzl:
#
#     stable_scmversion_cmd = "echo '-maybe-dirty'"
#
# It puts that in a localversion file as "-$android_release-$KMI_GENERATION"
# plus the scmversion, and the kernel appends CONFIG_LOCALVERSION after it:
#
#     6.12.81  -android16-6-maybe-dirty  -4k
#
# So this is the piece to change, not CONFIG_LOCALVERSION - and not the device
# defconfig fragment either, which configures the module build rather than the
# GKI image the release string belongs to. build/kernel is checked out from
# AOSP, so the edit is made to the workspace on the way past.
set -e
W="$1"
F="$W/build/kernel/kleaf/impl/stamp.bzl"
[ -f "$F" ] || { echo "no stamp.bzl at $F" >&2; exit 1; }

if grep -q "echo '-lineage'" "$F"; then
    echo "kernel release: already named"
    exit 0
fi
grep -q "echo '-maybe-dirty'" "$F" || {
    echo "stamp.bzl does not look the way this expects" >&2; exit 1; }
sed -i "s/echo '-maybe-dirty'/echo '-lineage'/" "$F"
echo "kernel release: -maybe-dirty -> -lineage"

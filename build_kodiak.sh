#!/bin/bash
#
# SPDX-License-Identifier: GPL-2.0-only
#
# Builds the kodiak kernel: applies the workspace patches in
# devices/google/common/tools/workspace-patches/ to the AOSP checkouts
# around private/, then runs Google's spacecraft build. Extra arguments are
# passed through to build_spacecraft.sh.

set -e

# this script lives at private/ in the workspace
PRIVATE="$(cd "$(dirname "$(realpath "${BASH_SOURCE[0]}")")" && pwd)"
WORKSPACE="$(cd "${PRIVATE}/.." && pwd)"

if [ ! -d "${WORKSPACE}/prebuilts/gki/kernel_aarch64" ]; then
    echo "prebuilts/gki is missing; see private/GKI-PREBUILTS.md" >&2
    exit 1
fi

for p in "${PRIVATE}"/devices/google/common/tools/workspace-patches/*.sh; do
    [ -e "${p}" ] || continue
    echo "workspace patch: $(basename "${p}")"
    bash "${p}" "${WORKSPACE}"
done

cd "${WORKSPACE}"
exec ./build_spacecraft.sh "$@"

# GKI prebuilts

Malibu builds its modules against a prebuilt GKI rather than against ACK
from source - `devices/google/malibu/device.bazelrc` sets
`--kernel_package=gki`. Bazel therefore cannot finish analysing the
spacecraft target until `prebuilts/gki/` exists in the workspace, and the
fips140 crypto module is a certified binary that must not be rebuilt at all.

That directory is 1.3G across 54 files, so it is not in this repository.

| | |
| --- | --- |
| Kernel release | `6.12.69-android16-6-g5c5f2fea42dd-ab15835541-4k` |
| GKI build id | 15835541 |
| Contents | `kernel_aarch64`, `kernel_aarch64_16k`, `kernel_aarch64_fips140` |

## Where it comes from

Google's kernel release tarball for CD1A.260714.001.A9:

    kernel-15938155.tar.xz  ->  kernel-15938155/prebuilts/gki/

**Not** from ci.android.com. `download_prebuilts.py` points at the public CI
now, but build 15835541 is not published there - it answers 404 - so the
download path that works for other Pixels does not work here.

## Restoring it

From the workspace root, after `repo sync`:

    ./private/get-gki-prebuilts.sh /path/to/kernel-15938155.tar.xz

Selective extraction still reads through the whole archive, so expect it to
take a while.

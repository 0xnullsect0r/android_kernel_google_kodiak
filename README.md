# Kodiak kernel

Linux 6.12 for the Google Pixel 11 Pro XL (kodiak), for LineageOS 24.0.

Kodiak is a `spacecraft` family device on the `malibu` SoC (Tensor G6), so the
kernel is built through the spacecraft target; the resulting dist contains the
dtbs for all three spacecraft devices.

## Contents

This repository carries the parts of Google's kernel release that Google wrote
for Pixel hardware, and nothing else:

| Path                       | What it is                                  |
| -------------------------- | ------------------------------------------- |
| `devices/google/common/`   | Shared Kleaf rules, workspace glue, tools    |
| `devices/google/malibu/`   | Tensor G6 SoC: defconfig, dts, module lists  |
| `devices/google/spacecraft/` | cubs / grizzly / kodiak dts and build target |
| `google-modules/`          | Pixel kernel modules                        |
| `default.xml`              | Manifest that assembles the build workspace  |

The kernel itself (ACK), the toolchains and the bazel rules are not vendored.
They are fetched from AOSP by the manifest, which keeps this repository to the
Pixel-specific sources.

## Building

```
mkdir kodiak-kernel && cd kodiak-kernel
repo init -u ssh://git@github.com/topsuplove1122/android_kernel_google_kodiak -b lineage-24.0
repo sync -c -j$(nproc)
./build_spacecraft.sh
```

The build output lands in `out/spacecraft/dist/`. To use it in a ROM build,
copy it into `device/google/kodiak/kernels/6.12/`; the device tree's
`build-kernel.sh` does both steps.

Sources are checked out at `private/`, matching the layout Google's release
tarball uses, so upstream paths such as
`//private/devices/google/spacecraft:spacecraft` resolve unchanged.

## Versions

| | |
| --- | --- |
| Base release | `CD1A.260714.001.A9` (kernel-15938155) |
| Kernel | 6.12.77, `android16-6.12` |
| GKI | 6.12.69, `android16-6.12-2026-03` |
| Clang | `clang-r547379` |

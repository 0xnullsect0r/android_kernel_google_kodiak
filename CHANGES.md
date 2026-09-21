# Changes against Google's kernel release

Everything in this repository is Google's kernel release for Pixel except
the commits below. Imports are marked; every other commit is a change made
here, and should be re-checked whenever a new Google release is imported.

## Imports

| Commit | What |
| --- | --- |
| `bf414677` | `kernel-15938155` (CD1A.260714.001.A9), trimmed to common / malibu / spacecraft |
| `748db143`, `2849c638` | `private/` from `kernel-16136351` (CP41) |

## Build and workspace

| Commit | What |
| --- | --- |
| `0cf5d5d2`, `51e7d34f`, `8c2ba27f`, `d5433208` | `default.xml`: Kleaf workspace manifest, vendored nanopb, ACK pin |
| `83e3e045` | `GKI-PREBUILTS.md`, `get-gki-prebuilts.sh` |
| `4e5d8cb2`, `b2f6fb7f`, `c9d52504` | Kernel release string (`-lineage`) via `workspace-patches/10-kernel-release.sh` |
| `9a409629` | `build_kodiak.sh`: applies the workspace patches, then runs `build_spacecraft.sh` |
| `06add9e9` | Manifest syncs `private/` over HTTPS from a public remote |
| `4f186871` | Drops the seventeen non-spacecraft devices the CP41 import brought back |

## Source changes

| Commit | Files | Why |
| --- | --- | --- |
| `4e998649` | `edgetpu/abrolhos` Kbuild | Hardcoded `GIT_REPO_TAG`. **Dead since the CP41 import**, which removed `abrolhos` (a Pixel 6 TPU, unused here) |
| `d0d7364a` | `google-modules/edgetpu` | CP41 dropped the santafe wrapper files but its BUILD still expects them |
| `188b1a27` | bcmdhd / syna wlan drivers | `MAC_ADDR_STR_LEN` collides with the one ACK 6.12.92 added |

## Not carried

Root and root-hiding patches (KernelSU variants, SUSFS) are not part of this
tree and will not be accepted here.

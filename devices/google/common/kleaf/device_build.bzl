# SPDX-License-Identifier: GPL-2.0-only

"""
Define build targets for a device.
"""

load("@bazel_skylib//rules:copy_file.bzl", "copy_file")
load("@bazel_skylib//rules:select_file.bzl", "select_file")
load(
    "@kleaf//build/kernel/kleaf:kernel.bzl",
    "dtb_image",
    "dtbo",
    "initramfs",
    "kernel_abi",
    "kernel_build",
    "kernel_build_config",
    "kernel_compile_commands",
    "kernel_dtstree",
    "kernel_module_group",
    "kernel_modules_install",
    "kernel_sbom",
    "kernel_unstripped_modules_archive",
    "merged_kernel_uapi_headers",
    "system_dlkm_image",
    "vendor_boot_image",
    "vendor_dlkm_image",
)
load("@rules_pkg//pkg:install.bzl", "pkg_install")
load("@rules_pkg//pkg:mappings.bzl", "pkg_files", "strip_prefix")
load("@rules_pkg//pkg:pkg.bzl", "pkg_zip")
load("//private/devices/google/common:constants.bzl", "PIXEL_GKI_MODULES_LIST")
load("//private/devices/google/common/kleaf:bste.bzl", "bste_test_package")
load("//private/devices/google/common/kleaf:copy_files.bzl", "STRIP_PREFIX_FILES_ONLY", "copy_files")
load("//private/devices/google/common/kleaf:create_file.bzl", "create_file")
load("//private/devices/google/common/kleaf:image_props.bzl", "image_props")
load("//private/devices/google/common/kleaf:merged_uapi_headers.bzl", "merged_uapi_headers")

def device_build(
        name,
        base_device = None,
        build_configs = [],
        kconfigs = [],
        kconfig_exts = [],
        defconfig_fragments = [],
        dts_srcs = None,
        dts_makefile = None,
        dtb_outs = [],
        dtbo_outs = [],
        dtbo_additional_outs = [],
        module_outs = [],
        ext_dtbos = [],
        ext_modules = [],
        kunit_modules = [],
        ddk_uapi_headers = [],
        build_dtb = False,
        build_dtbo = False,
        build_vendor_kernel_boot = False,
        build_system_dlkm = False,
        build_vendor_dlkm = False,
        vendor_ramdisk_modules_lists = [],
        system_dlkm_modules_blocklists = [],
        vendor_dlkm_modules_blocklists = [],
        fs_type = "ext4",
        insmod_cfgs = [],
        bste_test_suites = [],
        extra_dist_targets = []):
    """Define build targets for a device.

    Define the following targets and their dependencies for a device:
        {name}/build_config
        {name}/kconfigs
        {name}/kconfig_ext
        {name}/defconfig_fragments
        {name}/kernel
        {name}/dtbs
        {name}/dtbos
        {name}/ext_modules
        {name}/kunit_modules
        {name}/ddk_uapi_headers
        {name}/kernel_images
        {name}/dtb_image
        {name}/dist
        {name}/kernel_abi
        {name}/kernel_compile_commands
        {name}/bste_test_suites
        {name}/bste_test_package

    Args:
        name: Name of the device.
        base_device: The base device of this device.
        build_configs: Build configs for kernel. Combined with the base device.
        kconfigs: Additional Kconfig files. Combined with the base device.
        kconfig_exts: Additional kconfig.ext files. Combined with the base device.
        defconfig_fragments: Defconfig fragments. Combined with the base device.
        dts_srcs: The sources of device tree. Required if dtb_outs or dtbo_outs is set.
        dts_makefile: The Makefile of device tree in dts_srcs. Required if dtb_outs or dtbo_outs is
            set.
        dtb_outs: The list of dtbs building from dts_srcs.
        dtbo_outs: The list of dtbos building from dts_srcs.
        dtbo_additional_outs: The list of dtbos building from dts_srcs but being excluded from
            dtbo.img.
        module_outs: The list of in-tree modules building from kernel sources.
        ext_dtbos: Extra dtbos.
        ext_modules: Out-of-tree modules. Combined with the base device.
        kunit_modules: Kunit modules, packed into kunit_tests.zip. Combined with the base device.
        ddk_uapi_headers: DDK uapi headers which will be merged into kernel-uapi-headers.tar.gz.
            Combined with the base device.
        build_dtb: If True, build dtb.img from dtb_outs.
        build_dtbo: If True, build dtbo.img from dtbo_outs and ext_dtbos.
        build_vendor_kernel_boot: If True, build vendor_kernel_boot.img.
        build_system_dlkm: If True, build system_dlkm.img which contains GKI modules. If False and
            build_vendor_dlkm is True, GKI modules will be in vendor_dlkm.img instead.
        build_vendor_dlkm: If True, build vendor_dlkm.img.
        vendor_ramdisk_modules_lists: Files containing lists of modules which should be in vendor
            ramdisk (initramfs.img, vendor_kernel_boot.img). Combined with the base device.
        system_dlkm_modules_blocklists: Files containing lists of modules which should be blocked
            from being loaded from system_dlkm.img. If build_system_dlkm is False and
            build_vendor_dlkm is True, this will be combined into vendor_dlkm_modules_blocklists.
            Combined with the base device.
        vendor_dlkm_modules_blocklists: Files containing lists of modules which should be blocked
            from being loaded from vendor_dlkm.img. Combined with the base device.
        fs_type: Filesystem for system_dlkm and vendor_dlkm. Support ext4 (default) and erofs.
        insmod_cfgs: Insmod cfg files which will be copied to etc/ in vendor_dlkm.img. Combined with
            the base device.
        bste_test_suites: BSTE test suites which will be packaged into bste_test_package.tar.gz.
            Combined with the base device.
        extra_dist_targets: Extra targets added to dist.
    """

    if base_device:
        base_build_configs = ["{}/build_config".format(base_device)]
        base_kconfigs = ["{}/kconfigs".format(base_device)]
        base_kconfig_exts = ["{}/kconfig_ext".format(base_device)]
        base_defconfig_fragments = ["{}/defconfig_fragments".format(base_device)]
        base_ext_modules = ["{}/ext_modules".format(base_device)]
        base_kunit_modules = ["{}/kunit_modules".format(base_device)]
        base_ddk_uapi_headers = ["{}/ddk_uapi_headers".format(base_device)]
        base_vendor_ramdisk_modules_lists = ["{}/vendor_ramdisk_modules_list".format(base_device)]
        base_system_dlkm_modules_blocklists = ["{}/system_dlkm_modules_blocklist".format(base_device)]
        base_vendor_dlkm_modules_blocklists = ["{}/vendor_dlkm_modules_blocklist".format(base_device)]
        base_insmod_cfgs = ["{}/insmod_cfgs".format(base_device)]
        base_bste_test_suites = ["{}/bste_test_suites".format(base_device)]
    else:
        base_build_configs = ["//private/devices/google/common/kleaf/files:build.config.common"]
        base_kconfigs = []
        base_kconfig_exts = ["//common:kernel_package_kconfig"]
        base_defconfig_fragments = ["//common:kernel_package_defconfig_fragment"]
        base_ext_modules = select({
            "//private/devices/google/common:use_prebuilt_fips140_is_true": [
                "//private/devices/google/common:fips140",
            ],
            "//conditions:default": [],
        })
        base_kunit_modules = []
        base_ddk_uapi_headers = []
        base_vendor_ramdisk_modules_lists = []
        base_system_dlkm_modules_blocklists = []
        base_vendor_dlkm_modules_blocklists = []
        base_insmod_cfgs = []
        base_bste_test_suites = []

    target_build_config = "{}/build_config".format(name)
    target_kconfigs = "{}/kconfigs".format(name)
    target_kconfig_ext = "{}/kconfig_ext".format(name)
    target_defconfig_fragments = "{}/defconfig_fragments".format(name)
    target_kernel_sources = "{}/kernel_sources".format(name)
    target_kmi_symbol_list = "{}/kmi_symbol_list".format(name)
    target_kernel = "{}/kernel".format(name)
    target_dtstree = "{}/dtstree".format(name)
    target_dt = "{}/dt".format(name)
    target_dtbs = "{}/dtbs".format(name)
    target_dtbos = "{}/dtbos".format(name)
    target_additional_dtbos = "{}/additional_dtbos".format(name)
    target_ext_modules = "{}/ext_modules".format(name)
    target_kernel_modules_install = "{}/kernel_modules_install".format(name)
    target_kernel_unstripped_modules_archive = "{}/kernel_unstripped_modules_archive".format(name)
    target_merged_kernel_uapi_headers = "{}/merged_kernel_uapi_headers".format(name)
    target_kunit_modules = "{}/kunit_modules".format(name)
    target_kunit_modules_install = "{}/kunit_modules_install".format(name)
    target_kunit_modules_pkg = "{}/kunit_modules_pkg".format(name)
    target_kunit_tests_zip = "{}/kunit_tests_zip".format(name)
    target_ddk_uapi_headers = "{}/ddk_uapi_headers".format(name)
    target_merged_ddk_uapi_headers = "{}/merged_ddk_uapi_headers".format(name)
    target_cleaned_ddk_uapi_headers = "{}/cleaned_ddk_uapi_headers".format(name)
    target_merged_kernel_and_ddk_uapi_headers = "{}/merged_kernel_and_ddk_uapi_headers".format(name)
    target_vendor_ramdisk_modules_list = "{}/vendor_ramdisk_modules_list".format(name)
    target_system_dlkm_modules_list = "{}/system_dlkm_modules_list".format(name)
    target_system_dlkm_modules_blocklist = "{}/system_dlkm_modules_blocklist".format(name)
    target_system_dlkm_file_contexts = "{}/system_dlkm_file_contexts".format(name)
    target_system_dlkm_props = "{}/system_dlkm_props".format(name)
    target_vendor_dlkm_modules_list = "{}/vendor_dlkm_modules_list".format(name)
    target_vendor_dlkm_modules_blocklist = "{}/vendor_dlkm_modules_blocklist".format(name)
    target_vendor_dlkm_file_contexts = "{}/vendor_dlkm_file_contexts".format(name)
    target_vendor_dlkm_props = "{}/vendor_dlkm_props".format(name)
    target_merged_dlkm_modules_list = "{}/merged_dlkm_modules_list".format(name)
    target_merged_dlkm_modules_blocklist = "{}/merged_dlkm_modules_blocklist".format(name)
    target_insmod_cfgs = "{}/insmod_cfgs".format(name)
    target_bste_test_suites = "{}/bste_test_suites".format(name)
    target_bste_test_package = "{}/bste_test_package".format(name)
    target_initramfs = "{}/initramfs".format(name)
    target_boot_image_selection = "{}/boot_image_selection".format(name)
    target_boot_image = "{}/boot_image".format(name)
    target_dtb_image = "{}/dtb_image".format(name)
    target_dtbo_image = "{}/dtbo_image".format(name)
    target_vendor_kernel_boot_image = "{}/vendor_kernel_boot_image".format(name)
    target_system_dlkm_image_types = "{}/system_dlkm_image_types".format(name)
    target_system_dlkm_image = "{}/system_dlkm_image".format(name)
    target_vendor_dlkm_image = "{}/vendor_dlkm_image".format(name)
    target_dist_files = "{}/dist_files".format(name)
    target_dist = "{}/dist".format(name)
    target_kernel_abi = "{}/kernel_abi".format(name)
    target_kernel_compile_commands = "{}/kernel_compile_commands".format(name)

    kernel_build_config(
        name = target_build_config,
        srcs = base_build_configs + build_configs,
    )

    native.filegroup(
        name = target_kconfigs,
        srcs = base_kconfigs + kconfigs,
    )

    create_file(
        name = target_kconfig_ext,
        srcs = base_kconfig_exts + kconfig_exts,
        out = "{}/Kconfig.ext".format(name),
    )

    native.filegroup(
        name = target_defconfig_fragments,
        srcs = base_defconfig_fragments + defconfig_fragments,
    )

    native.filegroup(
        name = target_kernel_sources,
        srcs = [
            "//common:kernel_aarch64_sources",
            target_kconfigs,
        ],
        visibility = ["//visibility:private"],
    )

    select_file(
        name = target_kmi_symbol_list,
        srcs = "//common:aarch64_additional_kmi_symbol_lists",
        subpath = "gki/aarch64/symbols/pixel",
    )

    kernel_build_kwargs = {
        "srcs": [target_kernel_sources],
        "base_kernel": "//common:kernel_aarch64",
        "build_config": target_build_config,
        "kcflags": [
            "-D__ANDROID_COMMON_KERNEL__",
        ] + select({
            "//private/devices/google/common/kleaf:is_base_kernel_ignored": [],
            "//conditions:default": [
                "-Wmissing-declarations",
                "-Wmissing-prototypes",
            ],
        }),
        "kconfig_ext": target_kconfig_ext,
        "makefile": "//common:Makefile",
        "post_defconfig_fragments": [target_defconfig_fragments],
    }

    kernel_build(
        name = target_kernel,
        outs = [".config"],
        collect_unstripped_modules = True,
        kmi_symbol_list = target_kmi_symbol_list,
        make_goals = ["modules"],
        module_outs = module_outs,
        strip_modules = True,
        visibility = ["//visibility:private"],
        **kernel_build_kwargs
    )

    need_dt = True if dtb_outs or dtbo_outs or dtbo_additional_outs else False

    if need_dt:
        if dts_srcs == None:
            fail("dts_srcs is required for building dtb/dtbos")

        if dts_makefile == None:
            fail("dts_makefile is required for building dtb/dtbos")

        kernel_dtstree(
            name = target_dtstree,
            srcs = dts_srcs,
            makefile = dts_makefile,
            visibility = ["//visibility:private"],
        )

        kernel_build(
            name = target_dt,
            outs = dtb_outs + dtbo_outs + dtbo_additional_outs,
            dtstree = target_dtstree,
            make_goals = ["dtbs"],
            visibility = ["//visibility:private"],
            **kernel_build_kwargs
        )

    native.filegroup(
        name = target_dtbs,
        srcs = ["{}/{}".format(target_dt, dtb) for dtb in dtb_outs],
    )

    native.filegroup(
        name = target_dtbos,
        srcs = ["{}/{}".format(target_dt, dtbo) for dtbo in dtbo_outs] + ext_dtbos,
    )

    native.filegroup(
        name = target_additional_dtbos,
        srcs = ["{}/{}".format(target_dt, dtbo) for dtbo in dtbo_additional_outs],
    )

    kernel_module_group(
        name = target_ext_modules,
        srcs = base_ext_modules + ext_modules,
    )

    kernel_modules_install(
        name = target_kernel_modules_install,
        kernel_build = target_kernel,
        kernel_modules = [target_ext_modules],
        visibility = ["//visibility:private"],
    )

    kernel_unstripped_modules_archive(
        name = target_kernel_unstripped_modules_archive,
        kernel_build = target_kernel,
        kernel_modules = [target_ext_modules],
        visibility = ["//visibility:private"],
    )

    merged_kernel_uapi_headers(
        name = target_merged_kernel_uapi_headers,
        kernel_build = target_kernel,
        kernel_modules = [target_ext_modules],
        visibility = ["//visibility:private"],
    )

    kernel_module_group(
        name = target_kunit_modules,
        srcs = base_kunit_modules + kunit_modules,
    )

    kernel_modules_install(
        name = target_kunit_modules_install,
        kernel_build = target_kernel,
        kernel_modules = [target_kunit_modules],
        visibility = ["//visibility:private"],
    )

    pkg_files(
        name = target_kunit_modules_pkg,
        srcs = [
            "//common:kernel_aarch64/lib/kunit/kunit.ko",
            target_kunit_modules_install,
        ],
        prefix = "kunit_test_modules",
        visibility = ["//visibility:private"],
    )

    pkg_zip(
        name = target_kunit_tests_zip,
        srcs = [
            "//private/devices/google/common/testing:kunit_test_scripts",
            target_kunit_modules_pkg,
        ],
        out = "{}/kunit_tests.zip".format(name),
        visibility = ["//visibility:private"],
    )

    native.filegroup(
        name = target_ddk_uapi_headers,
        srcs = base_ddk_uapi_headers + ddk_uapi_headers,
    )

    merged_uapi_headers(
        name = target_merged_ddk_uapi_headers,
        out = "{}/ddk-uapi-headers.tar.gz".format(name),
        uapi_headers = [target_ddk_uapi_headers],
        visibility = ["//visibility:private"],
    )

    merged_uapi_headers(
        name = target_cleaned_ddk_uapi_headers,
        out = "{}/cleaned-ddk-uapi-headers.tar.gz".format(name),
        clean = True,
        uapi_headers = [target_ddk_uapi_headers],
        visibility = ["//visibility:private"],
    )

    merged_uapi_headers(
        name = target_merged_kernel_and_ddk_uapi_headers,
        uapi_headers = [
            target_merged_kernel_uapi_headers,
            target_ddk_uapi_headers,
        ],
        visibility = ["//visibility:private"],
    )

    create_file(
        name = target_vendor_ramdisk_modules_list,
        srcs = base_vendor_ramdisk_modules_lists + vendor_ramdisk_modules_lists,
        out = "{}/vendor_ramdisk.modules".format(name),
    )

    create_file(
        name = target_system_dlkm_modules_list,
        out = "{}/system_dlkm.modules".format(name),
        # The list is used to filter modules with `grep -w`.
        contents = ["^kernel/" + m for m in PIXEL_GKI_MODULES_LIST],
        visibility = ["//visibility:private"],
    )

    create_file(
        name = target_system_dlkm_modules_blocklist,
        srcs = base_system_dlkm_modules_blocklists + system_dlkm_modules_blocklists,
        out = "{}/system_dlkm.blocklist".format(name),
    )

    create_file(
        name = target_system_dlkm_file_contexts,
        srcs = ["//private/devices/google/common:sepolicy/system_dlkm_file_contexts"],
        out = "{}/sepolicy/system_dlkm_file_contexts".format(name),
        visibility = ["//visibility:private"],
    )

    image_props(
        name = target_system_dlkm_props,
        out = "{}/system_dlkm.props".format(name),
        fs_type = fs_type,
        mount_point = "system_dlkm",
        selinux_fc = target_system_dlkm_file_contexts,
        visibility = ["//visibility:private"],
    )

    create_file(
        name = target_vendor_dlkm_modules_list,
        out = "{}/vendor_dlkm.modules".format(name),
        # The list is used to filter modules with `grep -w`.
        contents = ["^kernel/" + m for m in module_outs] + ["^extra/.*"],
        visibility = ["//visibility:private"],
    )

    create_file(
        name = target_vendor_dlkm_modules_blocklist,
        srcs = base_vendor_dlkm_modules_blocklists + vendor_dlkm_modules_blocklists,
        out = "{}/vendor_dlkm.blocklist".format(name),
    )

    create_file(
        name = target_vendor_dlkm_file_contexts,
        srcs = ["//private/devices/google/common:sepolicy/vendor_dlkm_file_contexts"],
        out = "{}/sepolicy/vendor_dlkm_file_contexts".format(name),
        visibility = ["//visibility:private"],
    )

    image_props(
        name = target_vendor_dlkm_props,
        out = "{}/vendor_dlkm.props".format(name),
        fs_type = fs_type,
        mount_point = "vendor_dlkm",
        selinux_fc = target_vendor_dlkm_file_contexts,
        visibility = ["//visibility:private"],
    )

    create_file(
        name = target_merged_dlkm_modules_list,
        srcs = [
            target_system_dlkm_modules_list,
            target_vendor_dlkm_modules_list,
        ],
        out = "{}/merged_dlkm.modules".format(name),
        visibility = ["//visibility:private"],
    )

    create_file(
        name = target_merged_dlkm_modules_blocklist,
        srcs = [
            target_system_dlkm_modules_blocklist,
            target_vendor_dlkm_modules_blocklist,
        ],
        out = "{}/merged_dlkm.blocklist".format(name),
        visibility = ["//visibility:private"],
    )

    native.filegroup(
        name = target_insmod_cfgs,
        srcs = base_insmod_cfgs + insmod_cfgs,
    )

    native.filegroup(
        name = target_bste_test_suites,
        srcs = base_bste_test_suites + bste_test_suites,
    )

    bste_test_package(
        name = target_bste_test_package,
        test_suites = [target_bste_test_suites],
    )

    initramfs(
        name = target_initramfs,
        kernel_modules_install = target_kernel_modules_install,
        modules_list = target_vendor_ramdisk_modules_list,
        ramdisk_compression = "lz4",
        trim_unused_modules = True,
        vendor_boot_name = "vendor_kernel_boot" if build_vendor_kernel_boot else None,
        visibility = ["//visibility:private"],
    )

    select_file(
        name = target_boot_image_selection,
        srcs = "//common:kernel_aarch64_gki_artifacts",
        subpath = "boot-lz4.img",
        visibility = ["//visibility:private"],
    )

    copy_file(
        name = target_boot_image,
        src = target_boot_image_selection,
        out = "{}/boot.img".format(name),
        visibility = ["//visibility:private"],
    )

    dist_targets = [
        target_boot_image,
        target_dtbs,
        target_dtbos,
        target_additional_dtbos,
        target_initramfs,
        target_insmod_cfgs,
        target_kernel,
        target_kernel_modules_install,
        target_kernel_unstripped_modules_archive,
        target_kmi_symbol_list,
        target_kunit_modules_install,
        target_kunit_tests_zip,
        target_merged_ddk_uapi_headers,
        target_cleaned_ddk_uapi_headers,
        target_merged_kernel_and_ddk_uapi_headers,
        target_bste_test_package,
        "@kleaf//build/kernel:gki_certification_tools",
        "//common:kernel_aarch64",
        "//common:kernel_aarch64_headers",
        "//private/devices/google/common:kernel_gki_modules",
    ] + extra_dist_targets

    dtb_image(
        name = target_dtb_image,
        srcs = [target_dtbs],
        out = "{}/dtb.img".format(name),
        visibility = ["//visibility:private"],
    )

    if build_dtb:
        dist_targets.append(target_dtb_image)

    if build_dtbo:
        dtbo(
            name = target_dtbo_image,
            srcs = [target_dtbos],
            kernel_build = target_kernel,
            visibility = ["//visibility:private"],
        )
        dist_targets.append(target_dtbo_image)

    if build_vendor_kernel_boot:
        vendor_boot_image(
            name = target_vendor_kernel_boot_image,
            outs = ["vendor_kernel_boot.img"],
            dtb_image = target_dtb_image,
            header_version = 4,
            initramfs = target_initramfs,
            kernel_build = target_kernel,
            ramdisk_compression = "lz4",
            unpack_ramdisk = True,
            vendor_boot_name = "vendor_kernel_boot",
            visibility = ["//visibility:private"],
        )
        dist_targets.append(target_vendor_kernel_boot_image)

    if build_system_dlkm:
        system_dlkm_image(
            name = target_system_dlkm_image_types,
            base = "//common:kernel_aarch64_system_dlkm_image",
            fs_types = [fs_type],
            kernel_modules_install = target_kernel_modules_install,
            modules_blocklist = target_system_dlkm_modules_blocklist,
            modules_list = target_system_dlkm_modules_list,
            props = target_system_dlkm_props,
            visibility = ["//visibility:private"],
            deps = [target_system_dlkm_file_contexts],
        )
        copy_files(
            name = target_system_dlkm_image,
            srcs = [target_system_dlkm_image_types],
            prefix = target_system_dlkm_image,
            renames = {
                "system_dlkm.{}.img".format(fs_type): "system_dlkm.img",
            },
            strip_prefix = STRIP_PREFIX_FILES_ONLY,
        )
        dist_targets += [
            target_system_dlkm_image,
            target_system_dlkm_file_contexts,
            target_system_dlkm_props,
        ]

    if build_vendor_dlkm:
        vendor_dlkm_image(
            name = target_vendor_dlkm_image,
            archive = True,
            etc_files = [target_insmod_cfgs],
            kernel_modules_install = target_kernel_modules_install,
            modules_blocklist = target_vendor_dlkm_modules_blocklist if build_system_dlkm else target_merged_dlkm_modules_blocklist,
            modules_list = target_vendor_dlkm_modules_list if build_system_dlkm else target_merged_dlkm_modules_list,
            props = target_vendor_dlkm_props,
            vendor_boot_modules_load = target_initramfs,
            visibility = ["//visibility:private"],
            deps = [target_vendor_dlkm_file_contexts],
        )
        dist_targets += [
            target_vendor_dlkm_image,
            target_vendor_dlkm_file_contexts,
            target_vendor_dlkm_props,
        ]

    sbom_file_generator_target = "{}/kernel_sbom".format(name)

    kernel_sbom(
        name = sbom_file_generator_target,
        kernel_build = target_kernel,
        srcs = dist_targets,
        out = "kernel_sbom.spdx.json",
    )
    dist_targets.append(sbom_file_generator_target)

    pkg_files(
        name = target_dist_files,
        srcs = dist_targets,
        strip_prefix = strip_prefix.files_only(),
        visibility = ["//visibility:private"],
    )

    pkg_install(
        name = target_dist,
        srcs = [target_dist_files],
        destdir = "out/{}/dist".format(name),
        visibility = ["//visibility:private"],
    )

    kernel_abi(
        name = target_kernel_abi,
        kernel_build = target_kernel,
        kernel_modules = [target_ext_modules],
        kmi_symbol_list_add_only = True,
        module_grouping = False,
        visibility = ["//visibility:private"],
    )

    kernel_compile_commands(
        name = target_kernel_compile_commands,
        deps = [
            target_kernel,
            target_ext_modules,
        ],
        visibility = ["//visibility:private"],
    )

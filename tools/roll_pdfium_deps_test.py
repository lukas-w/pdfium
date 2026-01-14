#!/usr/bin/env python3
# Copyright 2026 The PDFium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

from roll_pdfium_deps import roll_all_deps
from roll_pdfium_deps import roll_dep

CHROMIUM_DEPS = '''
# This file is used to manage the dependencies of the Chromium src repo. It is
# used by gclient to determine what version of each dependency to check out, and
# where.

# We expect all git dependencies specified in this file to be in sync with git
# submodules (gitlinks).
git_dependencies = 'SYNC'

gclient_gn_args_file = 'src/build/config/gclient_args.gni'
gclient_gn_args = [
  'build_with_chromium',
  'checkout_android',
  'checkout_android_prebuilts_build_tools',
  'checkout_clang_coverage_tools',
  'checkout_copybara',
  'checkout_glic_e2e_tests',
  'checkout_ios_webkit',
  'checkout_mutter',
  'checkout_openxr',
  'checkout_src_internal',
  'checkout_src_internal_infra',
  'checkout_clusterfuzz_data',
  'cros_boards',
  'cros_boards_with_qemu_images',
  'generate_location_tags',
]


vars = {
  # Variable that can be used to support multiple build scenarios, like having
  # Chromium specific targets in a client project's GN file or sync dependencies
  # conditionally etc.
  'build_with_chromium': True,

  # By default, we should check out everything needed to run on the main
  # chromium waterfalls. This var can be also be set to "small", in order
  # to skip things are not strictly needed to build chromium for development
  # purposes, by adding the following line to src.git's .gclient entry:
  #      "custom_vars": { "checkout_configuration": "small" },
  'checkout_configuration': 'default',

  # By default, don't check out android. Will be overridden by gclient
  # variables.
  # TODO(crbug.com/875037): Remove this once the problem in gclient is fixed.
  'checkout_android': False,

  # By default, don't check out Fuchsia. Will be overridden by gclient
  # variables.
  # TODO(crbug.com/875037): Remove this once the problem in gclient is fixed.
  'checkout_fuchsia': False,

  # For code related to internal Fuchsia images.
  'checkout_fuchsia_internal': False,

  # Fetches the internal Fuchsia SDK boot images, with the images in a
  # comma-separated list.
  'checkout_fuchsia_internal_images': '',

  # Used for downloading the Fuchsia SDK without running hooks.
  'checkout_fuchsia_no_hooks': False,

  # Pull in Android prebuilts build tools so we can create Java xrefs
  'checkout_android_prebuilts_build_tools': False,

  # By default, do not check out Cast3P.
  'checkout_cast3p': False,

  # By default, do not check out Chromium autofill captured sites test
  # dependencies. These dependencies include very large numbers of very
  # large web capture files. Captured sites test dependencies are also
  # restricted to Googlers only.
  'checkout_chromium_autofill_test_dependencies': False,

  # By default, do not check out Chromium password manager captured sites test
  # dependencies. These dependencies include very large numbers of very
  # large web capture files. Captured sites test dependencies are also
  # restricted to Googlers only.
  'checkout_chromium_password_manager_test_dependencies': False,

  # Checkout fuzz archive. Should not need in builders.
  'checkout_clusterfuzz_data': False,

  # By default, checkout JavaScript coverage node modules. These packages
  # are used to post-process raw v8 coverage reports into IstanbulJS compliant
  # output.
  'checkout_js_coverage_modules': True,

  # Checkout out mutter and its dependencies to be able to run tests like
  # interactive_ui_tests on the linux/wayland compositor.
  'checkout_mutter': False,

  # By default, do not check out src-internal. This can be overridden e.g. with
  # custom_vars.
  'checkout_src_internal': False,

  # By default, do not check out //src/internal. This can be overridden e.g. with
  # custom_vars. This acts the same way as checkout_src_internal, but only affects
  # the internal infra folder, instead of all internal repos. It is used by
  # Cronet internal gn2bp to make sure no internal source code is uploaded.
  # See https://crbug.com/404202679: do not modify the set of directories this
  # acts upon.
  'checkout_src_internal_infra' : False,

  # Checkout legacy src_internal. This variable is ignored if
  # checkout_src_internal is set as false.
  'checkout_legacy_src_internal': True,


  # Checkout test code and archives for glic E2E tests.
  'checkout_glic_e2e_tests': False,

  # For super-internal deps. Set by the official builders.
  'checkout_google_internal': False,

  # Fetch the prebuilt binaries for llvm-cov and llvm-profdata. Needed to
  # process the raw profiles produced by instrumented targets (built with
  # the gn arg 'use_clang_coverage').
  'checkout_clang_coverage_tools': False,

  # Fetch the pgo profiles to optimize official builds.
  'checkout_pgo_profiles': False,

  # Fetch clang-tidy into the same bin/ directory as our clang binary.
  'checkout_clang_tidy': False,

  # Fetch clangd into the same bin/ directory as our clang binary.
  'checkout_clangd': False,

  # By default checkout the OpenXR loader library only on Windows and Android.
  # The OpenXR backend for VR in Chromium is currently only supported for these
  # platforms, but support for other platforms may be added in the future.
  'checkout_openxr' : 'checkout_win or checkout_android',

  'checkout_instrumented_libraries': 'checkout_linux and checkout_configuration != "small"',

  # By default bot checkouts the WPR archive files only when this
  # flag is set True.
  'checkout_wpr_archives': False,

  # By default, do not check out WebKit for iOS, as it is not needed unless
  # running against ToT WebKit rather than system WebKit. This can be overridden
  # e.g. with custom_vars.
  'checkout_ios_webkit': False,

  # Default to the empty board. Desktop Chrome OS builds don't need cros SDK
  # dependencies. Other Chrome OS builds should always define this explicitly.
  'cros_boards': Str(''),
  'cros_boards_with_qemu_images': Str(''),
  # Building for CrOS is only supported on linux currently.
  'checkout_simplechrome': '"{cros_boards}" != ""',
  'checkout_simplechrome_with_vms': '"{cros_boards_with_qemu_images}" != ""',

  # Generate location tag metadata to include in tests result data uploaded
  # to ResultDB. This isn't needed on some configs and the tool that generates
  # the data may not run on them, so we make it possible for this to be
  # turned off. Note that you also generate the metadata but not include it
  # via a GN build arg (tests_have_location_tags).
  'generate_location_tags': True,

  # By default, do not check out Copybara 3pp dependency that is specifically
  # needed by Cronet gn2bp CI builder.
  'checkout_copybara': False,

  # luci-go CIPD package version.
  # Make sure the revision is uploaded by infra-packagers builder.
  # https://ci.chromium.org/p/infra-internal/g/infra-packagers/console
  'luci_go': 'git_revision:808a00437f24bb404c09608ad8bf3847a78de369',

  # This can be overridden, e.g. with custom_vars, to build clang from HEAD
  # instead of downloading the prebuilt pinned revision.
  'llvm_force_head_revision': False,

  # This can be overridden, e.g. with custom_vars, to build rust from HEAD
  # against ToT LLVM, instead of downloading the prebuilt pinned revision.
  'rust_force_head_revision': False,

  # Fetch configuration files required for the 'use_remoteexec' gn arg
  'download_remoteexec_cfg': False,
  # RBE instance to use for running remote builds
  # Ignored if reapi_instance is configured for non-RBE address.
  'rbe_instance': Str('projects/rbe-chrome-untrusted/instances/default_instance'),
  # REAPI instance for non-RBE backends.
  # need to set reapi_address too.
  'reapi_instance': Str(''),
  # REAPI address for REAPI backends.
  'reapi_address': Str(''),
  # REAPI backend config path for Siso.
  # pathname relative to build/config/siso/backend_config, or absolute path.
  'reapi_backend_config_path': Str(''),
  # siso CIPD package version.
  'siso_version': 'git_revision:49dcca5d2be985d8ac6d512e59ee59e315264fb8',

  # reclient options.
  # download reclient binaries, required for 'use_reclient` gn arg.
  # TODO(crbug.com/448517720): make it false by default.
  'download_reclient': 'checkout_chromeos',
  # RBE project to download rewrapper config files for. Only needed if
  # different from the project used in 'rbe_instance'
  'rewrapper_cfg_project': Str(''),
  # reclient CIPD package
  'reclient_package': 'infra/rbe/client/',
  # reclient CIPD package version
  'reclient_version': 're_client_version:0.185.0.db415f21-gomaip',

  # download libaom test data
  'download_libaom_testdata': False,

  # download libvpx test data
  'download_libvpx_testdata': False,

  'android_git': 'https://android.googlesource.com',
  'chrome_git': 'https://chrome-internal.googlesource.com',
  'chromium_git': 'https://chromium.googlesource.com',
  'dawn_git': 'https://dawn.googlesource.com',
  'pdfium_git': 'https://pdfium.googlesource.com',
  'quiche_git': 'https://quiche.googlesource.com',
  'skia_git': 'https://skia.googlesource.com',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling V8
  # and whatever else without interference from each other.
  'src_internal_revision': '633507c59d3426468b548d06d4c63977f4663dce',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling Skia
  # and whatever else without interference from each other.
  'skia_revision': '83b0b9e6065a60617504aa1d6e6997c511c3afac',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling V8
  # and whatever else without interference from each other.
  'v8_revision': '5bd40de12569247beb113659504906aa41e48959',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling PDFium
  # and whatever else without interference from each other.
  'pdfium_revision': 'e3c8ca8285b232beac2bbd85decc3f792fd74524',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling googletest
  # and whatever else without interference from each other.
  'googletest_revision': '4fe3307fb2d9f86d19777c7eb0e4809e9694dde7',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling lss
  # and whatever else without interference from each other.
  'lss_revision': '29164a80da4d41134950d76d55199ea33fbb9613',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling breakpad
  # and whatever else without interference from each other.
  'breakpad_revision': 'd0b41ca2a38c7b14c4b7853254eb5bf3b4039691',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling freetype
  # and whatever else without interference from each other.
  'freetype_revision': '30e45abe939d7c2cbdf268f277c293400096868c',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling HarfBuzz
  # and whatever else without interference from each other.
  'harfbuzz_revision': '31695252eb6ed25096893aec7f848889dad874bc',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling catapult
  # and whatever else without interference from each other.
  'catapult_revision': 'e617119e2c9cadf4c4147597112fd40b2b2fa522',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling CrossBench
  # and whatever else without interference from each other.
  'crossbench_revision': 'c0cf68ef7183fe9e2c4e096e8df1ab88a57f674b',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling CrossBench
  # and whatever else without interference from each other.
  'crossbench_web_tests_revision': '3c76c8201f0732fe9781742229ab8ac43bf90cbf',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling libFuzzer
  # and whatever else without interference from each other.
  'libfuzzer_revision': 'bea408a6e01f0f7e6c82a43121fe3af4506c932e',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling fuzztest
  # and whatever else without interference from each other.
  'fuzztest_revision': '893f793594a6bff669f33faf863c7041971b86fd',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling domato
  # and whatever else without interference from each other.
  'domato_revision': '053714bccbda79cf76dac3fee48ab2b27f21925e',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling devtools-frontend
  # and whatever else without interference from each other.
  'devtools_frontend_revision': '86276b6159e1eba2114c1157be7e4e523127d97f',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling libprotobuf-mutator
  # and whatever else without interference from each other.
  'libprotobuf-mutator': '7bf98f78a30b067e22420ff699348f084f802e12',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling feed
  # and whatever else without interference from each other.
  'quiche_revision': '901d0a7b4dbb141f2ad4eb8e1f00eb87f945044e',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling ink
  # and whatever else without interference from each other.
  'ink_revision': '11ca89062782d7e5a57741a303a925f510b91015',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling ink_stroke_modeler
  # and whatever else without interference from each other.
  'ink_stroke_modeler_revision': '2cd45e8683025c28fa2efcf672ad46607e8af869',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling crabbyavif
  # and whatever else without interference from each other.
  'crabbyavif_revision': '640d2758f8d2e59d1a55ae0933673f0f65de68c4',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling feed
  # and whatever else without interference from each other.
  'result_adapter_revision': 'git_revision:5fb3ca203842fd691cab615453f8e5a14302a1d8',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling feed
  # and whatever else without interference from each other.
  'libcxxabi_revision':    '83a852080747b9a362e8f9e361366b7a601f302c',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling feed
  # and whatever else without interference from each other.
  'libunwind_revision':    '88fc07ed143a5b3bbf45d430b72a4617ee9e235f',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling feed
  # and whatever else without interference from each other.
  'clang_format_revision':    'c2725e0622e1a86d55f14514f2177a39efea4a0e',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling feed
  # and whatever else without interference from each other.
  'highway_revision': '84379d1c73de9681b54fbe1c035a23c7bd5d272d',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling ffmpeg
  # and whatever else without interference from each other.
  'ffmpeg_revision': '8d855ef50e30da5c3660ddcde4df37bf31f5cdb3',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling llvm-libc
  # and whatever else without interference from each other.
  'llvm_libc_revision':    '03f822d2a88c8f68f6a92c5cb3e79ccc3002e8a9',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling llvm-libc
  # and whatever else without interference from each other.
  'compiler_rt_revision': 'f21b1c4878ca57b74bf65d26c114c11cf8f41cb4',

  # If you change this, also update the libc++ revision in
  # //buildtools/deps_revisions.gni.
  'libcxx_revision':       'c5dd8ade977af3a7441bbf99a2dcac2d5820e702',

  # GN CIPD package version.
  'gn_version': 'git_revision:5964f499767097d81dbe034e8b541c3988168073',

  # ninja CIPD package.
  'ninja_package': 'infra/3pp/tools/ninja/',

  # ninja CIPD package version.
  # https://chrome-infra-packages.appspot.com/p/infra/3pp/tools/ninja
  'ninja_version': 'version:3@1.12.1.chromium.4',

  # 'magic' variable to tell depot_tools that git submodules should be accepted
  # but parity with DEPS file is expected.
  'SUBMODULE_MIGRATION': 'True',

  # condition to allowlist deps to be synced in Cider. Allowlisting is needed
  # because not all deps are compatible with Cider. Once we migrate everything
  # to be compatible we can get rid of this allowlisting mecahnism and remove
  # this condition. Tracking bug for removing this condition: b/349365433
  'non_git_source': 'True',
}

# Only these hosts are allowed for dependencies in this DEPS file.
# If you need to add a new host, contact chrome infrastracture team.
allowed_hosts = [
  'android.googlesource.com',
  'aomedia.googlesource.com',
  'boringssl.googlesource.com',
  'chrome-infra-packages.appspot.com',
  'chrome-internal.googlesource.com',
  'chromium.googlesource.com',
  'dawn.googlesource.com',
  'pdfium.googlesource.com',
  'quiche.googlesource.com',
  'skia.googlesource.com',
  'swiftshader.googlesource.com',
  'webrtc.googlesource.com',

   # TODO(337061377): Move into a separate allowed gcs bucket list.
  'chromium-ads-detection',
  'chromium-browser-clang',
  'chromium-clang-format',
  'chromium-doclava',
  'chromium-nodejs',
  'chrome-linux-sysroot',
  'chromium-fonts',
  'chromium-style-perftest',
  'chromium-telemetry',
  'chromium-webrtc-resources',
  'meet-bundles',
  'perfetto',
]

deps = {
  # NPM dependencies for JavaScript code coverage.
  'src/third_party/js_code_coverage/node_modules': {
    'dep_type': 'gcs',
    'bucket': 'chromium-nodejs',
    'objects': [
      {
        'object_name': 'js_code_coverage/e932c86d2d4f250416970dc270002a9cb6acecbec034998cdadf9a394d0f1abc',
        'sha256sum': 'e932c86d2d4f250416970dc270002a9cb6acecbec034998cdadf9a394d0f1abc',
        'size_bytes': 1557200,
        'generation': 1742338539536352,
      }
    ]
  },
  'src/build/linux/debian_bullseye_amd64-sysroot': {
    'bucket': 'chrome-linux-sysroot',
    'condition': 'checkout_linux and checkout_x64 and non_git_source',
    'dep_type': 'gcs',
    'objects': [
      {
        'generation': 1741221486381719,
        'object_name': '36a164623d03f525e3dfb783a5e9b8a00e98e1ddd2b5cff4e449bd016dd27e50',
        'sha256sum': '36a164623d03f525e3dfb783a5e9b8a00e98e1ddd2b5cff4e449bd016dd27e50',
        'size_bytes': 20781612,
      },
    ],
  },
  'src/build/linux/debian_bullseye_arm64-sysroot': {
    'bucket': 'chrome-linux-sysroot',
    'condition': 'checkout_linux and checkout_arm64 and non_git_source',
    'dep_type': 'gcs',
    'objects': [
      {
        'generation': 1741221484487736,
        'object_name': '2f915d821eec27515c0c6d21b69898e23762908d8d7ccc1aa2a8f5f25e8b7e18',
        'sha256sum': '2f915d821eec27515c0c6d21b69898e23762908d8d7ccc1aa2a8f5f25e8b7e18',
        'size_bytes': 19204088,
      },
    ],
  },
  'src/build/linux/debian_bullseye_armhf-sysroot': {
    'bucket': 'chrome-linux-sysroot',
    'condition': 'checkout_linux and checkout_arm and non_git_source',
    'dep_type': 'gcs',
    'objects': [
      {
        'generation': 1741221481689337,
        'object_name': '47b3a0b161ca011b2b33d4fc1ef6ef269b8208a0b7e4c900700c345acdfd1814',
        'sha256sum': '47b3a0b161ca011b2b33d4fc1ef6ef269b8208a0b7e4c900700c345acdfd1814',
        'size_bytes': 19054416,
      },
    ],
  },
  'src/build/linux/debian_bullseye_i386-sysroot': {
    'bucket': 'chrome-linux-sysroot',
    'condition': 'checkout_linux and (checkout_x86 or checkout_x64) and non_git_source',
    'dep_type': 'gcs',
    'objects': [
      {
        'generation': 1741221485445080,
        'object_name': '63f0e5128b84f7b0421956a4a40affa472be8da0e58caf27e9acbc84072daee7',
        'sha256sum': '63f0e5128b84f7b0421956a4a40affa472be8da0e58caf27e9acbc84072daee7',
        'size_bytes': 20786772,
      },
    ],
  },
  'src/build/linux/debian_bullseye_mips64el-sysroot': {
    'bucket': 'chrome-linux-sysroot',
    'condition': 'checkout_linux and checkout_mips64 and non_git_source',
    'dep_type': 'gcs',
    'objects': [
      {
        'generation': 1741221481819702,
        'object_name': '58f8594905bfe0fa0b7c7a7e882f01725455d07b7161e6539de5169867009b9f',
        'sha256sum': '58f8594905bfe0fa0b7c7a7e882f01725455d07b7161e6539de5169867009b9f',
        'size_bytes': 19896004,
      },
    ],
  },
  'src/build/linux/debian_bullseye_mipsel-sysroot': {
    'bucket': 'chrome-linux-sysroot',
    'condition': 'checkout_linux and checkout_mips and non_git_source',
    'dep_type': 'gcs',
    'objects': [
      {
        'generation': 1741221481662026,
        'object_name': '2098b42d9698f5c8a15683abbf6d424b7f56200bd2488198e15f31554acb391f',
        'sha256sum': '2098b42d9698f5c8a15683abbf6d424b7f56200bd2488198e15f31554acb391f',
        'size_bytes': 19690120,
      },
    ],
  },
  'src/build/linux/debian_bullseye_ppc64el-sysroot': {
    'bucket': 'chrome-linux-sysroot',
    'condition': 'checkout_linux and checkout_ppc and non_git_source',
    'dep_type': 'gcs',
    'objects': [
      {
        'generation': 1741221484110989,
        'object_name': '485f85dde52830594f7b58ad53b9ca8ff6088b397cacb52aff682be5ffd6f198',
        'sha256sum': '485f85dde52830594f7b58ad53b9ca8ff6088b397cacb52aff682be5ffd6f198',
        'size_bytes': 19637392,
      },
    ],
  },
  'src/build/linux/debian_trixie_riscv64-sysroot': {
    'bucket': 'chrome-linux-sysroot',
    'condition': 'checkout_linux and checkout_riscv64 and non_git_source',
    'dep_type': 'gcs',
    'objects': [
      {
        'generation': 1749572829637587,
        'object_name': '5c8ef4067f41a625d81113a6292180acf4ef49a2ffe015c2779123c133b8e250',
        'sha256sum': '5c8ef4067f41a625d81113a6292180acf4ef49a2ffe015c2779123c133b8e250',
        'size_bytes': 20178952,
      },
    ],
  },
  'src/buildtools/win-format': {
    'bucket': 'chromium-clang-format',
    'condition': 'host_os == "win" and non_git_source',
    'dep_type': 'gcs',
    'objects': [
      {
        'object_name': '565cab9c66d61360c27c7d4df5defe1a78ab56d3',
        'sha256sum': '5557943a174e3b67cdc389c10b0ceea2195f318c5c665dd77a427ed01a094557',
        'size_bytes': 3784704,
        'generation': 1738622386314064,
        'output_file': 'clang-format.exe',
      },
    ],
  },
  'src/buildtools/mac-format': {
    'bucket': 'chromium-clang-format',
    'condition': 'host_os == "mac" and host_cpu == "x64" and non_git_source',
    'dep_type': 'gcs',
    'objects': [
      {
        'object_name': '7d46d237f9664f41ef46b10c1392dcb559250f25',
        'sha256sum': '0c3c13febeb0495ef0086509c24605ecae9e3d968ff9669d12514b8a55c7824e',
        'size_bytes': 3204008,
        'generation': 1738622388489334,
        'output_file': 'clang-format',
      },
    ],
  },
  'src/buildtools/mac_arm64-format': {
    'bucket': 'chromium-clang-format',
    'condition': 'host_os == "mac" and host_cpu == "arm64" and non_git_source',
    'dep_type': 'gcs',
    'objects': [
      {
        'object_name': '8503422f469ae56cc74f0ea2c03f2d872f4a2303',
        'sha256sum': 'dabf93691361e8bd1d07466d67584072ece5c24e2b812c16458b8ff801c33e29',
        'size_bytes': 3212560,
        'generation': 1738622390717009,
        'output_file': 'clang-format',
      },
    ],
  },
  'src/buildtools/linux64-format': {
    'bucket': 'chromium-clang-format',
    'condition': 'host_os == "linux" and non_git_source',
    'dep_type': 'gcs',
    'objects': [
      {
        'object_name': '79a7b4e5336339c17b828de10d80611ff0f85961',
        'sha256sum': '889266a51681d55bd4b9e02c9a104fa6ee22ecdfa7e8253532e5ea47e2e4cb4a',
        'size_bytes': 3899440,
        'generation': 1738622384130717,
        'output_file': 'clang-format',
      },
    ],
  },
  'src/third_party/data_sharing_sdk': {
      'packages': [
          {
              'package': 'chrome_internal/third_party/google3/data_sharing_sdk',
              'version': 'NQq9pR3VrvsqPtQSfrezFe8hL-z_SfZsONkwC-MqX6wC',
          },
      ],
      'condition': 'checkout_src_internal and non_git_source',
      'dep_type': 'cipd',
  },
  'src/third_party/llvm-build/Release+Asserts': {
    'dep_type': 'gcs',
    'bucket': 'chromium-browser-clang',
    'condition': 'not llvm_force_head_revision',
    'objects': [
      {
        # The Android libclang_rt.builtins libraries are currently only included in the Linux clang package.
        'object_name': 'Linux_x64/clang-llvmorg-22-init-14273-gea10026b-3.tar.xz',
        'sha256sum': '91f570063e53b0ece9d68e5237135d52ccf38e543abb31ac50c0c01588c8d40a',
        'size_bytes': 56531092,
        'generation': 1764612628593714,
        'condition': '(host_os == "linux" or checkout_android) and non_git_source',
      },
      {
        'object_name': 'Linux_x64/clang-tidy-llvmorg-22-init-14273-gea10026b-3.tar.xz',
        'sha256sum': '0728a5b1db154978e36f9a11fdf3b30c450b27aa0ee8607ca512fa3057e0585d',
        'size_bytes': 14265312,
        'generation': 1764612628949595,
        'condition': 'host_os == "linux" and checkout_clang_tidy and non_git_source',
      },
      {
        'object_name': 'Linux_x64/clangd-llvmorg-22-init-14273-gea10026b-3.tar.xz',
        'sha256sum': 'bcd1702f30d2f933cba33248b2f48a1d3a116e70d619fdd62192642e57b6cf06',
        'size_bytes': 14463416,
        'generation': 1764612629026548,
        'condition': 'host_os == "linux" and checkout_clangd and non_git_source',
      },
      {
        'object_name': 'Linux_x64/llvm-code-coverage-llvmorg-22-init-14273-gea10026b-3.tar.xz',
        'sha256sum': '8ba145f5474b2f3fb3cd81a4effb72c56f10cdffae1d3365b9254bd6889172c1',
        'size_bytes': 2307044,
        'generation': 1764612629531956,
        'condition': 'host_os == "linux" and checkout_clang_coverage_tools and non_git_source',
      },
      {
        'object_name': 'Linux_x64/llvmobjdump-llvmorg-22-init-14273-gea10026b-3.tar.xz',
        'sha256sum': '6caf67f5ba0891b0e5b0228ebb63a24956477f3a72255a62c0b8b0b8667e742e',
        'size_bytes': 5730840,
        'generation': 1764612629250891,
        'condition': '((checkout_linux or checkout_mac or checkout_android) and host_os == "linux") and non_git_source',
      },
      {
        'object_name': 'Mac/clang-llvmorg-22-init-14273-gea10026b-3.tar.xz',
        'sha256sum': '70320598b220bc73e7d4099dd87c0e8b510f640cadfdcf517e0a35e2cb6d8b1a',
        'size_bytes': 54119080,
        'generation': 1764612631537558,
        'condition': 'host_os == "mac" and host_cpu == "x64"',
      },
      {
        'object_name': 'Mac/clang-mac-runtime-library-llvmorg-22-init-14273-gea10026b-3.tar.xz',
        'sha256sum': 'aaa59817158482d916d4b821669c372090481ee7b96959b5157b7be4aa004cec',
        'size_bytes': 1007104,
        'generation': 1764612653376647,
        'condition': 'checkout_mac and not host_os == "mac"',
      },
      {
        'object_name': 'Mac/clang-tidy-llvmorg-22-init-14273-gea10026b-3.tar.xz',
        'sha256sum': 'ec1ec60f9d6098c9c84c5f09ae7aa9f5d94de3f256e12a64c6c4d715480d6b07',
        'size_bytes': 14295328,
        'generation': 1764612631824441,
        'condition': 'host_os == "mac" and host_cpu == "x64" and checkout_clang_tidy',
      },
      {
        'object_name': 'Mac/clangd-llvmorg-22-init-14273-gea10026b-3.tar.xz',
        'sha256sum': '484cff288c3ad25730ff94eda319d7aeee5c86f4367fda48e8825b0cbc9c9b33',
        'size_bytes': 15822332,
        'generation': 1764612632004677,
        'condition': 'host_os == "mac" and host_cpu == "x64" and checkout_clangd',
      },
      {
        'object_name': 'Mac/llvm-code-coverage-llvmorg-22-init-14273-gea10026b-3.tar.xz',
        'sha256sum': '1271fef56556d158c960eda57d9fb6e15d9125234b62a4591742832c0c21306e',
        'size_bytes': 2334364,
        'generation': 1764612632455988,
        'condition': 'host_os == "mac" and host_cpu == "x64" and checkout_clang_coverage_tools',
      },
      {
        'object_name': 'Mac/llvmobjdump-llvmorg-22-init-14273-gea10026b-3.tar.xz',
        'sha256sum': 'b185d69c3d53cd59c2d7a8b9f037f436656f5b79a97e1010b4580757fdb59c2d',
        'size_bytes': 5609228,
        'generation': 1764612632088648,
        'condition': 'host_os == "mac" and host_cpu == "x64"',
      },
      {
        'object_name': 'Mac_arm64/clang-llvmorg-22-init-14273-gea10026b-3.tar.xz',
        'sha256sum': 'c83d90592462dc612042e14031673f7ee99f540ecf09b0ccf4e1ab8204c8edfd',
        'size_bytes': 45143528,
        'generation': 1764612655158394,
        'condition': 'host_os == "mac" and host_cpu == "arm64"',
      },
      {
        'object_name': 'Mac_arm64/clang-tidy-llvmorg-22-init-14273-gea10026b-3.tar.xz',
        'sha256sum': '12b2d9e0c08a0e7c62b181f1cab3cfa38f106fe29f493c6821ab21a9b856a0b4',
        'size_bytes': 12299224,
        'generation': 1764612655792796,
        'condition': 'host_os == "mac" and host_cpu == "arm64" and checkout_clang_tidy',
      },
      {
        'object_name': 'Mac_arm64/clangd-llvmorg-22-init-14273-gea10026b-3.tar.xz',
        'sha256sum': 'df79c36d4ad39ca9feae0ee9640b0f9b4b0f5194933392dab2bc707583acbe92',
        'size_bytes': 12684180,
        'generation': 1764612655708892,
        'condition': 'host_os == "mac" and host_cpu == "arm64" and checkout_clangd',
      },
      {
        'object_name': 'Mac_arm64/llvm-code-coverage-llvmorg-22-init-14273-gea10026b-3.tar.xz',
        'sha256sum': 'b1d2a7e44582f671e980de9747412ea41f3248de67953c765cdc7e6725f61404',
        'size_bytes': 1966596,
        'generation': 1764612656252438,
        'condition': 'host_os == "mac" and host_cpu == "arm64" and checkout_clang_coverage_tools',
      },
      {
        'object_name': 'Mac_arm64/llvmobjdump-llvmorg-22-init-14273-gea10026b-3.tar.xz',
        'sha256sum': '2871b960e650898c448e9e04f753cfb9d202a16e5abeacf9c6d81090ebdef11d',
        'size_bytes': 5345876,
        'generation': 1764612656056708,
        'condition': 'host_os == "mac" and host_cpu == "arm64"',
      },
      {
        'object_name': 'Win/clang-llvmorg-22-init-14273-gea10026b-3.tar.xz',
        'sha256sum': 'ccbd73543ee52947950a0ed0cc9f283b5d3f0daf4ded41f1623aa45b12e840d6',
        'size_bytes': 48347024,
        'generation': 1764612682127943,
        'condition': 'host_os == "win"',
      },
      {
        'object_name': 'Win/clang-tidy-llvmorg-22-init-14273-gea10026b-3.tar.xz',
        'sha256sum': 'f7a0d8072cf2b27cb894dfc3ce541040d4d8aeda0f9cde417c5e66f34084ce16',
        'size_bytes': 14235468,
        'generation': 1764612682470243,
        'condition': 'host_os == "win" and checkout_clang_tidy',
      },
      {
        'object_name': 'Win/clang-win-runtime-library-llvmorg-22-init-14273-gea10026b-3.tar.xz',
        'sha256sum': '1c31fe0026ec6adbb707433ab43f674804b4345184144dcef987ae5e6bc405fd',
        'size_bytes': 2521152,
        'generation': 1764612703739250,
        'condition': 'checkout_win and not host_os == "win"',
      },
      {
        'object_name': 'Win/clangd-llvmorg-22-init-14273-gea10026b-3.tar.xz',
        'sha256sum': 'f13e22f876e4874a3b0cfd1be05f4a5b6905b48ae77fcdcf4e9a2da6c38b0435',
        'size_bytes': 14642200,
        'generation': 1764612682678053,
       'condition': 'host_os == "win" and checkout_clangd',
      },
      {
        'object_name': 'Win/llvm-code-coverage-llvmorg-22-init-14273-gea10026b-3.tar.xz',
        'sha256sum': 'c85af645852ccd93fe2a0a4768b338f14b7fc9169709bbbf29cfbae1d91c872e',
        'size_bytes': 2385668,
        'generation': 1764612683270464,
        'condition': 'host_os == "win" and checkout_clang_coverage_tools',
      },
      {
        'object_name': 'Win/llvmobjdump-llvmorg-22-init-14273-gea10026b-3.tar.xz',
        'sha256sum': 'ae6c7002b8871676da1f73a350fcc37c80836f962421ea77c8aaba3a0a6a0c93',
        'size_bytes': 5722184,
        'generation': 1764612682869464,
        'condition': '(checkout_linux or checkout_mac or checkout_android) and host_os == "win"',
      },
    ]
  },
  # Update prebuilt Rust toolchain.
  'src/third_party/rust-toolchain': {
    'dep_type': 'gcs',
    'bucket': 'chromium-browser-clang',
    'condition': 'not rust_force_head_revision',
    'objects': [
      {
        'object_name': 'Linux_x64/rust-toolchain-11339a0ef5ed586bb7ea4f85a9b7287880caac3a-1-llvmorg-22-init-14273-gea10026b.tar.xz',
        'sha256sum': '15cf3019dd864ea64d63e5cf33de1ded76ace4f70cbb224812fccbf03b342096',
        'size_bytes': 140380848,
        'generation': 1762971367461755,
        'condition': 'host_os == "linux" and non_git_source',
      },
      {
        'object_name': 'Mac/rust-toolchain-11339a0ef5ed586bb7ea4f85a9b7287880caac3a-1-llvmorg-22-init-14273-gea10026b.tar.xz',
        'sha256sum': 'bb8be97e327b72f03f9caadae5557c7ce0ddac66e6dcc37cd3b8e65fca074e4b',
        'size_bytes': 134345832,
        'generation': 1762971369029231,
        'condition': 'host_os == "mac" and host_cpu == "x64"',
      },
      {
        'object_name': 'Mac_arm64/rust-toolchain-11339a0ef5ed586bb7ea4f85a9b7287880caac3a-1-llvmorg-22-init-14273-gea10026b.tar.xz',
        'sha256sum': '72b4421aa531822ba4b32533e6efae4e7c06eb15dcc32d1fb361023dd937c63c',
        'size_bytes': 121923464,
        'generation': 1762971370566007,
        'condition': 'host_os == "mac" and host_cpu == "arm64"',
      },
      {
        'object_name': 'Win/rust-toolchain-11339a0ef5ed586bb7ea4f85a9b7287880caac3a-1-llvmorg-22-init-14273-gea10026b.tar.xz',
        'sha256sum': '8cd2f0c4d312c0a7c127a8e6adf0a73ceecad316997e925fbebc1d855e95f6ba',
        'size_bytes': 197809928,
        'generation': 1762971372158285,
        'condition': 'host_os == "win"',
      },
    ],
  },
  'src/third_party/clang-format/script':
    Var('chromium_git') +
    '/external/github.com/llvm/llvm-project/clang/tools/clang-format.git@' +
    Var('clang_format_revision'),
  'src/buildtools/linux64': {
    'packages': [
      {
        'package': 'gn/gn/linux-${{arch}}',
        'version': Var('gn_version'),
      }
    ],
    'dep_type': 'cipd',
    'condition': 'host_os == "linux" and non_git_source',
  },
  'src/buildtools/mac': {
    'packages': [
      {
        'package': 'gn/gn/mac-${{arch}}',
        'version': Var('gn_version'),
      }
    ],
    'dep_type': 'cipd',
    'condition': 'host_os == "mac"',
  },
  'src/buildtools/win': {
    'packages': [
      {
        'package': 'gn/gn/windows-amd64',
        'version': Var('gn_version'),
      }
    ],
    'dep_type': 'cipd',
    'condition': 'host_os == "win"',
  },
  'src/buildtools/reclient': {
    'packages': [
      {
        'package': Var('reclient_package') + '${{platform}}',
        'version': Var('reclient_version'),
      }
    ],
    'condition': 'download_reclient and non_git_source',
    'dep_type': 'cipd',
  },

  'src/content/test/data/gpu/meet_effects_videos': {
    'packages': [
      {
        'package': 'chromium/testing/meet-effects-videos',
        'version': 'FSo3cmbzDsTKY2z_P-d3yBMgtaXIKzHpW4q6w4z4HpYC',
      }
    ],
    'dep_type': 'cipd',
  },
  'src/content/test/data/gpu/meet_effects': {
    'dep_type': 'gcs',
    'bucket': 'meet-bundles',
    'condition': 'non_git_source',
    'objects': [
      {
        'object_name': 'meet-gpu-tests/841784911.tar.gz',
        'sha256sum': 'a5a6ef7df534f1fdc0c5f175b2cf7ab00d9fc105c390fa807214eab1a1c4438e',
        'size_bytes': 279900378,
        'generation': 1765268822846619,
      },
    ],
  },

  'src/third_party/compiler-rt/src':
    Var('chromium_git') +
    '/external/github.com/llvm/llvm-project/compiler-rt.git' + '@' +
    Var('compiler_rt_revision'),
  'src/third_party/libc++/src':
    Var('chromium_git') +
    '/external/github.com/llvm/llvm-project/libcxx.git' + '@' +
    Var('libcxx_revision'),
  'src/third_party/libc++abi/src':
    Var('chromium_git') +
    '/external/github.com/llvm/llvm-project/libcxxabi.git' + '@' +
    Var('libcxxabi_revision'),
  'src/third_party/libunwind/src':
    Var('chromium_git') +
    '/external/github.com/llvm/llvm-project/libunwind.git' + '@' +
    Var('libunwind_revision'),
  'src/third_party/llvm-libc/src':
    Var('chromium_git') +
    '/external/github.com/llvm/llvm-project/libc.git' + '@' +
    Var('llvm_libc_revision'),

  'src/third_party/updater/chrome_linux64/cipd': {
      'dep_type': 'cipd',
      'condition': 'checkout_linux and non_git_source',
      'packages': [
        {
          'package': 'chromium/third_party/updater/chrome_linux64',
          'version': 'version:2@144.0.7547.0',
        },
      ],
  },

  'src/third_party/updater/chrome_mac_universal/cipd': {
      'dep_type': 'cipd',
      'condition': 'checkout_mac',
      'packages': [
        {
          'package': 'chromium/third_party/updater/chrome_mac_universal',
          'version': 'version:2@144.0.7547.0',
        },
      ],
  },

  'src/third_party/updater/chrome_mac_universal_prod/cipd': {
      'dep_type': 'cipd',
      'condition': 'checkout_mac',
      'packages': [
        {
          'package': 'chromium/third_party/updater/chrome_mac_universal_prod',
          'version': 'version:2@144.0.7547.0',
        },
      ],
  },

  'src/chrome/test/data/perf/canvas_bench':
    Var('chromium_git') + '/chromium/canvas_bench.git' + '@' + 'a7b40ea5ae0239517d78845a5fc9b12976bfc732',

  'src/chrome/test/data/perf/frame_rate/content':
    Var('chromium_git') + '/chromium/frame_rate/content.git' + '@' + 'c10272c88463efeef6bb19c9ec07c42bc8fe22b9',

  'src/chrome/test/data/safe_browsing/dmg': {
    'packages': [
      {
        'package': 'chromium/chrome/test/data/safe_browsing/dmg',
        'version': '03TLfNQgc59nHmyWtYWJfFaUrEW8QDJJzXwm-672m-QC',
      },
    ],
    'condition': 'checkout_mac',
    'dep_type': 'cipd',
  },

  'src/components/variations/test_data/cipd': {
    'packages': [
      {
        'package': 'chromium/chrome/test/data/variations/cipd',
        'version': '_0-UKLP9RE9ZgLUaV0grnFr6RondxoK-_4sJEaRtdiUC',
      },
    ],
    'dep_type': 'cipd',
  },

  'src/chrome/test/data/xr/webvr_info':
    Var('chromium_git') + '/external/github.com/toji/webvr.info.git' + '@' + 'c58ae99b9ff9e2aa4c524633519570bf33536248',

  'src/net/third_party/quiche/src':
    Var('quiche_git') + '/quiche.git' + '@' +  Var('quiche_revision'),

  'src/testing/libfuzzer/fuzzers/wasm_corpus':
    Var('chromium_git') + '/v8/fuzzer_wasm_corpus.git' + '@' +  '1df5e50a45db9518a56ebb42cb020a94a090258b',

  'src/tools/copybara': {
      'packages' : [
          {
              'package': 'infra/3pp/tools/copybara',
              'version': 'dgqNEgpF5OVIfAUisIefJgYrPj2E1KlPd2hC9cN9LfcC',
          },
      ],
      'condition': 'host_os == "linux" and checkout_copybara',
      'dep_type': 'cipd',
  },

  'src/tools/luci-go': {
      'packages': [
        {
          'package': 'infra/tools/luci/cas/${{platform}}',
          'version': Var('luci_go'),
        },
        # TODO(crbug.com/382506663): Remove after investigation/deprecation
        {
          'package': 'infra/tools/luci/isolate/${{platform}}',
          'version': Var('luci_go'),
        },
        {
          'package': 'infra/tools/luci/swarming/${{platform}}',
          'version': Var('luci_go'),
        },
      ],
      'condition': 'non_git_source',
      'dep_type': 'cipd',
  },

  'src/third_party/android_toolchain/ndk': {
      'packages': [
            {
                'package': 'chromium/third_party/android_toolchain/android_toolchain',
                'version': 'KXOia11cm9lVdUdPlbGLu8sCz6Y4ey_HV2s8_8qeqhgC',
            },
      ],
      'condition': 'checkout_android and non_git_source',
      'dep_type': 'cipd',
  },

  'src/third_party/androidx/cipd': {
    'packages': [
      {
          'package': 'chromium/third_party/androidx',
          'version': 'dUYDoYVH-NzyM8K7wLLtKL4FIBeid3P4p_gbwEOapJ4C',
      },
    ],
    'condition': 'checkout_android and non_git_source',
    'dep_type': 'cipd',
  },

  'src/third_party/androidx_javascriptengine/src': {
      'url': Var('chromium_git') + '/aosp/platform/frameworks/support/javascriptengine/javascriptengine/src.git' + '@' + '7539442db1dc790c64d0d9bade922329292d834b',
      'condition': 'checkout_android',
  },

  'src/third_party/android_system_sdk/cipd': {
      'packages': [
          {
              'package': 'chromium/third_party/android_system_sdk/public',
              'version': 'Pfb3HDUW_uRir_VVTCYkGhf6bnPPF55NUJO2WXOxIe0C',
          },
      ],
      'condition': 'checkout_android and non_git_source',
      'dep_type': 'cipd',
  },

  'src/third_party/highway/src':
    Var('chromium_git') + '/external/github.com/google/highway.git' + '@' + Var('highway_revision'),

  'src/third_party/breakpad/breakpad':
    Var('chromium_git') + '/breakpad/breakpad.git' + '@' + Var('breakpad_revision'),

  'src/third_party/cast_core/public/src':
    Var('chromium_git') + '/cast_core/public' + '@' + 'f5ee589bdaea60418f670fa176be15ccb9a34942',

  'src/third_party/catapult':
    Var('chromium_git') + '/catapult.git' + '@' + Var('catapult_revision'),

  'src/third_party/ced/src':
    Var('chromium_git') + '/external/github.com/google/compact_enc_det.git' + '@' + 'ba412eaaacd3186085babcd901679a48863c7dd5',

  'src/third_party/cpu_features/src':
    Var('chromium_git') + '/external/github.com/google/cpu_features.git' + '@' + '936b9ab5515dead115606559502e3864958f7f6e',

  'src/third_party/cpuinfo/src':
    Var('chromium_git') + '/external/github.com/pytorch/cpuinfo.git' + '@' + '161a9ec374884f4b3e85725cb22e05f9458fdc93',

  'src/third_party/crc32c/src':
    Var('chromium_git') + '/external/github.com/google/crc32c.git' + '@' + 'd3d60ac6e0f16780bcfcc825385e1d338801a558',

  # For Linux and Chromium OS.
  'src/third_party/cros_system_api': {
      'url': Var('chromium_git') + '/chromiumos/platform2/system_api.git' + '@' + '23dafcbba81fc5bfc920b07774193a49c41e66a7',
      'condition': 'checkout_linux or checkout_chromeos',
  },

  'src/third_party/crossbench':
    Var('chromium_git') + '/crossbench.git' + '@' + Var('crossbench_revision'),

  'src/third_party/crossbench-web-tests':
    Var('chromium_git') + '/chromium/web-tests.git' + '@' + Var('crossbench_web_tests_revision'),

  'src/third_party/depot_tools':
    Var('chromium_git') + '/chromium/tools/depot_tools.git' + '@' + '3f0c3aaedcee8a9afc865baf9093875d0feb4473',

  'src/third_party/devtools-frontend/src':
    Var('chromium_git') + '/devtools/devtools-frontend' + '@' + Var('devtools_frontend_revision'),

  'src/third_party/dom_distiller_js/dist':
    Var('chromium_git') + '/chromium/dom-distiller/dist.git' + '@' + '199de96b345ada7c6e7e6ba3d2fa7a6911b8767d',

  'src/third_party/dragonbox/src':
    Var('chromium_git') + '/external/github.com/jk-jeon/dragonbox.git' + '@' + '6c7c925b571d54486b9ffae8d9d18a822801cbda',

  'src/third_party/eigen3/src':
    Var('chromium_git') + '/external/gitlab.com/libeigen/eigen.git' + '@' + '49623d0c4e1af3c680845191948d10f6d3e92f8a',

  'src/third_party/fast_float/src':
    Var('chromium_git') + '/external/github.com/fastfloat/fast_float.git' + '@' + 'cb1d42aaa1e14b09e1452cfdef373d051b8c02a4',

  'src/third_party/federated_compute/src':
    Var('chromium_git') + '/external/github.com/google-parfait/federated-compute.git' + '@' + 'e51058dfe7888094ecc09cda38bfceffd4d4664b',

  'src/third_party/ffmpeg':
    Var('chromium_git') + '/chromium/third_party/ffmpeg.git' + '@' + Var('ffmpeg_revision'),

  'src/third_party/flac':
    Var('chromium_git') + '/chromium/deps/flac.git' + '@' + '807e251d9f8c5dd6059e547931e9c6a4251967af',

  'src/third_party/flatbuffers/src':
    Var('chromium_git') + '/external/github.com/google/flatbuffers.git' + '@' + '187240970746d00bbd26b0f5873ed54d2477f9f3',

  'src/third_party/fontconfig/src': {
      'url': Var('chromium_git') + '/external/fontconfig.git' + '@' + 'd62c2ab268d1679335daa8fb0ea6970f35224a76',
      'condition': 'checkout_linux',
  },

  'src/third_party/fp16/src':
    Var('chromium_git') + '/external/github.com/Maratyszcza/FP16.git' + '@' + '3d2de1816307bac63c16a297e8c4dc501b4076df',

  'src/third_party/freetype/src':
    Var('chromium_git') + '/chromium/src/third_party/freetype2.git' + '@' + Var('freetype_revision'),

  'src/third_party/fxdiv/src':
    Var('chromium_git') + '/external/github.com/Maratyszcza/FXdiv.git' + '@' + '63058eff77e11aa15bf531df5dd34395ec3017c8',

  'src/third_party/harfbuzz-ng/src':
    Var('chromium_git') + '/external/github.com/harfbuzz/harfbuzz.git' + '@' + Var('harfbuzz_revision'),

  'src/third_party/ink/src':
    Var('chromium_git') + '/external/github.com/google/ink.git' + '@' + Var('ink_revision'),

  'src/third_party/ink_stroke_modeler/src':
    Var('chromium_git') + '/external/github.com/google/ink-stroke-modeler.git' + '@' + Var('ink_stroke_modeler_revision'),

  'src/third_party/instrumented_libs': {
    'url': Var('chromium_git') + '/chromium/third_party/instrumented_libraries.git' + '@' + '69015643b3f68dbd438c010439c59adc52cac808',
    'condition': 'checkout_instrumented_libraries',
  },

  'src/third_party/googletest/src':
    Var('chromium_git') + '/external/github.com/google/googletest.git' + '@' + Var('googletest_revision'),

  'src/third_party/icu':
    Var('chromium_git') + '/chromium/deps/icu.git' + '@' + 'a86a32e67b8d1384b33f8fa48c83a6079b86f8cd',

  'src/third_party/domato/src':
    Var('chromium_git') + '/external/github.com/googleprojectzero/domato.git' + '@' + Var('domato_revision'),

  'src/third_party/libaddressinput/src':
    Var('chromium_git') + '/external/libaddressinput.git' + '@' + '2610f7b1043d6784ada41392fc9392d1ea09ea07',

  'src/third_party/crabbyavif/src':
    Var('chromium_git') + '/external/github.com/webmproject/CrabbyAvif.git' + '@' + Var('crabbyavif_revision'),

  'src/third_party/libjpeg_turbo':
    Var('chromium_git') + '/chromium/deps/libjpeg_turbo.git' + '@' + '6383cf609c1f63c18af0f59b2738caa0c6c7e379',

  'src/third_party/lss': {
      'url': Var('chromium_git') + '/linux-syscall-support.git' + '@' + Var('lss_revision'),
      'condition': 'checkout_android or checkout_linux',
  },

  'src/third_party/nasm': {
      'url': Var('chromium_git') + '/chromium/deps/nasm.git' + '@' +
      'af5eeeb054bebadfbb79c7bcd100a95e2ad4525f'
  },

  'src/third_party/ninja': {
    'packages': [
      {
        'package': Var('ninja_package') + '${{platform}}',
        'version': Var('ninja_version'),
      }
    ],
    'condition': 'non_git_source',
    'dep_type': 'cipd',
  },
  'src/third_party/siso/cipd': {
    'packages': [
      {
        'package': 'build/siso/${{platform}}',
        'version': Var('siso_version'),
      }
    ],
    'condition': 'non_git_source',
    'dep_type': 'cipd',
  },

  'src/third_party/pdfium':
    Var('pdfium_git') + '/pdfium.git' + '@' +  Var('pdfium_revision'),

  'src/third_party/perfetto':
    Var('chromium_git') + '/external/github.com/google/perfetto.git' + '@' + 'bb4ddb7e367cfbb10d8c611ce544657eb18a16b4',

  'src/third_party/re2/src':
    Var('chromium_git') + '/external/github.com/google/re2.git' + '@' + 'e7aec5985072c1dbe735add802653ef4b36c231a',

  'src/third_party/r8/cipd': {
      'packages': [
          {
              'package': 'chromium/third_party/r8',
              'version': 'aQiRizhSSPFGfHGuoC-0t2O4lFR5OY2qySLXtBS_vtIC',
          },
      ],
      'condition': 'checkout_android and non_git_source',
      'dep_type': 'cipd',
  },

  # This duplication is intentional, so we avoid updating the r8.jar used by
  # dexing unless necessary, since each update invalidates all incremental
  # dexing and unnecessarily slows down all bots.
  'src/third_party/r8/d8/cipd': {
      'packages': [
          {
              'package': 'chromium/third_party/r8',
              'version': 'a4fVqbIycCDqs1714SLRqxEdz6P-sH-z1QT_eeeF0PcC',
          },
      ],
      'condition': 'checkout_android and non_git_source',
      'dep_type': 'cipd',
  },

  'src/third_party/requests/src': {
      'url': Var('chromium_git') + '/external/github.com/kennethreitz/requests.git' + '@' + 'c7e0fc087ceeadb8b4c84a0953a422c474093d6d',
      'condition': 'checkout_android',
  },

  'src/third_party/skia':
    Var('skia_git') + '/skia.git' + '@' +  Var('skia_revision'),

  'src/third_party/vulkan-deps': '{chromium_git}/vulkan-deps@2dae52f68388badde6f2e357e11388b7412dd947',
  'src/third_party/glslang/src': '{chromium_git}/external/github.com/KhronosGroup/glslang@53ead8fa37722785bf478689c78bab8f01dc1797',
  'src/third_party/spirv-cross/src': '{chromium_git}/external/github.com/KhronosGroup/SPIRV-Cross@b8fcf307f1f347089e3c46eb4451d27f32ebc8d3',
  'src/third_party/spirv-headers/src': '{chromium_git}/external/github.com/KhronosGroup/SPIRV-Headers@6146b3d9ad4fcc5fb512209d348e97ce03749169',
  'src/third_party/spirv-tools/src': '{chromium_git}/external/github.com/KhronosGroup/SPIRV-Tools@940d4850f12cef87336b94fee4b7eaf9ca394222',
  'src/third_party/vulkan-headers/src': '{chromium_git}/external/github.com/KhronosGroup/Vulkan-Headers@2fa203425eb4af9dfc6b03f97ef72b0b5bcb8350',
  'src/third_party/vulkan-loader/src': '{chromium_git}/external/github.com/KhronosGroup/Vulkan-Loader@0db9a0d55e33f8b63b733f37dc59eb441d51abf0',
  'src/third_party/vulkan-tools/src': '{chromium_git}/external/github.com/KhronosGroup/Vulkan-Tools@2170c33bb3d2af976a1894666bfd3dc80cbe4da8',
  'src/third_party/vulkan-utility-libraries/src': '{chromium_git}/external/github.com/KhronosGroup/Vulkan-Utility-Libraries@c010c19e796035e92fb3b0462cb887518a41a7c1',
  'src/third_party/vulkan-validation-layers/src': '{chromium_git}/external/github.com/KhronosGroup/Vulkan-ValidationLayers@967e32890c476312fdd62be2622edea664215ff1',

  'src/tools/skia_goldctl/linux': {
      'packages': [
        {
          'package': 'skia/tools/goldctl/linux-amd64',
          'version': 'yTR-Js4FbeTvqObewyJyRmr39wR04DHCNjjeLkgEfZEC',
        },
      ],
      'dep_type': 'cipd',
      'condition': 'checkout_linux and non_git_source',
  },
  'src/tools/skia_goldctl/win': {
      'packages': [
        {
          'package': 'skia/tools/goldctl/windows-amd64',
          'version': 'Da5Ya6mYbLxKhrtipFoPIZ5OASVAOy4lxDi8n5OnhN0C',
        },
      ],
      'dep_type': 'cipd',
      'condition': 'checkout_win',
  },

  'src/tools/skia_goldctl/mac_amd64': {
      'packages': [
        {
          'package': 'skia/tools/goldctl/mac-amd64',
          'version': '1EbFABRLnKXjSwrQyXiazV1dpZmsc6rNsPulULz1aYcC',
        },
      ],
      'dep_type': 'cipd',
      'condition': 'checkout_mac',
  },

  'src/tools/skia_goldctl/mac_arm64': {
      'packages': [
        {
          'package': 'skia/tools/goldctl/mac-arm64',
          'version': 'R_blxv41vyVuvXG6g0-8OU0kt5eHXBJeZmXB0FJ5beYC',
        },
      ],
      'dep_type': 'cipd',
      'condition': 'checkout_mac',
  },

  'src/v8':
    Var('chromium_git') + '/v8/v8.git' + '@' +  Var('v8_revision'),

# See checkout_src_internal_infra declaration.
# LINT.IfChange
  'src/internal': {
    'url': Var('chrome_git') + '/chrome/src-internal.git' + '@' + Var('src_internal_revision'),
    'condition': 'checkout_src_internal or checkout_src_internal_infra',
  },
# LINT.ThenChange(/components/cronet/gn2bp/copy.bara.sky)

  # === ANDROID_DEPS Generated Code Start ===
  # Generated by //third_party/android_deps/fetch_all.py
  'src/third_party/android_deps/cipd/libs/com_android_tools_common': {
      'packages': [
          {
              'package': 'chromium/third_party/android_deps/libs/com_android_tools_common',
              'version': 'version:2@30.2.0-beta01.cr2',
          },
      ],
      'condition': 'checkout_android and non_git_source',
      'dep_type': 'cipd',
  },
  # === ANDROID_DEPS Generated Code End ===

  'src/tools/resultdb': {
      'packages': [
        {
          'package': 'infra/tools/result_adapter/${{platform}}',
          'version': Var('result_adapter_revision'),
        },
      ],
      'condition': 'non_git_source',
      'dep_type': 'cipd',
  },
}


include_rules = [
  # Everybody can use some things.
  # NOTE: THIS HAS TO STAY IN SYNC WITH third_party/DEPS which disallows these.
  '+base',
  '+build',
  '+ipc',
  # perfetto is base's public dependency.
  '+third_party/perfetto/include/perfetto/tracing',
  '+third_party/perfetto/include/perfetto/test',

  # PartitionAlloc is located at `base/allocator/partition_allocator` but
  # prefers its own include path:
  # `#include "partition_alloc/..."` is prefered to
  # `#include "base/allocator/partition_allocator/src/partition_alloc/..."`.
  "+partition_alloc",
  "-base/allocator/partition_allocator",

  # Everybody can use headers generated by tools/generate_library_loader.
  '+library_loaders',

  '+testing',
  '+third_party/jni_zero',
  '+third_party/google_benchmark/src/include/benchmark/benchmark.h',
  '+third_party/icu/source/common/unicode',
  '+third_party/icu/source/i18n/unicode',
  '+url',

  # Abseil is allowed by default, but some features are banned. See
  # //styleguide/c++/c++-features.md.
  '+third_party/abseil-cpp',
  '-third_party/abseil-cpp/absl/algorithm/container.h',
  '-third_party/abseil-cpp/absl/base/attributes.h',
  '-third_party/abseil-cpp/absl/base/no_destructor.h',
  '-third_party/abseil-cpp/absl/base/nullability.h',
  '-third_party/abseil-cpp/absl/container/btree_map.h',
  '-third_party/abseil-cpp/absl/container/btree_set.h',
  '-third_party/abseil-cpp/absl/flags',
  '-third_party/abseil-cpp/absl/functional/any_invocable.h',
  '-third_party/abseil-cpp/absl/functional/bind_front.h',
  '-third_party/abseil-cpp/absl/functional/function_ref.h',
  '-third_party/abseil-cpp/absl/hash',
  '+third_party/abseil-cpp/absl/hash/hash_testing.h',
  '-third_party/abseil-cpp/absl/log',
  '-third_party/abseil-cpp/absl/random',
  '-third_party/abseil-cpp/absl/status/statusor.h',
  '-third_party/abseil-cpp/absl/strings',
  '+third_party/abseil-cpp/absl/strings/ascii.h',
  '+third_party/abseil-cpp/absl/strings/cord.h',
  '+third_party/abseil-cpp/absl/strings/str_format.h',
  '-third_party/abseil-cpp/absl/synchronization',
  '-third_party/abseil-cpp/absl/time',
  '-third_party/abseil-cpp/absl/types/any.h',
  '-third_party/abseil-cpp/absl/types/optional.h',
  '-third_party/abseil-cpp/absl/types/span.h',
  '-third_party/abseil-cpp/absl/types/variant.h',
  '-third_party/abseil-cpp/absl/utility/utility.h',
]


# checkdeps.py shouldn't check include paths for files in these dirs:
skip_child_includes = [
  'out',
  'skia',
  'testing',
  'third_party/abseil-cpp',
  'v8',
]


hooks = [
  # Download and initialize "vpython" VirtualEnv environment packages for
  # Python3. We do this before running any other hooks so that any other
  # hooks that might use vpython don't trip over unexpected issues and
  # don't run slower than they might otherwise need to.
  {
    'name': 'vpython3_common',
    'pattern': '.',
    'action': [ 'vpython3',
                '-vpython-spec', 'src/.vpython3',
                '-vpython-tool', 'install',
    ],
  },
  {
    # This clobbers when necessary (based on get_landmines.py). This should
    # run as early as possible so that other things that get/generate into the
    # output directory will not subsequently be clobbered.
    'name': 'landmines',
    'pattern': '.',
    'action': [
        'python3',
        'src/build/landmines.py',
    ],
  },
  {
    # This clobbers when necessary (based on the internal ios version of
    # get_landmines.py). This should run as early as possible so that
    # other things that get/generate into the output directory will not
    # subsequently be clobbered. This script is only run# for iOS build
    # with src_internal.
    'name': 'landmines_ios_internal',
    'pattern': '.',
    'condition': 'checkout_ios and checkout_src_internal',
    'action': [
        'python3',
        'src/build/landmines.py',
        '--landmine-scripts',
        'src/ios_internal/build/get_landmines.py',
        '--landmines-path',
        'src/ios_internal/.landmines',
    ],
  },
  {
    # Ensure that the DEPS'd "depot_tools" has its self-update capability
    # disabled.
    'name': 'disable_depot_tools_selfupdate',
    'pattern': '.',
    'action': [
        'python3',
        'src/third_party/depot_tools/update_depot_tools_toggle.py',
        '--disable',
    ],
  },
  {
    # Ensure we remove any file from disk that is no longer needed (e.g. after
    # hooks to native GCS deps migration).
    'name': 'remove_stale_files',
    'pattern': '.',
    'action': [
        'python3',
        'src/tools/remove_stale_files.py',
        'src/third_party/test_fonts/test_fonts.tar.gz', # Remove after 20240901
        'src/third_party/node/node_modules.tar.gz', # TODO: Remove after 20241201, see https://crbug.com/351092787
        'src/third_party/tfhub_models', # TODO: Remove after 20241211
        'src/tools/clang/crashreports', # TODO: Remove after 20260401
    ],
  },
  {
    # Ensure that we don't accidentally reference any .pyc files whose
    # corresponding .py files have since been deleted.
    # We could actually try to avoid generating .pyc files, crbug.com/500078.
    'name': 'remove_stale_pyc_files',
    'pattern': '.',
    'action': [
        'python3',
        'src/tools/remove_stale_pyc_files.py',
        'src/android_webview/tools',
        'src/build/android',
        'src/gpu/gles2_conform_support',
        'src/infra',
        'src/ppapi',
        'src/printing',
        'src/third_party/blink/renderer/build/scripts',
        'src/third_party/blink/tools',  # See http://crbug.com/625877.
        'src/third_party/catapult',
        'src/third_party/mako', # Some failures triggered by crrev.com/c/3686969
        'src/tools',
    ],
  },
  {
    # Case-insensitivity for the Win SDK. Must run before win_toolchain below.
    'name': 'ciopfs_linux',
    'pattern': '.',
    'condition': 'checkout_win and host_os == "linux"',
    'action': [ 'python3',
                'src/third_party/depot_tools/download_from_google_storage.py',
                '--no_resume',
                '--bucket', 'chromium-browser-clang/ciopfs',
                '-s', 'src/build/ciopfs.sha1',
    ]
  },
  {
    # Update the Windows toolchain if necessary.  Must run before 'clang' below.
    'name': 'win_toolchain',
    'pattern': '.',
    'condition': 'checkout_win',
    'action': ['python3', 'src/build/vs_toolchain.py', 'update', '--force'],
  },
  {
    # Update the Mac toolchain if necessary.
    'name': 'mac_toolchain',
    'pattern': '.',
    'condition': 'checkout_mac or checkout_ios',
    'action': ['python3', 'src/build/mac_toolchain.py'],
  },
]

# Add any corresponding DEPS files from this list to chromium.exclusions in
# //testing/buildbot/trybot_analyze_config.json
# ctx: https://crbug.com/1201994
recursedeps = [
  # ANGLE manages DEPS that it also owns the build files for, such as dEQP.
  'src/third_party/angle',
  # Dawn manages DEPS for its copy of the WebGPU CTS as well as GLFW for which
  # it has build files.
  'src/third_party/dawn',
  'src/third_party/instrumented_libs',
  'src/third_party/openscreen/src',
  'src/third_party/devtools-frontend/src',
  # clank has its own DEPS file, does not need to be in trybot_analyze_config
  # since the roller does not run tests.
  'src/clank',
  'src/components/optimization_guide/internal',
  'src/ios_internal',
]
'''

PDFIUM_DEPS = '''
use_relative_paths = True

gclient_gn_args_file = 'build/config/gclient_args.gni'
gclient_gn_args = [
  'build_with_chromium',
  'checkout_android',
  'checkout_skia',
]

vars = {
  # Variable that can be used to support multiple build scenarios, like having
  # Chromium specific targets in a client project's GN file or sync dependencies
  # conditionally etc.
  # Standalone PDFium, by definition, does not build with Chromium.
  'build_with_chromium': False,

  # By default, we should check out everything needed to run on the main
  # pdfium waterfalls. This var can be also be set to 'small', in order to skip
  # things are not strictly needed to build pdfium for development purposes,
  # by adding the following line to the .gclient file inside a solutions entry:
  #      "custom_vars": { "checkout_configuration": "small" },
  # Similarly, this var can be set to 'minimal' to also skip the Skia and V8
  # checkouts for the smallest possible checkout, where some features will not
  # work.
  'checkout_configuration': 'default',

  # By default, don't check out android. Will be overridden by gclient
  # variables.
  # TODO(crbug.com/875037): Remove this once the bug in gclient is fixed.
  'checkout_android': False,

  # Pull in Android native toolchain dependencies, so we can build ARC++
  # support libraries.
  'checkout_android_native_support': 'checkout_android',

  'checkout_instrumented_libraries': 'checkout_linux and checkout_configuration != "small" and checkout_configuration != "minimal"',

  # Fetch the rust toolchain.
  #
  # Use a custom_vars section to enable it:
  # "custom_vars": {
  #   "checkout_rust": True,
  # }
  'checkout_rust': False,

  'checkout_skia': 'checkout_configuration != "minimal"',

  'checkout_testing_corpus': 'checkout_configuration != "small" and checkout_configuration != "minimal"',

  'checkout_v8': 'checkout_configuration != "minimal"',

  # condition to allowlist deps for non-git-source processing.
  'non_git_source': 'True',

  # Fetch configuration files required for the 'use_remoteexec' gn arg
  'download_remoteexec_cfg': False,
  # RBE instance to use for running remote builds
  'rbe_instance': Str('projects/rbe-chrome-untrusted/instances/default_instance'),
  # RBE project to download rewrapper config files for. Only needed if
  # different from the project used in 'rbe_instance'
  'rewrapper_cfg_project': Str(''),
  # reclient CIPD package
  'reclient_package': 'infra/rbe/client/',
  # reclient CIPD package version
  'reclient_version': 're_client_version:0.185.0.db415f21-gomaip',

  'chromium_git': 'https://chromium.googlesource.com',
  'pdfium_git': 'https://pdfium.googlesource.com',
  'skia_git': 'https://skia.googlesource.com',

  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling abseil
  # and whatever else without interference from each other.
  'abseil_revision': '14a5b78f39684f85a19690e63c722d6bb680f2cd',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling android_toolchain
  # and whatever else without interference from each other.
  'android_toolchain_version': 'KXOia11cm9lVdUdPlbGLu8sCz6Y4ey_HV2s8_8qeqhgC',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling build
  # and whatever else without interference from each other.
  'build_revision': '3c58005adf6e658f44946971a544d62815373e00',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling buildtools
  # and whatever else without interference from each other.
  'buildtools_revision': '24b075a4d7ea447126ff322e3e8bfecb78012b75',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling catapult
  # and whatever else without interference from each other.
  'catapult_revision': '0e07ae47d5e6323eba30a0f71cec48d84b2555a9',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling clang format
  # and whatever else without interference from each other.
  'clang_format_revision': 'c2725e0622e1a86d55f14514f2177a39efea4a0e',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling clang
  # and whatever else without interference from each other.
  'clang_revision': 'f0aeeca2d4eea4ade7308a31bc4023fc52614332',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling code_coverage
  # and whatever else without interference from each other.
  'code_coverage_revision': '9e4876df273e2b637b56d5e35815d27fed1dfce7',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling cpu_features
  # and whatever else without interference from each other.
  'cpu_features_revision': '936b9ab5515dead115606559502e3864958f7f6e',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling depot_tools
  # and whatever else without interference from each other.
  'depot_tools_revision': 'f8cc59a94be950b1f174d5f3373fdc0843c90036',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling dragonbox
  # and whatever else without interference from each other.
  'dragonbox_revision': '6c7c925b571d54486b9ffae8d9d18a822801cbda',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling fast_float
  # and whatever else without interference from each other.
  'fast_float_revision': 'cb1d42aaa1e14b09e1452cfdef373d051b8c02a4',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling fp16
  # and whatever else without interference from each other.
  'fp16_revision': '3d2de1816307bac63c16a297e8c4dc501b4076df',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling freetype
  # and whatever else without interference from each other.
  'freetype_revision': 'fc9cc5038e05edceec3d0f605415540ac76163e9',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling GN CIPD package version
  # and whatever else without interference from each other.
  'gn_version': 'git_revision:07d3c6f4dc290fae5ca6152ebcb37d6815c411ab',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling goldctl CIPD package version
  # and whatever else without interference from each other.
  'goldctl_version': 'git_revision:68c457e302c11fee68a2f59393f46a224e20fa10',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling gtest
  # and whatever else without interference from each other.
  'gtest_revision': 'b2b9072ecbe874f5937054653ef8f2731eb0f010',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling highway
  # and whatever else without interference from each other.
  'highway_revision': '84379d1c73de9681b54fbe1c035a23c7bd5d272d',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling icu
  # and whatever else without interference from each other.
  'icu_revision': 'f27805b7d7d8618fa73ce89e9d28e0a8b2216fec',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling instrumented_lib
  # and whatever else without interference from each other.
  'instrumented_lib_revision': '69015643b3f68dbd438c010439c59adc52cac808',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling jinja2
  # and whatever else without interference from each other.
  'jinja2_revision': 'c3027d884967773057bf74b957e3fea87e5df4d7',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling jpeg_turbo
  # and whatever else without interference from each other.
  'jpeg_turbo_revision': 'e14cbfaa85529d47f9f55b0f104a579c1061f9ad',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling libc++
  # and whatever else without interference from each other.
  # If you change this, also update the libc++ revision in
  # //buildtools/deps_revisions.gni.
  'libcxx_revision': 'e586acfc743b471da9bb3989246f77eafd356476',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling libc++abi
  # and whatever else without interference from each other.
  'libcxxabi_revision': 'a02fa0058d8d52aca049868d229808a3e5dadbad',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling libpng
  # and whatever else without interference from each other.
  'libpng_revision': 'a79803bfd1683fb3934e736d9353c82e4675228e',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling libunwind
  # and whatever else without interference from each other.
  'libunwind_revision': '14b9dee79c12dd8cdd3d662ee0be49200c6d9b71',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling llvm-libc
  # and whatever else without interference from each other.
  'llvm_libc_revision': '006672b9b6481bae04b9428100ed0486ab99f452',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling markupsafe
  # and whatever else without interference from each other.
  'markupsafe_revision': '4256084ae14175d38a3ff7d739dca83ae49ccec6',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling nasm_source
  # and whatever else without interference from each other.
  'nasm_source_revision': 'e2c93c34982b286b27ce8b56dd7159e0b90869a2',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling Ninja CIPD package version
  # and whatever else without interference from each other.
  'ninja_version': 'version:3@1.12.1.chromium.4',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling partition_allocator
  # and whatever else without interference from each other.
  'partition_allocator_revision': '2f4069fb8d7afbd9e3c078a14a5bab8280a68cf6',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling pdfium_tests
  # and whatever else without interference from each other.
  'pdfium_tests_revision': 'c851e3dd320446e4e9b97c65edd8a0eed4ecb55f',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling result_adapter_revision
  # and whatever else without interference from each other.
  'result_adapter_revision': 'git_revision:5fb3ca203842fd691cab615453f8e5a14302a1d8',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling rust
  # and whatever else without interference from each other.
  'rust_revision': '84c4bbe80381e6f098b793784ae291a24d981b56',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling simdutf
  # and whatever else without interference from each other.
  'simdutf_revision': 'acd71a451c1bcb808b7c3a77e0242052909e381e',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling siso
  # and whatever else without interference from each other.
  'siso_version': 'git_revision:0915813c4c786240e12d03aa3018c02bab4df14f',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling skia
  # and whatever else without interference from each other.
  'skia_revision': 'a9e9573a3f1b6afcd7a323bd663482a9834f5fb5',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling test_fonts
  # and whatever else without interference from each other.
  'test_fonts_revision': '7f51783942943e965cd56facf786544ccfc07713',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling testing_rust
  # and whatever else without interference from each other.
  'testing_rust_revision': '6712dc59f4a6c5626f391057cded3842700a17eb',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling tools_memory
  # and whatever else without interference from each other.
  'tools_memory_revision': '27e942fcc0c46109be1cf02d1257784115974c9f',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling tools_rust
  # and whatever else without interference from each other.
  'tools_rust_revision': '59ea51c9c56c2f1b9554d3d0dc5666ee50333fbd',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling tools_win_revision
  # and whatever else without interference from each other.
  'tools_win_revision': '24494b071e019a2baea4355d9870ffc5fc0bbafe',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling v8
  # and whatever else without interference from each other.
  'v8_revision': 'dd1717b9d27b2c78f45848bc953fdf0f0ccebc40',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling zlib
  # and whatever else without interference from each other.
  'zlib_revision': '63d7e16739d83e3a16c673692a348e52db1a3a11',
}

# Only these hosts are allowed for dependencies in this DEPS file.
# If you need to add a new host, and the new host is not in Chromium's DEPS
# file's allowed_hosts list, contact Chrome infrastructure team.
allowed_hosts = [
  'chromium.googlesource.com',
  'pdfium.googlesource.com',
  'skia.googlesource.com',

   # TODO(337061377): Move into a separate allowed gcs bucket list.
  'chromium-browser-clang',
]

deps = {
  'base/allocator/partition_allocator':
    Var('chromium_git') +
        '/chromium/src/base/allocator/partition_allocator.git@' +
        Var('partition_allocator_revision'),

  'build':
    Var('chromium_git') + '/chromium/src/build.git@' + Var('build_revision'),

  'buildtools':
    Var('chromium_git') + '/chromium/src/buildtools.git@' +
        Var('buildtools_revision'),

  'buildtools/linux64': {
    'packages': [
      {
        'package': 'gn/gn/linux-${{arch}}',
        'version': Var('gn_version'),
      }
    ],
    'dep_type': 'cipd',
    'condition': 'host_os == "linux"',
  },

  'buildtools/mac': {
    'packages': [
      {
        'package': 'gn/gn/mac-${{arch}}',
        'version': Var('gn_version'),
      }
    ],
    'dep_type': 'cipd',
    'condition': 'host_os == "mac"',
  },

  'buildtools/reclient': {
    'packages': [
      {
        'package': Var('reclient_package') + '${{platform}}',
        'version': Var('reclient_version'),
      }
    ],
    'dep_type': 'cipd',
  },

  'buildtools/win': {
    'packages': [
      {
        'package': 'gn/gn/windows-amd64',
        'version': Var('gn_version'),
      }
    ],
    'dep_type': 'cipd',
    'condition': 'host_os == "win"',
  },

  'testing/corpus': {
    'url': Var('pdfium_git') + '/pdfium_tests@' + Var('pdfium_tests_revision'),
    'condition': 'checkout_testing_corpus',
  },

  'testing/scripts/rust': {
    'url': Var('chromium_git') + '/chromium/src/testing/scripts/rust.git@' +
        Var('testing_rust_revision'),
    'condition': 'checkout_rust',
  },

  'third_party/abseil-cpp':
    Var('chromium_git') + '/chromium/src/third_party/abseil-cpp.git@' +
        Var('abseil_revision'),

  'third_party/android_toolchain/ndk': {
    'packages': [
      {
        'package': 'chromium/third_party/android_toolchain/android_toolchain',
        'version': Var('android_toolchain_version'),
      },
    ],
    'condition': 'checkout_android_native_support',
    'dep_type': 'cipd',
  },

  'third_party/catapult': {
    'url': Var('chromium_git') + '/catapult.git@' + Var('catapult_revision'),
    'condition': 'checkout_android',
  },

  'third_party/clang-format/script':
    Var('chromium_git') +
        '/external/github.com/llvm/llvm-project/clang/tools/clang-format.git@' +
        Var('clang_format_revision'),

  'third_party/cpu_features/src': {
    'url': Var('chromium_git') +
        '/external/github.com/google/cpu_features.git@' +
        Var('cpu_features_revision'),
    'condition': 'checkout_android',
  },

  'third_party/depot_tools':
    Var('chromium_git') + '/chromium/tools/depot_tools.git@' +
        Var('depot_tools_revision'),

  'third_party/dragonbox/src': {
    'url': Var('chromium_git') + '/external/github.com/jk-jeon/dragonbox.git@' +
        Var('dragonbox_revision'),
    'condition': 'checkout_v8',
  },

  'third_party/fast_float/src':
    Var('chromium_git') + '/external/github.com/fastfloat/fast_float.git@' +
        Var('fast_float_revision'),

  'third_party/fp16/src':
    Var('chromium_git') + '/external/github.com/Maratyszcza/FP16.git@' +
        Var('fp16_revision'),

  'third_party/freetype/src':
    Var('chromium_git') + '/chromium/src/third_party/freetype2.git@' +
        Var('freetype_revision'),

  'third_party/googletest/src':
    Var('chromium_git') + '/external/github.com/google/googletest.git@' +
        Var('gtest_revision'),

  'third_party/highway/src': {
    'url': Var('chromium_git') + '/external/github.com/google/highway.git@' +
        Var('highway_revision'),
    'condition': 'checkout_v8',
  },

  'third_party/icu':
    Var('chromium_git') + '/chromium/deps/icu.git@' + Var('icu_revision'),

  'third_party/instrumented_libs':
    Var('chromium_git') +
        '/chromium/third_party/instrumented_libraries.git@' +
        Var('instrumented_lib_revision'),

  'third_party/jinja2':
    Var('chromium_git') + '/chromium/src/third_party/jinja2.git@' +
        Var('jinja2_revision'),

  'third_party/libc++/src':
    Var('chromium_git') +
        '/external/github.com/llvm/llvm-project/libcxx.git@' +
        Var('libcxx_revision'),

  'third_party/libc++abi/src':
    Var('chromium_git') +
        '/external/github.com/llvm/llvm-project/libcxxabi.git@' +
        Var('libcxxabi_revision'),

  'third_party/libunwind/src': {
    'url': Var('chromium_git') +
        '/external/github.com/llvm/llvm-project/libunwind.git@' +
        Var('libunwind_revision'),
    'condition': 'checkout_android',
  },

  'third_party/libjpeg_turbo':
    Var('chromium_git') + '/chromium/deps/libjpeg_turbo.git@' +
        Var('jpeg_turbo_revision'),

  'third_party/libpng':
    Var('chromium_git') + '/chromium/src/third_party/libpng.git@' +
        Var('libpng_revision'),

  'third_party/llvm-build/Release+Asserts': {
    'dep_type': 'gcs',
    'bucket': 'chromium-browser-clang',
    'objects': [
      {
        'object_name': 'Linux_x64/clang-llvmorg-22-init-14273-gea10026b-1.tar.xz',
        'sha256sum': '1ef7b1d60fb433100c27b4552b44577ab86ef5394531d1fbebc237db64a893fd',
        'size_bytes': 56552908,
        'generation': 1762971374100697,
        'condition': '(host_os == "linux" or checkout_android) and non_git_source',
      },
      {
        'object_name': 'Linux_x64/llvmobjdump-llvmorg-22-init-14273-gea10026b-1.tar.xz',
        'sha256sum': 'ec1d88867045b8348659f7a8f677d12aa91d7d61a68603a82bad1926bf57c3b0',
        'size_bytes': 5723188,
        'generation': 1762971374436694,
        'condition': '((checkout_linux or checkout_mac or checkout_android) and host_os == "linux") and non_git_source',
      },
      {
        'object_name': 'Mac/clang-llvmorg-22-init-14273-gea10026b-1.tar.xz',
        'sha256sum': 'f266b79576d4fc0075e9380b68b8879ec2bc9617c973e7bdea694ec006f43636',
        'size_bytes': 54056416,
        'generation': 1762971376161293,
        'condition': 'host_os == "mac" and host_cpu == "x64"',
      },
      {
        'object_name': 'Mac/clang-mac-runtime-library-llvmorg-22-init-14273-gea10026b-1.tar.xz',
        'sha256sum': '6f2d61383a3c0ab28286e5a57b7e755eb14726bb9a73a7737b685488eae18b90',
        'size_bytes': 1010052,
        'generation': 1762971385382392,
        'condition': 'checkout_mac and not host_os == "mac"',
      },
      {
        'object_name': 'Mac/llvmobjdump-llvmorg-22-init-14273-gea10026b-1.tar.xz',
        'sha256sum': '9a282bf252e0c7ac88152844f347428e02970aa22941fb583439ce72134f0161',
        'size_bytes': 5607404,
        'generation': 1762971376526568,
        'condition': 'host_os == "mac" and host_cpu == "x64"',
      },
      {
        'object_name': 'Mac_arm64/clang-llvmorg-22-init-14273-gea10026b-1.tar.xz',
        'sha256sum': 'a7b7caf53f4e722234e85aecfdbb3eeb94608c37394672bebd074d6b2f300362',
        'size_bytes': 45184380,
        'generation': 1762971386895625,
        'condition': 'host_os == "mac" and host_cpu == "arm64"',
      },
      {
        'object_name': 'Mac_arm64/llvmobjdump-llvmorg-22-init-14273-gea10026b-1.tar.xz',
        'sha256sum': 'c5ee70e78ae5aa7a0d9b613ea5a8e21629438f12acb50bca0f7e18fae6abfe0a',
        'size_bytes': 5353832,
        'generation': 1762971387217357,
        'condition': 'host_os == "mac" and host_cpu == "arm64"',
      },
      {
        'object_name': 'Win/clang-llvmorg-22-init-14273-gea10026b-1.tar.xz',
        'sha256sum': '483b9b2809c3f53b9640e77d83ca6ab3017a0974979d242198abf23d99639e62',
        'size_bytes': 48337640,
        'generation': 1762971401378315,
        'condition': 'host_os == "win"',
      },
      {
        'object_name': 'Win/clang-win-runtime-library-llvmorg-22-init-14273-gea10026b-1.tar.xz',
        'sha256sum': 'd8b3310760c3a8f5dac4801583f7872601f4ba312742b0bf530f043ce6b6f36f',
        'size_bytes': 2520664,
        'generation': 1762971410370409,
        'condition': 'checkout_win and not host_os == "win"',
      },
      {
        'object_name': 'Win/llvmobjdump-llvmorg-22-init-14273-gea10026b-1.tar.xz',
        'sha256sum': '00c4dab7747534548e2111b3adbdbf9ef561887e18c7d6de4c7e273af799c190',
        'size_bytes': 5742908,
        'generation': 1762971401692156,
        'condition': '(checkout_linux or checkout_mac or checkout_android) and host_os == "win"',
      },
    ]
  },

  'third_party/llvm-libc/src':
    Var('chromium_git') + '/external/github.com/llvm/llvm-project/libc.git@' +
        Var('llvm_libc_revision'),

  'third_party/markupsafe':
    Var('chromium_git') + '/chromium/src/third_party/markupsafe.git@' +
        Var('markupsafe_revision'),

  'third_party/nasm':
    Var('chromium_git') + '/chromium/deps/nasm.git@' +
        Var('nasm_source_revision'),

  'third_party/ninja': {
    'packages': [
      {
        # https://chrome-infra-packages.appspot.com/p/infra/3pp/tools/ninja
        'package': 'infra/3pp/tools/ninja/${{platform}}',
        'version': Var('ninja_version'),
      }
    ],
    'dep_type': 'cipd',
  },

  'third_party/rust': {
    'url': Var('chromium_git') + '/chromium/src/third_party/rust@' +
        Var('rust_revision'),
    'condition': 'checkout_rust',
  },

  'third_party/rust-toolchain': {
    'dep_type': 'gcs',
    'bucket': 'chromium-browser-clang',
    'objects': [
      {
        'object_name': 'Linux_x64/rust-toolchain-11339a0ef5ed586bb7ea4f85a9b7287880caac3a-1-llvmorg-22-init-14273-gea10026b.tar.xz',
        'sha256sum': '15cf3019dd864ea64d63e5cf33de1ded76ace4f70cbb224812fccbf03b342096',
        'size_bytes': 140380848,
        'generation': 1762971367461755,
        'condition': 'host_os == "linux" and non_git_source',
      },
      {
        'object_name': 'Mac/rust-toolchain-11339a0ef5ed586bb7ea4f85a9b7287880caac3a-1-llvmorg-22-init-14273-gea10026b.tar.xz',
        'sha256sum': 'bb8be97e327b72f03f9caadae5557c7ce0ddac66e6dcc37cd3b8e65fca074e4b',
        'size_bytes': 134345832,
        'generation': 1762971369029231,
        'condition': 'host_os == "mac" and host_cpu == "x64"',
      },
      {
        'object_name': 'Mac_arm64/rust-toolchain-11339a0ef5ed586bb7ea4f85a9b7287880caac3a-1-llvmorg-22-init-14273-gea10026b.tar.xz',
        'sha256sum': '72b4421aa531822ba4b32533e6efae4e7c06eb15dcc32d1fb361023dd937c63c',
        'size_bytes': 121923464,
        'generation': 1762971370566007,
        'condition': 'host_os == "mac" and host_cpu == "arm64"',
      },
      {
        'object_name': 'Win/rust-toolchain-11339a0ef5ed586bb7ea4f85a9b7287880caac3a-1-llvmorg-22-init-14273-gea10026b.tar.xz',
        'sha256sum': '8cd2f0c4d312c0a7c127a8e6adf0a73ceecad316997e925fbebc1d855e95f6ba',
        'size_bytes': 197809928,
        'generation': 1762971372158285,
        'condition': 'host_os == "win"',
      },
    ],
  },

  'third_party/simdutf': {
    'url': Var('chromium_git') + '/chromium/src/third_party/simdutf@' +
        Var('simdutf_revision'),
    'condition': 'checkout_v8',
  },

  'third_party/siso/cipd': {
    'packages': [
      {
        'package': 'build/siso/${{platform}}',
        'version': Var('siso_version'),
      }
    ],
    'dep_type': 'cipd',
  },

  'third_party/skia': {
    'url': Var('skia_git') + '/skia.git@' + Var('skia_revision'),
    'condition': 'checkout_skia',
  },

  'third_party/test_fonts':
    Var('chromium_git') + '/chromium/src/third_party/test_fonts.git@' +
        Var('test_fonts_revision'),

  'third_party/zlib':
    Var('chromium_git') + '/chromium/src/third_party/zlib.git@' +
        Var('zlib_revision'),

  'tools/clang':
    Var('chromium_git') + '/chromium/src/tools/clang@' + Var('clang_revision'),

  'tools/code_coverage':
    Var('chromium_git') + '/chromium/src/tools/code_coverage.git@' +
        Var('code_coverage_revision'),

  'tools/memory':
    Var('chromium_git') + '/chromium/src/tools/memory@' +
        Var('tools_memory_revision'),

  'tools/resultdb': {
    'packages': [
      {
        'package': 'infra/tools/result_adapter/${{platform}}',
        'version': Var('result_adapter_revision'),
      },
    ],
    'dep_type': 'cipd',
  },

  'tools/rust': {
    'url': Var('chromium_git') + '/chromium/src/tools/rust@' +
        Var('tools_rust_revision'),
    'condition': 'checkout_rust',
  },

  'tools/skia_goldctl/linux': {
    'packages': [
      {
        'package': 'skia/tools/goldctl/linux-amd64',
        'version': Var('goldctl_version'),
      }
    ],
    'dep_type': 'cipd',
    'condition': 'checkout_linux',
  },

  'tools/skia_goldctl/mac_amd64': {
    'packages': [
      {
        'package': 'skia/tools/goldctl/mac-amd64',
        'version': Var('goldctl_version'),
      }
    ],
    'dep_type': 'cipd',
    'condition': 'checkout_mac',
  },

  'tools/skia_goldctl/mac_arm64': {
    'packages': [
      {
        'package': 'skia/tools/goldctl/mac-arm64',
        'version': Var('goldctl_version'),
      }
    ],
    'dep_type': 'cipd',
    'condition': 'checkout_mac',
  },

  'tools/skia_goldctl/win': {
    'packages': [
      {
        'package': 'skia/tools/goldctl/windows-amd64',
        'version': Var('goldctl_version'),
      }
    ],
    'dep_type': 'cipd',
    'condition': 'checkout_win',
  },

  'tools/win': {
    'url': Var('chromium_git') + '/chromium/src/tools/win.git@' +
        Var('tools_win_revision'),
    'condition': 'checkout_win',
  },

  'v8': {
    'url': Var('chromium_git') + '/v8/v8.git@' + Var('v8_revision'),
    'condition': 'checkout_v8',
  },

}

recursedeps = [
  'build',
  'buildtools',
  'third_party/instrumented_libs',
]

include_rules = [
  # Basic stuff that everyone can use.
  # Note: public is not here because core cannot depend on public.
  '+build/build_config.h',
  '+constants',
  '+testing',

  # Abseil is allowed by default, but some features are banned. See Chromium's
  # //styleguide/c++/c++-features.md.
  '+third_party/abseil-cpp',
  '-third_party/abseil-cpp/absl/algorithm/container.h',
  '-third_party/abseil-cpp/absl/base/attributes.h',
  '-third_party/abseil-cpp/absl/base/no_destructor.h',
  '-third_party/abseil-cpp/absl/base/nullability.h',
  '-third_party/abseil-cpp/absl/container/btree_map.h',
  '-third_party/abseil-cpp/absl/container/btree_set.h',
  '-third_party/abseil-cpp/absl/flags',
  '-third_party/abseil-cpp/absl/functional/any_invocable.h',
  '-third_party/abseil-cpp/absl/functional/bind_front.h',
  '-third_party/abseil-cpp/absl/functional/function_ref.h',
  '-third_party/abseil-cpp/absl/hash',
  '-third_party/abseil-cpp/absl/log',
  '-third_party/abseil-cpp/absl/random',
  '-third_party/abseil-cpp/absl/status/statusor.h',
  '-third_party/abseil-cpp/absl/strings',
  '+third_party/abseil-cpp/absl/strings/ascii.h',
  '+third_party/abseil-cpp/absl/strings/cord.h',
  '+third_party/abseil-cpp/absl/strings/str_format.h',
  '-third_party/abseil-cpp/absl/synchronization',
  '-third_party/abseil-cpp/absl/time',
  '-third_party/abseil-cpp/absl/types/any.h',
  '-third_party/abseil-cpp/absl/types/optional.h',
  '-third_party/abseil-cpp/absl/types/span.h',
  '-third_party/abseil-cpp/absl/types/variant.h',
  '-third_party/abseil-cpp/absl/utility/utility.h',
]

specific_include_rules = {
  # Allow embedder tests to use public APIs.
  '(.*embeddertest\.cpp)': [
    '+public',
  ]
}

hooks = [
  {
    # Ensure that the DEPS'd "depot_tools" has its self-update capability
    # disabled.
    'name': 'disable_depot_tools_selfupdate',
    'pattern': '.',
    'action': [ 'python3',
                'third_party/depot_tools/update_depot_tools_toggle.py',
                '--disable',
    ],
  },
  {
    # Case-insensitivity for the Win SDK. Must run before win_toolchain below.
    'name': 'ciopfs_linux',
    'pattern': '.',
    'condition': 'checkout_win and host_os == "linux"',
    'action': [ 'python3',
                'third_party/depot_tools/download_from_google_storage.py',
                '--no_resume',
                '--bucket', 'chromium-browser-clang/ciopfs',
                '-s', 'build/ciopfs.sha1',
    ]
  },
  {
    # Update the Windows toolchain if necessary.  Must run before 'clang' below.
    'name': 'win_toolchain',
    'pattern': '.',
    'condition': 'checkout_win',
    'action': ['python3', 'build/vs_toolchain.py', 'update', '--force'],
  },
  {
    # Update the Mac toolchain if necessary.
    'name': 'mac_toolchain',
    'pattern': '.',
    'condition': 'checkout_mac',
    'action': ['python3', 'build/mac_toolchain.py'],
  },
  # Pull dsymutil binaries using checked-in hashes.
  {
    'name': 'dsymutil_mac_arm64',
    'pattern': '.',
    'condition': 'host_os == "mac" and host_cpu == "arm64"',
    'action': [ 'python3',
                'third_party/depot_tools/download_from_google_storage.py',
                '--no_resume',
                '--bucket', 'chromium-browser-clang',
                '-s', 'tools/clang/dsymutil/bin/dsymutil.arm64.sha1',
                '-o', 'tools/clang/dsymutil/bin/dsymutil',
    ],
  },
  {
    'name': 'dsymutil_mac_x64',
    'pattern': '.',
    'condition': 'host_os == "mac" and host_cpu == "x64"',
    'action': [ 'python3',
                'third_party/depot_tools/download_from_google_storage.py',
                '--no_resume',
                '--bucket', 'chromium-browser-clang',
                '-s', 'tools/clang/dsymutil/bin/dsymutil.x64.sha1',
                '-o', 'tools/clang/dsymutil/bin/dsymutil',
    ],
  },
  {
    'name': 'test_fonts',
    'pattern': '.',
    'action': [ 'python3',
                'third_party/depot_tools/download_from_google_storage.py',
                '--no_resume',
                '--extract',
                '--bucket', 'chromium-fonts',
                '-s', 'third_party/test_fonts/test_fonts.tar.gz.sha1',
    ],
  },
  {
    # Update LASTCHANGE.
    'name': 'lastchange',
    'pattern': '.',
    'action': ['python3', 'build/util/lastchange.py',
               '-o', 'build/util/LASTCHANGE'],
  },
  # Configure remote exec cfg files
  {
    # Use luci_auth if on windows and using chrome-untrusted project
    'name': 'download_and_configure_reclient_cfgs',
    'pattern': '.',
    'condition': 'download_remoteexec_cfg and host_os == "win"',
    'action': ['python3',
               'buildtools/reclient_cfgs/configure_reclient_cfgs.py',
               '--rbe_instance',
               Var('rbe_instance'),
               '--reproxy_cfg_template',
               'reproxy.cfg.template',
               '--rewrapper_cfg_project',
               Var('rewrapper_cfg_project'),
               '--use_luci_auth_credshelper',
               '--quiet',
               ],
  },  {
    'name': 'download_and_configure_reclient_cfgs',
    'pattern': '.',
    'condition': 'download_remoteexec_cfg and not host_os == "win"',
    'action': ['python3',
               'buildtools/reclient_cfgs/configure_reclient_cfgs.py',
               '--rbe_instance',
               Var('rbe_instance'),
               '--reproxy_cfg_template',
               'reproxy.cfg.template',
               '--rewrapper_cfg_project',
               Var('rewrapper_cfg_project'),
               '--quiet',
               ],
  },
  {
    'name': 'configure_reclient_cfgs',
    'pattern': '.',
    'condition': 'not download_remoteexec_cfg',
    'action': ['python3',
               'buildtools/reclient_cfgs/configure_reclient_cfgs.py',
               '--rbe_instance',
               Var('rbe_instance'),
               '--reproxy_cfg_template',
               'reproxy.cfg.template',
               '--rewrapper_cfg_project',
               Var('rewrapper_cfg_project'),
               '--skip_remoteexec_cfg_fetch',
               '--quiet',
               ],
  },
  # Configure Siso for developer builds.
  {
    'name': 'configure_siso',
    'pattern': '.',
    'action': ['python3',
               'build/config/siso/configure_siso.py',
               '--rbe_instance',
               Var('rbe_instance'),
               ],
  },
]
'''


class RollDepTest(unittest.TestCase):

  def testCipdDepsRolls(self):
    success, message = roll_dep(CHROMIUM_DEPS, PDFIUM_DEPS, 'gn_version')
    self.assertTrue(success)
    self.assertEqual(
        'CIPD gn_version: '
        'git_revision:5964f499767097d81dbe034e8b541c3988168073', message)

    success, message = roll_dep(CHROMIUM_DEPS, PDFIUM_DEPS, 'siso_version')
    self.assertTrue(success)
    self.assertEqual(
        'CIPD siso_version: '
        'git_revision:49dcca5d2be985d8ac6d512e59ee59e315264fb8', message)

  def testCipdDepsRollsNothingToRoll(self):
    success, message = roll_dep(CHROMIUM_DEPS, PDFIUM_DEPS,
                                'android_toolchain_version')
    self.assertTrue(success)
    self.assertEqual('Revisions are the same.', message)

    success, message = roll_dep(CHROMIUM_DEPS, PDFIUM_DEPS, 'ninja_version')
    self.assertTrue(success)
    self.assertEqual('Revisions are the same.', message)

    success, message = roll_dep(CHROMIUM_DEPS, PDFIUM_DEPS, 'reclient_version')
    self.assertTrue(success)
    self.assertEqual('Revisions are the same.', message)

    success, message = roll_dep(CHROMIUM_DEPS, PDFIUM_DEPS,
                                'result_adapter_revision')
    self.assertTrue(success)
    self.assertEqual('Revisions are the same.', message)

  def testGitDepsRollsToChromiumRevision(self):
    success, message = roll_dep(CHROMIUM_DEPS, PDFIUM_DEPS, 'gtest_revision')
    self.assertTrue(success)
    self.assertEqual(
        'roll-dep third_party/googletest/src '
        '--roll-to 4fe3307fb2d9f86d19777c7eb0e4809e9694dde7 '
        '--ignore-dirty-tree --no-log', message)

    success, message = roll_dep(CHROMIUM_DEPS, PDFIUM_DEPS, 'icu_revision')
    self.assertTrue(success)
    self.assertEqual(
        'roll-dep third_party/icu '
        '--roll-to a86a32e67b8d1384b33f8fa48c83a6079b86f8cd '
        '--ignore-dirty-tree --no-log', message)

    success, message = roll_dep(CHROMIUM_DEPS, PDFIUM_DEPS,
                                'jpeg_turbo_revision')
    self.assertTrue(success)
    self.assertEqual(
        'roll-dep third_party/libjpeg_turbo '
        '--roll-to 6383cf609c1f63c18af0f59b2738caa0c6c7e379 '
        '--ignore-dirty-tree --no-log', message)

    success, message = roll_dep(CHROMIUM_DEPS, PDFIUM_DEPS, 'libcxx_revision')
    self.assertTrue(success)
    self.assertEqual(
        'roll-dep third_party/libc++/src '
        '--roll-to c5dd8ade977af3a7441bbf99a2dcac2d5820e702 '
        '--ignore-dirty-tree --no-log', message)

    success, message = roll_dep(CHROMIUM_DEPS, PDFIUM_DEPS,
                                'libcxxabi_revision')
    self.assertTrue(success)
    self.assertEqual(
        'roll-dep third_party/libc++abi/src '
        '--roll-to 83a852080747b9a362e8f9e361366b7a601f302c '
        '--ignore-dirty-tree --no-log', message)

    success, message = roll_dep(CHROMIUM_DEPS, PDFIUM_DEPS,
                                'libunwind_revision')
    self.assertTrue(success)
    self.assertEqual(
        'roll-dep third_party/libunwind/src '
        '--roll-to 88fc07ed143a5b3bbf45d430b72a4617ee9e235f '
        '--ignore-dirty-tree --no-log', message)

    success, message = roll_dep(CHROMIUM_DEPS, PDFIUM_DEPS,
                                'llvm_libc_revision')
    self.assertTrue(success)
    self.assertEqual(
        'roll-dep third_party/llvm-libc/src '
        '--roll-to 03f822d2a88c8f68f6a92c5cb3e79ccc3002e8a9 '
        '--ignore-dirty-tree --no-log', message)

    success, message = roll_dep(CHROMIUM_DEPS, PDFIUM_DEPS,
                                'nasm_source_revision')
    self.assertTrue(success)
    self.assertEqual(
        'roll-dep third_party/nasm '
        '--roll-to af5eeeb054bebadfbb79c7bcd100a95e2ad4525f '
        '--ignore-dirty-tree --no-log', message)

    success, message = roll_dep(CHROMIUM_DEPS, PDFIUM_DEPS, 'v8_revision')
    self.assertTrue(success)
    self.assertEqual(
        'roll-dep v8 '
        '--roll-to 5bd40de12569247beb113659504906aa41e48959 '
        '--ignore-dirty-tree --no-log', message)

  def testGitDepsRollsToChromiumRevisionForFreeType(self):
    success, message = roll_dep(CHROMIUM_DEPS, PDFIUM_DEPS, 'freetype_revision')
    self.assertTrue(success)
    self.assertEqual(
        'third_party/freetype/roll-freetype.sh '
        '--roll-to 30e45abe939d7c2cbdf268f277c293400096868c '
        '--ignore-dirty-tree --no-log', message)

  def testGitDepsRollsToChromiumRevisionNothingToRoll(self):
    success, message = roll_dep(CHROMIUM_DEPS, PDFIUM_DEPS,
                                'clang_format_revision')
    self.assertTrue(success)
    self.assertEqual('Revisions are the same.', message)

    success, message = roll_dep(CHROMIUM_DEPS, PDFIUM_DEPS,
                                'cpu_features_revision')
    self.assertTrue(success)
    self.assertEqual('Revisions are the same.', message)

    success, message = roll_dep(CHROMIUM_DEPS, PDFIUM_DEPS,
                                'dragonbox_revision')
    self.assertTrue(success)
    self.assertEqual('Revisions are the same.', message)

    success, message = roll_dep(CHROMIUM_DEPS, PDFIUM_DEPS,
                                'fast_float_revision')
    self.assertTrue(success)
    self.assertEqual('Revisions are the same.', message)

    success, message = roll_dep(CHROMIUM_DEPS, PDFIUM_DEPS, 'fp16_revision')
    self.assertTrue(success)
    self.assertEqual('Revisions are the same.', message)

    success, message = roll_dep(CHROMIUM_DEPS, PDFIUM_DEPS, 'highway_revision')
    self.assertTrue(success)
    self.assertEqual('Revisions are the same.', message)

  def testGitDepsRollsToTipOfTree(self):
    success, message = roll_dep(CHROMIUM_DEPS, PDFIUM_DEPS, 'abseil_revision')
    self.assertTrue(success)
    self.assertEqual(
        'roll-dep third_party/abseil-cpp '
        '--ignore-dirty-tree --no-log', message)

    success, message = roll_dep(CHROMIUM_DEPS, PDFIUM_DEPS, 'build_revision')
    self.assertTrue(success)
    self.assertEqual('roll-dep build --ignore-dirty-tree --no-log', message)

    success, message = roll_dep(CHROMIUM_DEPS, PDFIUM_DEPS,
                                'buildtools_revision')
    self.assertTrue(success)
    self.assertEqual('roll-dep buildtools --ignore-dirty-tree --no-log',
                     message)

    success, message = roll_dep(CHROMIUM_DEPS, PDFIUM_DEPS, 'clang_revision')
    self.assertTrue(success)
    self.assertEqual('roll-dep tools/clang --ignore-dirty-tree --no-log',
                     message)

    success, message = roll_dep(CHROMIUM_DEPS, PDFIUM_DEPS,
                                'partition_allocator_revision')
    self.assertTrue(success)
    self.assertEqual(
        'roll-dep base/allocator/partition_allocator '
        '--ignore-dirty-tree --no-log', message)

    success, message = roll_dep(CHROMIUM_DEPS, PDFIUM_DEPS, 'rust_revision')
    self.assertTrue(success)
    self.assertEqual('roll-dep third_party/rust --ignore-dirty-tree --no-log',
                     message)

    success, message = roll_dep(CHROMIUM_DEPS, PDFIUM_DEPS,
                                'testing_rust_revision')
    self.assertTrue(success)
    self.assertEqual(
        'roll-dep testing/scripts/rust '
        '--ignore-dirty-tree --no-log', message)

    success, message = roll_dep(CHROMIUM_DEPS, PDFIUM_DEPS,
                                'tools_rust_revision')
    self.assertTrue(success)
    self.assertEqual('roll-dep tools/rust --ignore-dirty-tree --no-log',
                     message)

    success, message = roll_dep(CHROMIUM_DEPS, PDFIUM_DEPS,
                                'tools_win_revision')
    self.assertTrue(success)
    self.assertEqual('roll-dep tools/win --ignore-dirty-tree --no-log', message)

  def testInvalidEntry(self):
    success, message = roll_dep(CHROMIUM_DEPS, PDFIUM_DEPS, 'invalid')
    self.assertFalse(success)
    self.assertEqual('Entry "invalid" not found in PDFium DEPS.', message)

  def testUnsupportedEntry(self):
    success, message = roll_dep(CHROMIUM_DEPS, PDFIUM_DEPS,
                                'pdfium_tests_revision')
    self.assertFalse(success)
    self.assertEqual('Rolling pdfium_tests_revision is not supported.', message)

    success, message = roll_dep(CHROMIUM_DEPS, PDFIUM_DEPS,
                                'test_fonts_revision')
    self.assertFalse(success)
    self.assertEqual('Rolling test_fonts_revision is not supported.', message)


class RollAllDepsTest(unittest.TestCase):

  def testRollAll(self):
    EXPECTED_ALL = [
        'roll-dep third_party/abseil-cpp --ignore-dirty-tree --no-log',
        'roll-dep build --ignore-dirty-tree --no-log',
        'roll-dep buildtools --ignore-dirty-tree --no-log',
        'roll-dep tools/clang --ignore-dirty-tree --no-log',
        'third_party/freetype/roll-freetype.sh '
        '--roll-to 30e45abe939d7c2cbdf268f277c293400096868c '
        '--ignore-dirty-tree --no-log',
        'CIPD gn_version: '
        'git_revision:5964f499767097d81dbe034e8b541c3988168073',
        'roll-dep third_party/googletest/src '
        '--roll-to 4fe3307fb2d9f86d19777c7eb0e4809e9694dde7 '
        '--ignore-dirty-tree --no-log',
        'roll-dep third_party/icu '
        '--roll-to a86a32e67b8d1384b33f8fa48c83a6079b86f8cd '
        '--ignore-dirty-tree --no-log',
        'roll-dep third_party/libjpeg_turbo '
        '--roll-to 6383cf609c1f63c18af0f59b2738caa0c6c7e379 '
        '--ignore-dirty-tree --no-log',
        'roll-dep third_party/libc++abi/src '
        '--roll-to 83a852080747b9a362e8f9e361366b7a601f302c '
        '--ignore-dirty-tree --no-log',
        'roll-dep third_party/libc++/src '
        '--roll-to c5dd8ade977af3a7441bbf99a2dcac2d5820e702 '
        '--ignore-dirty-tree --no-log',
        'roll-dep third_party/libunwind/src '
        '--roll-to 88fc07ed143a5b3bbf45d430b72a4617ee9e235f '
        '--ignore-dirty-tree --no-log',
        'roll-dep third_party/llvm-libc/src '
        '--roll-to 03f822d2a88c8f68f6a92c5cb3e79ccc3002e8a9 '
        '--ignore-dirty-tree --no-log',
        'roll-dep third_party/nasm '
        '--roll-to af5eeeb054bebadfbb79c7bcd100a95e2ad4525f '
        '--ignore-dirty-tree --no-log',
        'roll-dep base/allocator/partition_allocator '
        '--ignore-dirty-tree --no-log',
        'roll-dep third_party/rust --ignore-dirty-tree --no-log',
        'CIPD siso_version: '
        'git_revision:49dcca5d2be985d8ac6d512e59ee59e315264fb8',
        'roll-dep third_party/skia '
        '--roll-to 83b0b9e6065a60617504aa1d6e6997c511c3afac '
        '--ignore-dirty-tree --no-log',
        'roll-dep testing/scripts/rust --ignore-dirty-tree --no-log',
        'roll-dep tools/rust --ignore-dirty-tree --no-log',
        'roll-dep tools/win --ignore-dirty-tree --no-log',
        'roll-dep v8 --roll-to 5bd40de12569247beb113659504906aa41e48959 '
        '--ignore-dirty-tree --no-log',
    ]

    success, message = roll_all_deps(CHROMIUM_DEPS, PDFIUM_DEPS)
    self.assertTrue(success)
    self.assertEqual(EXPECTED_ALL, message)


if __name__ == '__main__':
  unittest.main()

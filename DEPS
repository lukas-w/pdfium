use_relative_paths = True

gclient_gn_args_file = 'build/config/gclient_args.gni'
gclient_gn_args = [
  'android_ndk_version',
  'build_with_chromium',
  'checkout_android',
  'checkout_libpng',
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

  # Fetch the prebuilt binaries for llvm-cov and llvm-profdata. Needed to
  # process the raw profiles produced by instrumented targets (built with
  # the gn arg 'use_clang_coverage').
  'checkout_clang_coverage_tools': 'False',

  # Fetch clang-tidy into the same bin/ directory as the clang binary.
  'checkout_clang_tidy': 'False',

  # Fetch clangd into the same bin/ directory as the clang binary.
  'checkout_clangd': 'False',

  # Pull in Android native toolchain dependencies, so we can build ARC++
  # support libraries.
  'checkout_android_native_support': 'checkout_android',

  'checkout_instrumented_libraries': 'checkout_linux and checkout_configuration != "small" and checkout_configuration != "minimal"',

  'checkout_libpng': True,

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

  'chromium_git': 'https://chromium.googlesource.com',
  'pdfium_git': 'https://pdfium.googlesource.com',
  'skia_git': 'https://skia.googlesource.com',

  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling abseil
  # and whatever else without interference from each other.
  'abseil_revision': '56722f9eb6f40fb1a140caffd36e8f1f61b57aaf',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling android_ndk
  # and whatever else without interference from each other.
  'android_ndk_version': Str('2@30.0.16248370'),
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling brotli
  # and whatever else without interference from each other.
  'brotli_revision': '803ac71664ad62af1de39ff55e63ebbe07006fc8',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling build
  # and whatever else without interference from each other.
  'build_revision': '0c62137b8623a3fab2c6aa5d7c94f719b3aa31f6',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling buildtools
  # and whatever else without interference from each other.
  'buildtools_revision': 'd0d4c192e3c718016b3a0c5d44ccec632bccfe96',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling catapult
  # and whatever else without interference from each other.
  'catapult_revision': 'e1165abbf12a3d9ecf2d3151a404c930b5047894',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling clang format
  # and whatever else without interference from each other.
  'clang_format_revision': '9f796802e5d633c96dbe97e0b6683d5e62594f9f',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling clang
  # and whatever else without interference from each other.
  'clang_revision': '7ea1bdeeaf498a513c77b66667e56f6f41f9f0b3',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling code_coverage
  # and whatever else without interference from each other.
  'code_coverage_revision': '084f00efb99dc9a1726ed68cc2e05d25bc52802b',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling cpu_features
  # and whatever else without interference from each other.
  'cpu_features_revision': '81d13c49649f0714dd41fb56bb246398b6584085',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling cpython3_version
  # and whatever else without interference from each other.
  'cpython3_version': 'version:3@3.11.9.chromium.38',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling depot_tools
  # and whatever else without interference from each other.
  'depot_tools_revision': 'a03dfbf7f2d1cefa2795d81d59ca4f16c8616a8f',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling dragonbox
  # and whatever else without interference from each other.
  'dragonbox_revision': 'beeeef91cf6fef89a4d4ba5e95d47ca64ccb3a44',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling fast_float
  # and whatever else without interference from each other.
  'fast_float_revision': 'b0ab987b3dfdde13fa1915f65ef2a5c068d9208c',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling fp16
  # and whatever else without interference from each other.
  'fp16_revision': '782eea126dc5c755827be751a099eb01826175cf',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling freetype
  # and whatever else without interference from each other.
  'freetype_revision': '891f904e2e7b4289f615598cd4b69832f87d2f3a',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling GN CIPD package version
  # and whatever else without interference from each other.
  'gn_version': 'git_revision:841e072bef419ae1116477dc75e8c5a662c0212c',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling goldctl CIPD package version
  # and whatever else without interference from each other.
  'goldctl_version': 'git_revision:d528c7cb34ccc8dc701485b2ec6981044203bed5',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling gtest
  # and whatever else without interference from each other.
  'gtest_revision': '4fe3307fb2d9f86d19777c7eb0e4809e9694dde7',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling HarfBuzz
  # and whatever else without interference from each other.
  'harfbuzz_revision': 'ff36703b95013ec1d437e6038e1aa7c881275f4e',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling highway
  # and whatever else without interference from each other.
  'highway_revision': '2607d3b5b0113992fe84d3848859eae13b3b52c1',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling icu
  # and whatever else without interference from each other.
  'icu_revision': '5aa526207171ecadf31597f0980a7b7ad8872f33',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling instrumented_lib
  # and whatever else without interference from each other.
  'instrumented_lib_revision': '83128b60913a82a39d01055312687a27bb959b59',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling jinja2
  # and whatever else without interference from each other.
  'jinja2_revision': 'c3027d884967773057bf74b957e3fea87e5df4d7',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling jpeg_turbo
  # and whatever else without interference from each other.
  'jpeg_turbo_revision': '640f254ad0fa03f6b1f29f89b7dd9366f2f6e533',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling libc++
  # and whatever else without interference from each other.
  # If you change this, also update the libc++ revision in
  # //buildtools/deps_revisions.gni.
  'libcxx_revision': '9ba7713d3a0f1e179904c984bb626748b15e57d6',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling libc++abi
  # and whatever else without interference from each other.
  'libcxxabi_revision': '09351f6ec00c2b65f2d03585ae6da07098b024dc',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling libpng
  # and whatever else without interference from each other.
  'libpng_revision': '0179a807421cf96731cdfa10008bba04fda243c1',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling libunwind
  # and whatever else without interference from each other.
  'libunwind_revision': 'eb1ca4993b534b7d565daee144bbc7f9072b967b',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling libyuv
  # and whatever else without interference from each other.
  'libyuv_revision': '1e2e3a35993609da1d89ac170b907719d555f4b3',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling llvm-libc
  # and whatever else without interference from each other.
  'llvm_libc_revision': '14e51fd2cd7ec21294f18a3a0b2fc5339afea271',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling markupsafe
  # and whatever else without interference from each other.
  'markupsafe_revision': '4256084ae14175d38a3ff7d739dca83ae49ccec6',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling nasm_source
  # and whatever else without interference from each other.
  'nasm_source_revision': '525a09a813be0f75b646ee93fc2a31c27b87d722',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling Ninja CIPD package version
  # and whatever else without interference from each other.
  'ninja_version': 'version:3@1.12.1.chromium.4',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling partition_allocator
  # and whatever else without interference from each other.
  'partition_allocator_revision': '2b864a17731ee7ae5a994ea308178da1e444ba29',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling pdfium_tests
  # and whatever else without interference from each other.
  'pdfium_tests_revision': 'f7a1d03ba45e874b7d3cdf722959a2b8ba4dbfa5',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling result_adapter_revision
  # and whatever else without interference from each other.
  'result_adapter_revision': 'git_revision:5fb3ca203842fd691cab615453f8e5a14302a1d8',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling rust
  # and whatever else without interference from each other.
  'rust_revision': '79a8603b966b612119e3498bcf7e535dfa096c1c',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling simdutf
  # and whatever else without interference from each other.
  'simdutf_revision': 'f7356eed293f8208c40b3c1b344a50bd70971983',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling siso
  # and whatever else without interference from each other.
  'siso_version': 'git_revision:c574896b272b40aac6e44c228ee0b471ea7e3d7d',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling skia
  # and whatever else without interference from each other.
  'skia_revision': '0abfcf118530e0e46e7e466f6d028041af61581a',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling test_fonts
  # and whatever else without interference from each other.
  'test_fonts_revision': '7f51783942943e965cd56facf786544ccfc07713',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling testing_rust
  # and whatever else without interference from each other.
  'testing_rust_revision': '9022dfb9a7ba7ad28fe7798ed997ba57cd52335b',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling tools_memory
  # and whatever else without interference from each other.
  'tools_memory_revision': '3d4610fc0dd49053c0395bc2825dfc6ac72ab64e',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling tools_rust
  # and whatever else without interference from each other.
  'tools_rust_revision': 'b7b03f53fe7d9e6b781bd1837d92275279ce5794',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling tools_win_revision
  # and whatever else without interference from each other.
  'tools_win_revision': 'ae34cdd9867c5d2b185fadc9d4a9cc82bb741760',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling v8
  # and whatever else without interference from each other.
  'v8_revision': '0d546234ecf0af2032e958300ef8aeb14bc716c9',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling zlib
  # and whatever else without interference from each other.
  'zlib_revision': '456ae730017b9b4bd3371927abc82627fd51feef',
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
        'package': 'chromium/third_party/android_toolchain/android_toolchain/${{platform}}',
        'version': 'version:' + Var('android_ndk_version'),
      },
    ],
    'condition': 'checkout_android_native_support and '
                 '((host_os == "linux" and host_cpu == "x64") or '
                 '(host_os == "mac" and host_cpu == "arm64"))',
    'dep_type': 'cipd',
  },

  'third_party/brotli':
    Var('chromium_git') + '/chromium/src/third_party/brotli.git@' +
        Var('brotli_revision'),

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

  # Host platform package.
  'third_party/cpython3/host': {
    'packages': [
      {
        'package': 'infra/3pp/tools/cpython3/${{platform}}',
        'version': Var('cpython3_version'),
      },
    ],
    'condition': 'non_git_source',
    'dep_type': 'cipd',
  },

  # Always download Linux x64 package regardless of host OS for RBE workers.
  'third_party/cpython3/linux-amd64': {
    'packages': [
      {
        'package': 'infra/3pp/tools/cpython3/linux-amd64',
        'version': Var('cpython3_version'),
      },
    ],
    'condition': 'non_git_source',
    'dep_type': 'cipd',
  },

  'third_party/depot_tools':
    Var('chromium_git') + '/chromium/tools/depot_tools.git@' +
        Var('depot_tools_revision'),

  'third_party/dragonbox/src':
    Var('chromium_git') + '/external/github.com/jk-jeon/dragonbox.git@' +
        Var('dragonbox_revision'),

  'third_party/fast_float/src':
    Var('chromium_git') + '/external/github.com/fastfloat/fast_float.git@' +
        Var('fast_float_revision'),

  'third_party/fp16/src': {
    'url': Var('chromium_git') + '/external/github.com/Maratyszcza/FP16.git@' +
        Var('fp16_revision'),
    'condition': 'checkout_v8',
  },

  'third_party/freetype/src':
    Var('chromium_git') + '/chromium/src/third_party/freetype2.git@' +
        Var('freetype_revision'),

  'third_party/googletest/src':
    Var('chromium_git') + '/external/github.com/google/googletest.git@' +
        Var('gtest_revision'),

  'third_party/harfbuzz/src':
    Var('chromium_git') + '/external/github.com/harfbuzz/harfbuzz.git@' +
        Var('harfbuzz_revision'),

  'third_party/highway/src':
    Var('chromium_git') + '/external/github.com/google/highway.git@' +
        Var('highway_revision'),

  'third_party/icu':
    Var('chromium_git') + '/chromium/deps/icu.git@' + Var('icu_revision'),

  'third_party/instrumented_libs':
    Var('chromium_git') +
        '/chromium/third_party/instrumented_libraries.git@' +
        Var('instrumented_lib_revision'),

  'third_party/jinja2': {
    'url': Var('chromium_git') + '/chromium/src/third_party/jinja2.git@' +
        Var('jinja2_revision'),
    'condition': 'checkout_v8',
  },

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

  'third_party/libpng': {
    'url': Var('chromium_git') + '/chromium/src/third_party/libpng.git@' +
        Var('libpng_revision'),
    'condition': 'checkout_libpng',
  },

  'third_party/libyuv':
    Var('chromium_git') + '/libyuv/libyuv.git@' + Var('libyuv_revision'),

  'third_party/llvm-build/Release+Asserts': {
    'dep_type': 'gcs',
    'bucket': 'chromium-browser-clang',
    'objects': [
      {
        'object_name': 'Linux_x64/clang-android-runtime-library-llvmorg-24-init-14453-ge063be79-1.tar.xz',
        'sha256sum': 'dd03492cad72f223d7743c2f4af39c405f5bac7f7f342de09697a45b7108ce4e',
        'size_bytes': 6188284,
        'generation': 1792086693754706,
        'condition': 'checkout_android and non_git_source',
      },
      {
        'object_name': 'Linux_x64/clang-linux-runtime-library-llvmorg-24-init-14453-ge063be79-1.tar.xz',
        'sha256sum': '38338f434311fcd12a070757c23830d6076529f4dcbefebc9d094804dc4eca63',
        'size_bytes': 3145380,
        'generation': 1792086698521278,
        'condition': 'checkout_linux and not host_os == "linux"',
      },
      {
        'object_name': 'Linux_x64/clang-llvmorg-24-init-14453-ge063be79-1.tar.xz',
        'sha256sum': '0b76afc1a29b0583f7723e966b9b65a22b15d8fdcd38e7c567ae32343c68944d',
        'size_bytes': 58394648,
        'generation': 1792086691304470,
        'condition': 'host_os == "linux" and non_git_source',
      },
      {
        'object_name': 'Linux_x64/clang-tidy-llvmorg-24-init-14453-ge063be79-1.tar.xz',
        'sha256sum': 'cf2a7610086b45fb878f606709babd3cbd8e26ea97a087ea47929e4ee871b562',
        'size_bytes': 15213064,
        'generation': 1792086691338093,
        'condition': 'host_os == "linux" and checkout_clang_tidy and non_git_source',
      },
      {
        'object_name': 'Linux_x64/clangd-llvmorg-24-init-14453-ge063be79-1.tar.xz',
        'sha256sum': '5ef64ed6fdeab8dfd91f11db07c601ce5107fc9e61e9d7a18dcee97af42f03c6',
        'size_bytes': 15595776,
        'generation': 1792086691391942,
        'condition': 'host_os == "linux" and checkout_clangd and non_git_source',
      },
      {
        'object_name': 'Linux_x64/llvm-code-coverage-llvmorg-24-init-14453-ge063be79-1.tar.xz',
        'sha256sum': '756e08684179166571dd155471e0a9c4525223d30b45d776b3d75a4d53b820d3',
        'size_bytes': 2432768,
        'generation': 1792086691721951,
        'condition': 'host_os == "linux" and checkout_clang_coverage_tools and non_git_source',
      },
      {
        'object_name': 'Linux_x64/llvmobjdump-llvmorg-24-init-14453-ge063be79-1.tar.xz',
        'sha256sum': '2282c8a4bdc6e12242989525039a36e7eeb4335b22d39d3b5d624c38083881c5',
        'size_bytes': 6011380,
        'generation': 1792086691439732,
        'condition': '((checkout_linux or checkout_mac or checkout_ios or checkout_android) and host_os == "linux") and non_git_source',
      },
      {
        'object_name': 'Mac/clang-llvmorg-24-init-14453-ge063be79-1.tar.xz',
        'sha256sum': 'ae29ec360eda6e80e96f8638d368ecdeea93d19c18c527c525dc0932bd5243a4',
        'size_bytes': 54834700,
        'generation': 1792086700293656,
        'condition': 'host_os == "mac" and host_cpu == "x64"',
      },
      {
        'object_name': 'Mac/clang-mac-runtime-library-llvmorg-24-init-14453-ge063be79-1.tar.xz',
        'sha256sum': 'c18444eef64cf27a8913ce73ffa8fd92e6f41306a02bbc82d805aa0c521863e0',
        'size_bytes': 997164,
        'generation': 1792086707123823,
        'condition': '(checkout_mac or checkout_ios) and not host_os == "mac" and non_git_source',
      },
      {
        'object_name': 'Mac/clang-tidy-llvmorg-24-init-14453-ge063be79-1.tar.xz',
        'sha256sum': '3997f34e72dc28eb30af1b8af903ceb6b0f516db7821996297bf296793addc72',
        'size_bytes': 14437076,
        'generation': 1792086700400707,
        'condition': 'host_os == "mac" and host_cpu == "x64" and checkout_clang_tidy',
      },
      {
        'object_name': 'Mac/clangd-llvmorg-24-init-14453-ge063be79-1.tar.xz',
        'sha256sum': '0fc9c351ad3bc6c357cc53a57dad6f8ebf45ba9e38150012cb457812f2a1314f',
        'size_bytes': 15359244,
        'generation': 1792086700318835,
        'condition': 'host_os == "mac" and host_cpu == "x64" and checkout_clangd',
      },
      {
        'object_name': 'Mac/llvm-code-coverage-llvmorg-24-init-14453-ge063be79-1.tar.xz',
        'sha256sum': 'b8b2f099cb029ff36cce33c9983984f547d7da0d9c34c1b427bb1080a31b8047',
        'size_bytes': 2307268,
        'generation': 1792086700464636,
        'condition': 'host_os == "mac" and host_cpu == "x64" and checkout_clang_coverage_tools',
      },
      {
        'object_name': 'Mac/llvmobjdump-llvmorg-24-init-14453-ge063be79-1.tar.xz',
        'sha256sum': 'eae7800773f3955fd1dcc334d8779b9a232ca6ebd3a79a55be028bd8797b1197',
        'size_bytes': 5586688,
        'generation': 1792086700364844,
        'condition': 'host_os == "mac" and host_cpu == "x64"',
      },
      {
        'object_name': 'Mac_arm64/clang-llvmorg-24-init-14453-ge063be79-1.tar.xz',
        'sha256sum': '0a8d887534b3153c7f6860265dc7e075d420174521c60ccd52c2c243949ef468',
        'size_bytes': 44917408,
        'generation': 1792086708845653,
        'condition': 'host_os == "mac" and host_cpu == "arm64"',
      },
      {
        'object_name': 'Mac_arm64/clang-tidy-llvmorg-24-init-14453-ge063be79-1.tar.xz',
        'sha256sum': '1aee3a0078bbe3b840125f521d3e6d985de61531ee6686a0bef80abc6e6d6a78',
        'size_bytes': 12291060,
        'generation': 1792086708806950,
        'condition': 'host_os == "mac" and host_cpu == "arm64" and checkout_clang_tidy',
      },
      {
        'object_name': 'Mac_arm64/clangd-llvmorg-24-init-14453-ge063be79-1.tar.xz',
        'sha256sum': '799e034e1fdff7eb7d899329ab20bbeb96642234844c9e070797059bd6ce5ee9',
        'size_bytes': 12292508,
        'generation': 1792086709015693,
        'condition': 'host_os == "mac" and host_cpu == "arm64" and checkout_clangd',
      },
      {
        'object_name': 'Mac_arm64/llvm-code-coverage-llvmorg-24-init-14453-ge063be79-1.tar.xz',
        'sha256sum': 'a77bacfb3e6c08af7d51981d88fb4aa07e4fb797f37937071dd8657b7c2dea8e',
        'size_bytes': 1897140,
        'generation': 1792086709049271,
        'condition': 'host_os == "mac" and host_cpu == "arm64" and checkout_clang_coverage_tools',
      },
      {
        'object_name': 'Mac_arm64/llvmobjdump-llvmorg-24-init-14453-ge063be79-1.tar.xz',
        'sha256sum': '8d15119ffd7e9d52e86676bfdcfd8159506f0ba6c4b292ae4112cbf15e629d7d',
        'size_bytes': 5269524,
        'generation': 1792086709009144,
        'condition': 'host_os == "mac" and host_cpu == "arm64"',
      },
      {
        'object_name': 'Win/clang-llvmorg-24-init-14453-ge063be79-1.tar.xz',
        'sha256sum': '08abce6e1d01c2fff9b25a09e36d9976296f27842196f96411b8220364481818',
        'size_bytes': 52783904,
        'generation': 1792086718487613,
        'condition': 'host_os == "win"',
      },
      {
        'object_name': 'Win/clang-tidy-llvmorg-24-init-14453-ge063be79-1.tar.xz',
        'sha256sum': 'ee25e107e6da91254780b0e9b6fb34198dc03636330c5564a7786e6450916d45',
        'size_bytes': 15436080,
        'generation': 1792086718412245,
        'condition': 'host_os == "win" and checkout_clang_tidy',
      },
      {
        'object_name': 'Win/clang-win-runtime-library-llvmorg-24-init-14453-ge063be79-1.tar.xz',
        'sha256sum': 'a156f6fa1f149e45faa683f2b2d1cc010df1e29912b3cb0a0dd3e3d9186c513b',
        'size_bytes': 2707764,
        'generation': 1792086725161419,
        'condition': 'checkout_win and not host_os == "win"',
      },
      {
        'object_name': 'Win/clangd-llvmorg-24-init-14453-ge063be79-1.tar.xz',
        'sha256sum': '0db39ead1a1f49137570fd5e99ab7520262c2e6444900b551862ae2cccdf6bb8',
        'size_bytes': 15910376,
        'generation': 1792086718402867,
       'condition': 'host_os == "win" and checkout_clangd',
      },
      {
        'object_name': 'Win/llvm-code-coverage-llvmorg-24-init-14453-ge063be79-1.tar.xz',
        'sha256sum': '3cfd8304148bd6bdd86a99f4e5f01ea2b4fbcbf4f35e29a2ca3cb235cb88c393',
        'size_bytes': 2604364,
        'generation': 1792086718581883,
        'condition': 'host_os == "win" and checkout_clang_coverage_tools',
      },
      {
        'object_name': 'Win/llvmobjdump-llvmorg-24-init-14453-ge063be79-1.tar.xz',
        'sha256sum': '3b58242027a9e86114d05444e9e70a9ed497a6a9e9c14d1b86c355e140503a53',
        'size_bytes': 6123208,
        'generation': 1792086718463583,
        'condition': '(checkout_linux or checkout_mac or checkout_android) and host_os == "win"',
      },
    ]
  },

  'third_party/llvm-libc/src':
    Var('chromium_git') + '/external/github.com/llvm/llvm-project/libc.git@' +
        Var('llvm_libc_revision'),

  'third_party/llvm-libclang': {
    'dep_type': 'gcs',
    'bucket': 'chromium-browser-clang',
    'objects': [
      {
        'object_name': 'Linux_x64/rust-libclang-8a9dd7abdf013ec10cfeaaa3685feba18ef1c90c-6-llvmorg-24-init-14453-ge063be79.tar.xz',
        'sha256sum': '693fd176a27508efb6cbc06b7db9c64abfc19497664dbaf4a9f4d489b8b4239a',
        'size_bytes': 23021680,
        'generation': 1793195408791425,
        'condition': 'host_os == "linux" and non_git_source',
      },
      {
        'object_name': 'Mac/rust-libclang-8a9dd7abdf013ec10cfeaaa3685feba18ef1c90c-6-llvmorg-24-init-14453-ge063be79.tar.xz',
        'sha256sum': 'b08bf161b5acaba2fe39d9a8089f8134417bfaef33531b6e4e84365e7c828bfe',
        'size_bytes': 23159048,
        'generation': 1793195412269519,
        'condition': 'host_os == "mac" and host_cpu == "x64"',
      },
      {
        'object_name': 'Mac_arm64/rust-libclang-8a9dd7abdf013ec10cfeaaa3685feba18ef1c90c-6-llvmorg-24-init-14453-ge063be79.tar.xz',
        'sha256sum': 'aa01ed55d80721dc53acf6fb604113037ba5142f3668ce7f7a02427b787cfea5',
        'size_bytes': 20842496,
        'generation': 1793195415799242,
        'condition': 'host_os == "mac" and host_cpu == "arm64"',
      },
      {
        'object_name': 'Win/rust-libclang-8a9dd7abdf013ec10cfeaaa3685feba18ef1c90c-6-llvmorg-24-init-14453-ge063be79.tar.xz',
        'sha256sum': '38438c49085b1770020713fc981bf63a6e7d6398d8c1d7f094ec509347d384e8',
        'size_bytes': 21764276,
        'generation': 1793195419458813,
        'condition': 'host_os == "win"',
      },
    ],
    'condition': 'checkout_v8',
  },

  'third_party/markupsafe': {
    'url': Var('chromium_git') + '/chromium/src/third_party/markupsafe.git@' +
        Var('markupsafe_revision'),
    'condition': 'checkout_v8',
  },

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
        'object_name': 'Linux_x64/rust-toolchain-8a9dd7abdf013ec10cfeaaa3685feba18ef1c90c-6-llvmorg-24-init-14453-ge063be79.tar.xz',
        'sha256sum': '4b4f64d5e82e845099b6ff63c61df311f8202183f1996ca247e20db6d9cda29c',
        'size_bytes': 287835700,
        'generation': 1793195406905214,
        'condition': 'host_os == "linux" and non_git_source',
      },
      {
        'object_name': 'Mac/rust-toolchain-8a9dd7abdf013ec10cfeaaa3685feba18ef1c90c-6-llvmorg-24-init-14453-ge063be79.tar.xz',
        'sha256sum': '7bd89fb1973d32d0231a1ffcd48851d629986763695c43ba6e8d8182361c538f',
        'size_bytes': 274877668,
        'generation': 1793195410604331,
        'condition': 'host_os == "mac" and host_cpu == "x64"',
      },
      {
        'object_name': 'Mac_arm64/rust-toolchain-8a9dd7abdf013ec10cfeaaa3685feba18ef1c90c-6-llvmorg-24-init-14453-ge063be79.tar.xz',
        'sha256sum': '7d1680ebf1d9ea133cd34d5f229c8491fbdbf9fe0da24171b686c8ecb8a4cad8',
        'size_bytes': 258546192,
        'generation': 1793195414074050,
        'condition': 'host_os == "mac" and host_cpu == "arm64"',
      },
      {
        'object_name': 'Win/rust-toolchain-8a9dd7abdf013ec10cfeaaa3685feba18ef1c90c-6-llvmorg-24-init-14453-ge063be79.tar.xz',
        'sha256sum': '9e519f453f7fea971f531b7e4c5a8e686081b9647d7660deb0641efac306840f',
        'size_bytes': 426696244,
        'generation': 1793195417684260,
        'condition': 'host_os == "win"',
      },
    ],
  },

  'third_party/simdutf': {
    'url': Var('chromium_git') + '/chromium/src/third_party/simdutf@' +
        Var('simdutf_revision'),
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
  r'(.*embeddertest\.cpp)': [
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

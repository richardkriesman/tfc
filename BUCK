cxx_library(
    name='xxhash',
    header_namespace='xxhash',
    srcs=['lib/xxhash/xxhash.c'],
    exported_headers=subdir_glob([
        ('lib/xxhash', 'xxhash.h'),
    ]),
    visibility=['PUBLIC']
)

cxx_library(
    name='portable_endian',
    header_namespace='portable_endian',
    exported_headers=subdir_glob([
        ('lib/portable_endian', 'portable_endian.h')
    ]),
    compiler_flags = ['-std=c++11'],
    visibility=['PUBLIC']
)

cxx_library(
  name = 'libtfc',
  header_namespace = 'libtfc',
  srcs = glob([
    'libtfc/src/**/*.cpp'
  ]),
  exported_headers = subdir_glob([
    ('libtfc/extern', '**/*.h')
  ]),
  deps = [':xxhash', ':portable_endian'],
  compiler_flags = ['-std=c++11'],
  visibility = ['PUBLIC']
)

cxx_library(
  name = 'tasker',
  header_namespace = 'tasker',
  srcs = glob([
    'tasker/src/**/*.cpp'
  ]),
  exported_headers = subdir_glob([
    ('tasker/extern', '**/*.h')
  ]),
  compiler_flags = ['-std=c++11'],
  visibility = ['PUBLIC']
)

cxx_binary(
  name = 'tfc',
  header_namespace = 'tfc',
  srcs = glob([
    'tfc/src/**/*.cpp',
  ]),
  headers = subdir_glob([
    ('tfc/include', '**/*.h'),
    ('tfc/include', '**/*.hpp'),
  ]),
  deps = [
    ':libtfc',
    ':tasker'
  ],
  compiler_flags = ['-std=c++11'],
  linker_flags = ['-pthread'],
  visibility = ['PUBLIC']
)

cxx_binary(
  name = 'tfc-debug',
  header_namespace = 'tfc',
  srcs = glob([
    'tfc/src/**/*.cpp',
  ]),
  headers = subdir_glob([
    ('tfc/include', '**/*.h'),
    ('tfc/include', '**/*.hpp'),
  ]),
  deps = [
    ':libtfc',
    ':tasker'
  ],
  compiler_flags = ['-g', '-std=c++11'],
  linker_flags = ['-pthread'],
  visibility = ['PUBLIC']
)

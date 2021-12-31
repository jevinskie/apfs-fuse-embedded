set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(ENABLE_STATIC_LIB ON)
add_subdirectory(bzip2)
target_include_directories(bz2_static PUBLIC bzip2)

set(EXPECTED_BUILD_TESTS OFF)
add_subdirectory(expected)

set(LZFSE_BUNDLE_MODE ON)
add_subdirectory(lzfse)
target_include_directories(lzfse PUBLIC lzfse/src)

set(BUILD_TESTING OFF)
set(ENABLE_BENCHMARKS OFF)
add_subdirectory(visit)

set(ZLIB_COMPAT ON)
add_subdirectory(zlib-ng)
target_include_directories(zlib PUBLIC ${zlib_BINARY_DIR})
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(ENABLE_STATIC_LIB ON)
add_subdirectory(bzip2)
target_include_directories(bz2_static PUBLIC bzip2)
target_compile_definitions(bz2_static PRIVATE BZ2_DISABLE_FP BZ_DEBUG=0)
target_compile_options(bz2_static PRIVATE -Wno-sign-conversion -Wno-implicit-int-conversion)

set(expected_BUILD_TESTS OFF)
add_subdirectory(expected)

set(LZFSE_BUNDLE_MODE ON)
add_subdirectory(lzfse)
target_include_directories(lzfse PUBLIC lzfse/src)

add_subdirectory(miniz)
target_compile_options(miniz PRIVATE -fPIC)
target_compile_definitions(miniz PUBLIC MINIZ_NO_TIME)

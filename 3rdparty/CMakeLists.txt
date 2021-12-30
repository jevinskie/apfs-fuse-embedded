set(ENABLE_STATIC_LIB ON)
add_subdirectory(bzip2)
target_include_directories(bz2_static PUBLIC bzip2)

add_subdirectory(lzfse)
target_include_directories(lzfse PUBLIC lzfse/src)

add_subdirectory(zlib)
target_include_directories(zlibstatic PUBLIC zlib)


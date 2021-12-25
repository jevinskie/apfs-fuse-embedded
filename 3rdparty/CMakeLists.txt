set(ENABLE_STATIC_LIB ON)
add_subdirectory(bzip2)

add_subdirectory(lzfse)
target_include_directories(lzfse PUBLIC lzfse/src)

add_subdirectory(zlib)

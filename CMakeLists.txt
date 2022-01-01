cmake_minimum_required(VERSION 3.22)
# set(CMAKE_C_COMPILER "clang")
# set(CMAKE_CXX_COMPILER "clang++")

cmake_policy(VERSION 3.22)

project(Apfs)

option(USE_FUSE3 "Use the FUSE 3 library (required on 32-bit systems)" ON)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries")

option(FORCE_COLORED_OUTPUT "Always produce ANSI-colored output (GNU/Clang only)." ON)

if (${FORCE_COLORED_OUTPUT})
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
       add_compile_options(-fdiagnostics-color=always)
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
       add_compile_options(-fcolor-diagnostics)
    endif ()
endif ()


# set(CMAKE_C_STANDARD 99)
# set(CMAKE_CXX_STANDARD 11)
# set(CMAKE_BUILD_TYPE Release)

# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -march=native -fsanitize=undefined")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -march=native -fsanitize=undefined")
# set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=undefined")

# include_directories(. 3rdparty/lzfse/src)

add_subdirectory(3rdparty)

set(APFS_SRC
	ApfsLib/Aes.cpp
	ApfsLib/Aes.h
	ApfsLib/AesXts.cpp
	ApfsLib/AesXts.h
	ApfsLib/ApfsContainer.cpp
	ApfsLib/ApfsContainer.h
	ApfsLib/ApfsDir.cpp
	ApfsLib/ApfsDir.h
	ApfsLib/ApfsNodeMapper.cpp
	ApfsLib/ApfsNodeMapper.h
	ApfsLib/ApfsNodeMapperBTree.cpp
	ApfsLib/ApfsNodeMapperBTree.h
	ApfsLib/ApfsVolume.cpp
	ApfsLib/ApfsVolume.h
	ApfsLib/BlockDumper.cpp
	ApfsLib/BlockDumper.h
	ApfsLib/BTree.cpp
	ApfsLib/BTree.h
	ApfsLib/CheckPointMap.cpp
	ApfsLib/CheckPointMap.h
	ApfsLib/Crc32.cpp
	ApfsLib/Crc32.h
	ApfsLib/Crypto.cpp
	ApfsLib/Crypto.h
	ApfsLib/Decmpfs.cpp
	ApfsLib/Decmpfs.h
	ApfsLib/Des.cpp
	ApfsLib/Des.h
	ApfsLib/Device.cpp
	ApfsLib/Device.h
	ApfsLib/DeviceDMG.cpp
	ApfsLib/DeviceDMG.h
	ApfsLib/DeviceLinux.cpp
	ApfsLib/DeviceLinux.h
	ApfsLib/DeviceMac.cpp
	ApfsLib/DeviceMac.h
	ApfsLib/DeviceSparseImage.cpp
	ApfsLib/DeviceSparseImage.h
	ApfsLib/DeviceWinFile.cpp
	ApfsLib/DeviceWinFile.h
	ApfsLib/DeviceWinPhys.cpp
	ApfsLib/DeviceWinPhys.h
	ApfsLib/DiskImageFile.cpp
	ApfsLib/DiskImageFile.h
	ApfsLib/DiskStruct.h
	ApfsLib/Endian.h
	ApfsLib/Global.h
	ApfsLib/GptPartitionMap.cpp
	ApfsLib/GptPartitionMap.h
	ApfsLib/KeyMgmt.cpp
	ApfsLib/KeyMgmt.h
	ApfsLib/PList.cpp
	ApfsLib/PList.h
	ApfsLib/Sha1.cpp
	ApfsLib/Sha1.h
	ApfsLib/Sha256.cpp
	ApfsLib/Sha256.h
	ApfsLib/TripleDes.cpp
	ApfsLib/TripleDes.h
	ApfsLib/Util.cpp
	ApfsLib/Util.h
	ApfsLib/Unicode.cpp
	ApfsLib/Unicode.h
)

add_library(apfs_shared SHARED ${APFS_SRC})
target_link_libraries(apfs_shared zlibstatic bz2_static lzfse expected)
target_compile_definitions(apfs_shared PUBLIC _FILE_OFFSET_BITS=64 _DARWIN_USE_64_BIT_INODE)
target_include_directories(apfs_shared PUBLIC .)
target_compile_options(apfs_shared PUBLIC -fPIC -stdlib=libc++)
target_link_options(apfs_shared PUBLIC -fPIC -stdlib=libc++ -fuse-ld=lld)


add_library(apfs_static STATIC ${APFS_SRC})
target_link_libraries(apfs_static zlibstatic bz2_static lzfse expected)
target_compile_definitions(apfs_static PUBLIC _FILE_OFFSET_BITS=64 _DARWIN_USE_64_BIT_INODE)
target_include_directories(apfs_static PUBLIC .)
target_compile_options(apfs_static PUBLIC -fPIC -stdlib=libc++)
target_link_options(apfs_static PUBLIC -fPIC -stdlib=libc++ -fuse-ld=lld)
target_compile_options(apfs_static PUBLIC -Oz -fvisibility=hidden -fvisibility-inlines-hidden -ffunction-sections -fdata-sections)
target_compile_options(apfs_static PUBLIC -flto=full -fwhole-program-vtables -fvirtual-function-elimination)
target_compile_options(apfs_static PUBLIC -fno-rtti -fno-exceptions -fno-unwind-tables -fno-asynchronous-unwind-tables)

set_property(TARGET apfs_shared apfs_static PROPERTY CXX_STANDARD 20)

if (BUILD_SHARED_LIBS)
	add_library(apfs ALIAS apfs_shared)
else()
	add_library(apfs ALIAS apfs_static)
endif()

add_executable(apfs-dump
	ApfsDump/Dumper.cpp
	ApfsDump/Dumper.h
	ApfsDump/Apfs.cpp)
target_link_libraries(apfs-dump apfs)
set_property(TARGET apfs-dump PROPERTY CXX_STANDARD 20)

add_executable(apfs-dump-quick
	ApfsDumpQuick/ApfsDumpQuick.cpp)
target_link_libraries(apfs-dump-quick apfs)
set_property(TARGET apfs-dump-quick PROPERTY CXX_STANDARD 20)

add_executable(apfs-fuse
	apfsfuse/ApfsFuse.cpp)
target_compile_definitions(apfs-fuse PRIVATE _FILE_OFFSET_BITS=64 _DARWIN_USE_64_BIT_INODE)
if (APPLE)
target_include_directories(apfs-fuse PRIVATE /usr/local/include/)
target_compile_definitions(apfs-fuse PRIVATE USE_FUSE2)
# link_directories(/usr/local/lib/)
target_link_libraries(apfs-fuse apfs /usr/local/lib/libfuse.dylib)
else()
if (USE_FUSE3)
target_link_libraries(apfs-fuse apfs fuse3)
else()
target_link_libraries(apfs-fuse apfs fuse)
target_compile_definitions(apfs-fuse PRIVATE USE_FUSE2)
endif()
endif()
set_property(TARGET apfs-fuse PROPERTY CXX_STANDARD 20)

add_executable(apfsutil ApfsUtil/ApfsUtil.cpp)
target_link_libraries(apfsutil apfs)
set_property(TARGET apfsutil PROPERTY CXX_STANDARD 20)

include(GNUInstallDirs)
install(TARGETS apfs-fuse RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
install(TARGETS apfsutil RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")

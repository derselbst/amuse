add_executable(amuse-mkqticon mkqticon.c)
target_link_libraries(amuse-mkqticon ${PNG_LIBRARIES} ${ZLIB_LIBRARIES})

macro(declare_qticon_target)
add_custom_command(OUTPUT ${amuse_BINARY_DIR}/Editor/platforms/freedesktop/mainicon_qt.bin
                   COMMAND $<TARGET_FILE:amuse-mkqticon>
                   ARGS ${amuse_BINARY_DIR}/Editor/platforms/freedesktop/mainicon_qt.bin
                   DEPENDS
                   ${amuse_SOURCE_DIR}/Editor/platforms/freedesktop/128x128/apps/amuse-gui.png
                   ${amuse_SOURCE_DIR}/Editor/platforms/freedesktop/64x64/apps/amuse-gui.png
                   ${amuse_SOURCE_DIR}/Editor/platforms/freedesktop/48x48/apps/amuse-gui.png
                   ${amuse_SOURCE_DIR}/Editor/platforms/freedesktop/32x32/apps/amuse-gui.png
                   ${amuse_SOURCE_DIR}/Editor/platforms/freedesktop/16x16/apps/amuse-gui.png
                   WORKING_DIRECTORY ${amuse_SOURCE_DIR}/Editor/platforms/freedesktop
                   COMMENT "Generating mainicon_qt.bin")

include(ExternalProject)
set(BINTOC_CMAKE_ARGS
    -DCMAKE_BUILD_TYPE=Release
    -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>)

if (CMAKE_TOOLCHAIN_FILE AND NOT CMAKE_CROSSCOMPILING)
  if (IS_ABSOLUTE "${CMAKE_TOOLCHAIN_FILE}")
    set(BINTOC_TOOLCHAIN_FILE "${CMAKE_TOOLCHAIN_FILE}")
  else ()
    set(BINTOC_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/${CMAKE_TOOLCHAIN_FILE}")
  endif ()
  list(APPEND BINTOC_CMAKE_ARGS -DCMAKE_TOOLCHAIN_FILE:PATH=${BINTOC_TOOLCHAIN_FILE})
endif ()

if (CMAKE_MAKE_PROGRAM)
  list(APPEND BINTOC_CMAKE_ARGS -DCMAKE_MAKE_PROGRAM:FILEPATH=${CMAKE_MAKE_PROGRAM})
endif ()

if (DEFINED VCPKG_TARGET_TRIPLET AND NOT "${VCPKG_TARGET_TRIPLET}" STREQUAL "" AND NOT CMAKE_CROSSCOMPILING)
  list(APPEND BINTOC_CMAKE_ARGS -DVCPKG_TARGET_TRIPLET:STRING=${VCPKG_TARGET_TRIPLET})
endif ()

ExternalProject_Add(bintoc
    SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/bintoc"
    CMAKE_ARGS ${BINTOC_CMAKE_ARGS}
    INSTALL_COMMAND ${CMAKE_COMMAND} --build . --config Release --target install)
include(${CMAKE_CURRENT_LIST_DIR}/bintoc/bintocHelpers.cmake)

bintoc(mainicon_qt.cpp ${amuse_BINARY_DIR}/Editor/platforms/freedesktop/mainicon_qt.bin MAINICON_QT)
endmacro()

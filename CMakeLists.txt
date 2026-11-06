cmake_minimum_required(VERSION 3.19)

project(Inkpot
    VERSION 1.1.1
    DESCRIPTION "Draw and edit pictures made of objects"
    HOMEPAGE_URL "https://github.com/Neklor/Inkpot"
    LANGUAGES CXX)

# Reverse-DNS application id. It ties the binary, the desktop entry, the icon
# and the AppStream data together and is what Flatpak and software centres
# identify the program by.
set(INKPOT_APP_ID "io.github.neklor.Inkpot")
set(INKPOT_AUTHOR "Christoph Krogmann")
set(INKPOT_COPYRIGHT_YEARS "2026")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

include(GNUInstallDirs)

find_package(Qt6 6.4 REQUIRED COMPONENTS Widgets PrintSupport)

# Optional: without the Linguist tools the program still builds, it just runs
# untranslated.
find_package(Qt6 6.4 QUIET COMPONENTS LinguistTools)

# HEIF and HEIC support is optional. The pkg-config file of libheif pulls in
# further packages that are not always installed, so the library is looked up
# directly.
find_path(HEIF_INCLUDE_DIR NAMES libheif/heif.h)
find_library(HEIF_LIBRARY NAMES heif)

if(HEIF_INCLUDE_DIR AND HEIF_LIBRARY)
    set(INKPOT_HAVE_HEIF ON)
    message(STATUS "libheif found - HEIF and HEIC pictures can be opened")
else()
    set(INKPOT_HAVE_HEIF OFF)
    message(STATUS
        "libheif not found - building without HEIF and HEIC support. "
        "On Debian and Ubuntu it comes with the libheif-dev package.")
endif()

qt_standard_project_setup()

set(INKPOT_SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindow.h
    src/CanvasScene.cpp
    src/CanvasScene.h
    src/CanvasView.cpp
    src/CanvasView.h
    src/ResizeHandle.cpp
    src/ResizeHandle.h
    src/StrokeItem.cpp
    src/StrokeItem.h
    src/ImageItem.cpp
    src/ImageItem.h
    src/TextItem.cpp
    src/TextItem.h
    src/NewCanvasDialog.cpp
    src/NewCanvasDialog.h
    src/ObjectListDock.cpp
    src/ObjectListDock.h
    src/ShapeItem.cpp
    src/ShapeItem.h
    src/ShortcutsDialog.cpp
    src/ShortcutsDialog.h
    src/Commands.cpp
    src/Commands.h
    src/FillItem.cpp
    src/FillItem.h
    src/FloodFill.cpp
    src/FloodFill.h
    src/Icons.cpp
    src/Icons.h
    src/HeifImage.cpp
    src/HeifImage.h
    src/InkObject.cpp
    src/InkObject.h
)

qt_add_executable(inkpot ${INKPOT_SOURCES})

target_include_directories(inkpot PRIVATE src)

target_compile_definitions(inkpot PRIVATE
    INKPOT_APP_ID="${INKPOT_APP_ID}"
    INKPOT_VERSION="${PROJECT_VERSION}"
    INKPOT_HOMEPAGE="${PROJECT_HOMEPAGE_URL}"
    INKPOT_AUTHOR="${INKPOT_AUTHOR}"
    INKPOT_COPYRIGHT_YEARS="${INKPOT_COPYRIGHT_YEARS}"
    INKPOT_TRANSLATIONS_DIR="${CMAKE_INSTALL_FULL_DATADIR}/inkpot/translations"
)

target_link_libraries(inkpot PRIVATE
    Qt6::Widgets
    Qt6::PrintSupport
)

if(INKPOT_HAVE_HEIF)
    target_compile_definitions(inkpot PRIVATE INKPOT_HAVE_HEIF)
    target_include_directories(inkpot PRIVATE ${HEIF_INCLUDE_DIR})
    target_link_libraries(inkpot PRIVATE ${HEIF_LIBRARY})
endif()

# ---------------------------------------------------------------- Translations

set(INKPOT_TS_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/inkpot_de.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/inkpot_es.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/inkpot_fr.ts
)

# Debian and Ubuntu split the Linguist tools: the programs come with
# qt6-l10n-tools, while the CMake package lives in qt6-tools-dev. Fall back to
# calling the programs directly so the smaller package is enough.
if(TARGET Qt6::qmake)
    get_target_property(_inkpot_qmake Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_inkpot_qt_bin "${_inkpot_qmake}" DIRECTORY)
endif()

find_program(INKPOT_LRELEASE
    NAMES lrelease-qt6 lrelease6 lrelease
    HINTS "${_inkpot_qt_bin}" "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_BINS}"
          /usr/lib/qt6/bin /usr/lib64/qt6/bin
)
find_program(INKPOT_LUPDATE
    NAMES lupdate-qt6 lupdate6 lupdate
    HINTS "${_inkpot_qt_bin}" "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_BINS}"
          /usr/lib/qt6/bin /usr/lib64/qt6/bin
)

set(INKPOT_QM_FILES "")

if(Qt6LinguistTools_FOUND)
    qt_add_lupdate(inkpot TS_FILES ${INKPOT_TS_FILES})
    qt_add_lrelease(inkpot
        TS_FILES ${INKPOT_TS_FILES}
        QM_FILES_OUTPUT_VARIABLE INKPOT_QM_FILES
    )
elseif(INKPOT_LRELEASE)
    foreach(_ts IN LISTS INKPOT_TS_FILES)
        get_filename_component(_name "${_ts}" NAME_WE)
        set(_qm "${CMAKE_CURRENT_BINARY_DIR}/${_name}.qm")
        add_custom_command(
            OUTPUT "${_qm}"
            COMMAND "${INKPOT_LRELEASE}" -silent "${_ts}" -qm "${_qm}"
            DEPENDS "${_ts}"
            COMMENT "Compiling ${_name}.qm"
            VERBATIM
        )
        list(APPEND INKPOT_QM_FILES "${_qm}")
    endforeach()

    add_custom_target(inkpot_translations ALL DEPENDS ${INKPOT_QM_FILES})
    add_dependencies(inkpot inkpot_translations)

    if(INKPOT_LUPDATE)
        add_custom_target(update_translations
            COMMAND "${INKPOT_LUPDATE}" -no-obsolete -locations relative
                    ${INKPOT_SOURCES} -ts ${INKPOT_TS_FILES}
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            COMMENT "Updating translation catalogues from the sources"
            VERBATIM
        )
    endif()
else()
    message(STATUS
        "Neither the Qt LinguistTools CMake package nor lrelease was found - "
        "building without translations. On Debian and Ubuntu the programs come "
        "with the qt6-l10n-tools package.")
endif()

if(INKPOT_QM_FILES)
    install(FILES ${INKPOT_QM_FILES}
            DESTINATION ${CMAKE_INSTALL_DATADIR}/inkpot/translations)
endif()

# --------------------------------------------------------------- Installation

install(TARGETS inkpot
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(FILES data/${INKPOT_APP_ID}.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)

install(FILES data/${INKPOT_APP_ID}.metainfo.xml
        DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)

install(FILES data/${INKPOT_APP_ID}.xml
        DESTINATION ${CMAKE_INSTALL_DATADIR}/mime/packages)

install(FILES data/icons/hicolor/scalable/apps/${INKPOT_APP_ID}.svg
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)

# ------------------------------------------------------------ Debian package
#
# Built with tools/build-deb.sh, which configures with the prefix /usr and then
# runs cpack. The install rules above are reused, so nothing is described twice.

set(CPACK_PACKAGE_NAME "inkpot")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_VENDOR "${INKPOT_AUTHOR}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_HOMEPAGE_URL "${PROJECT_HOMEPAGE_URL}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
set(CPACK_GENERATOR "DEB")

set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Christoph Krogmann <ch.krogmann@outlook.de>")
set(CPACK_DEBIAN_PACKAGE_SECTION "graphics")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")

# Works out the needed libraries from the binary itself, so the list cannot
# drift away from what was actually linked.
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

# Not needed to run, but they widen what can be opened and saved.
set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "qt6-image-formats-plugins")

set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
    "${CMAKE_CURRENT_SOURCE_DIR}/packaging/deb/postinst"
    "${CMAKE_CURRENT_SOURCE_DIR}/packaging/deb/postrm")

# The one line summary above becomes the first line of the field; what follows
# is the long description. CPack indents it, so the lines carry no spaces of
# their own - with them the paragraph would break apart.
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION
"Brush strokes, lines, shapes, captions, filled areas and pasted pictures are
objects in their own right: they can be selected, moved, turned, scaled,
recoloured and deleted at any time, long after they were drawn. The eraser
and the bucket fill work across objects without destroying them, and the
project format keeps every object editable.")

include(CPack)

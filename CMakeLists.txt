cmake_minimum_required(VERSION 3.19)

project(Inkpot
    VERSION 0.1.0
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
    src/Icons.cpp
    src/Icons.h
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

# ---------------------------------------------------------------- Translations

set(INKPOT_TS_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/inkpot_de.ts
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

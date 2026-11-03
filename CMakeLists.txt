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

qt_add_executable(inkpot
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

target_include_directories(inkpot PRIVATE src)

target_compile_definitions(inkpot PRIVATE
    INKPOT_APP_ID="${INKPOT_APP_ID}"
    INKPOT_VERSION="${PROJECT_VERSION}"
    INKPOT_HOMEPAGE="${PROJECT_HOMEPAGE_URL}"
    INKPOT_TRANSLATIONS_DIR="${CMAKE_INSTALL_FULL_DATADIR}/inkpot/translations"
)

target_link_libraries(inkpot PRIVATE
    Qt6::Widgets
    Qt6::PrintSupport
)

# ---------------------------------------------------------------- Translations

set(INKPOT_TS_FILES
    translations/inkpot_de.ts
)

if(Qt6LinguistTools_FOUND)
    # "cmake --build build --target update_translations" refreshes the .ts
    # files from the sources; the .qm files are built along with the program.
    qt_add_lupdate(inkpot TS_FILES ${INKPOT_TS_FILES})
    qt_add_lrelease(inkpot
        TS_FILES ${INKPOT_TS_FILES}
        QM_FILES_OUTPUT_VARIABLE INKPOT_QM_FILES
    )
    install(FILES ${INKPOT_QM_FILES}
            DESTINATION ${CMAKE_INSTALL_DATADIR}/inkpot/translations)
else()
    message(STATUS
        "Qt LinguistTools not found - building without translations. "
        "On Debian and Ubuntu they come with the qt6-l10n-tools package.")
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

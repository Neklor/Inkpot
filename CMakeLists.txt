cmake_minimum_required(VERSION 3.19)

project(Inkpot
    VERSION 0.1.0
    DESCRIPTION "Ein einfaches, objektbasiertes Malprogramm fuer Linux"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build-Typ" FORCE)
endif()

find_package(Qt6 6.4 REQUIRED COMPONENTS Widgets PrintSupport)

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
    src/NewCanvasDialog.cpp
    src/NewCanvasDialog.h
    src/TextItem.cpp
    src/TextItem.h
    src/Icons.cpp
    src/Icons.h
    src/InkObject.cpp
    src/InkObject.h
)

target_include_directories(inkpot PRIVATE src)

target_link_libraries(inkpot PRIVATE
    Qt6::Widgets
    Qt6::PrintSupport
)

install(TARGETS inkpot RUNTIME DESTINATION bin)

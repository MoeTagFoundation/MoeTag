cmake_minimum_required(VERSION 3.5)

project(MoeTagCMake VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")

set(CMAKE_PREFIX_PATH "C:\\Qt\\6.4.2\\msvc2019_64\\")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/modules/")

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network Concurrent Multimedia MultimediaWidgets Svg LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network Concurrent Multimedia MultimediaWidgets Svg LinguistTools)

if (WIN32)
    set(FFMPEG_DIRECTORY ${CMAKE_SOURCE_DIR}/ffmpegwindows64)
else()
    find_path(FFMPEG_DIRECTORY ffmpeg)
endif()

set(FFMPEGTHUMBS_DIRECTORY ${CMAKE_SOURCE_DIR}/ffmpegthumbs)

include_directories(${FFMPEG_DIRECTORY}/include)
link_directories(${FFMPEG_DIRECTORY}/lib)

set(TS_FILES MoeTagCMake_en_US.ts)

set(THMB_FILES
    ${FFMPEGTHUMBS_DIRECTORY}/ffmpegthumbnailer.cpp
    ${FFMPEGTHUMBS_DIRECTORY}/ffmpegthumbnailer.h
    ${FFMPEGTHUMBS_DIRECTORY}/ffmpegthumbnailer/filmstrip.h
    ${FFMPEGTHUMBS_DIRECTORY}/ffmpegthumbnailer/filmstripfilter.h
    ${FFMPEGTHUMBS_DIRECTORY}/ffmpegthumbnailer/histogram.h
    ${FFMPEGTHUMBS_DIRECTORY}/ffmpegthumbnailer/ifilter.h
    ${FFMPEGTHUMBS_DIRECTORY}/ffmpegthumbnailer/imagewriter.cpp
    ${FFMPEGTHUMBS_DIRECTORY}/ffmpegthumbnailer/imagewriter.h
    ${FFMPEGTHUMBS_DIRECTORY}/ffmpegthumbnailer/moviedecoder.cpp
    ${FFMPEGTHUMBS_DIRECTORY}/ffmpegthumbnailer/moviedecoder.h
    ${FFMPEGTHUMBS_DIRECTORY}/ffmpegthumbnailer/videoframe.h
    ${FFMPEGTHUMBS_DIRECTORY}/ffmpegthumbnailer/videothumbnailer.cpp
    ${FFMPEGTHUMBS_DIRECTORY}/ffmpegthumbnailer/videothumbnailer.h
)

if(WIN32)
    set(APPMETADATA resources/resources.rc version.h)
endif()

set(MOESRC_DIR src/)

set(PROJECT_SOURCES
        ${MOESRC_DIR}directorylistmodel.cpp
        ${MOESRC_DIR}directoryindexer.cpp
        ${MOESRC_DIR}directorypopulator.cpp
        ${MOESRC_DIR}moeglobals.cpp
        ${MOESRC_DIR}main.cpp
        ${MOESRC_DIR}moemainwindow.cpp
        ${MOESRC_DIR}moepreviewlabel.cpp
        ${MOESRC_DIR}taglistitem.cpp
        ${MOESRC_DIR}delimitedcompleter.cpp

        ${MOESRC_DIR}directorylistmodel.h
        ${MOESRC_DIR}directoryindexer.h
        ${MOESRC_DIR}directorypopulator.h
        ${MOESRC_DIR}moeglobals.h
        ${MOESRC_DIR}moemainwindow.h
        ${MOESRC_DIR}moepreviewlabel.h
        ${MOESRC_DIR}taglistitem.h
        ${MOESRC_DIR}delimitedcompleter.h

        ${MOESRC_DIR}/DockInDock/dockindock.cpp
        ${MOESRC_DIR}/DockInDock/dockindockmanager.cpp
        ${MOESRC_DIR}/DockInDock/perspectiveactions.cpp
        ${MOESRC_DIR}/DockInDock/perspectives.cpp
        ${MOESRC_DIR}/DockInDock/dockindock.h
        ${MOESRC_DIR}/DockInDock/dockindockmanager.h
        ${MOESRC_DIR}/DockInDock/perspectiveactions.h
        ${MOESRC_DIR}/DockInDock/perspectives.h

        ${APPMETADATA}

        resources/bundled.qrc
        ${CMAKE_SOURCE_DIR}/BreezeStyleSheets/dist/qrc/custom.qrc

        ${THMB_FILES}
        ${TS_FILES}
)

add_subdirectory(${CMAKE_SOURCE_DIR}/Qt-Advanced-Docking-System)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(MoeTagCMake
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET MoeTagCMake APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation

    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
else()
    if(ANDROID)
        add_library(MoeTagCMake SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(MoeTagCMake
            ${PROJECT_SOURCES}
        )
    endif()

    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()

target_link_libraries(MoeTagCMake PRIVATE qtadvanceddocking
    Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Concurrent Qt${QT_VERSION_MAJOR}::MultimediaWidgets
    Qt${QT_VERSION_MAJOR}::Svg Qt${QT_VERSION_MAJOR}::Multimedia
    avcodec avformat swscale avutil avfilter
)

set_target_properties(MoeTagCMake PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER app.moetag.org
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

install(TARGETS MoeTagCMake
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

# BUILD DEPENDENCIES (NEEDED FOR VISUAL STUDIO)
if (WIN32)
    message("Running windeployqt")
    if(NOT ${CMAKE_BUILD_TYPE} MATCHES "Debug")
        message("Building windeployqt for release mode")
        SET(EXTRA_WINDEPLOY_FLAGS --release)
    else()
        message("Building windeployqt for debug mode")
    endif()

    set(WINDEPLOYQT_PATH "C:/Qt/6.4.2/msvc2019_64/bin/windeployqt.exe" CACHE STRING "Path to windeployqt") # TODO: Replace hardcoded windeployqt path

    execute_process(COMMAND ${WINDEPLOYQT_PATH} "${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.exe" ${EXTRA_WINDEPLOY_FLAGS})
else()
    message("Warning: You appear to be running CMake on a system that isnt WIN32, your output folder may need dependencies to be added")
endif()

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(MoeTagCMake)
endif()

# COPY BUILT DEPENDENCIES
if (WIN32)
    set (FFMPEG_BIN_PATH ./ffmpegwindows64/dependencies)

    message("Copying over ffmpeg libs from ${FFMPEG_BIN_PATH} to ${CMAKE_BINARY_DIR}")

    file(GLOB FFMPEG_BIN
        "${FFMPEG_BIN_PATH}/*.dll"
    )

    message("Found libs: ${FFMPEG_BIN}")

    file(COPY ${FFMPEG_BIN} DESTINATION ${CMAKE_BINARY_DIR})
endif()

message("Copying over tags")

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/resources/tags.json DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/resources/)

# Default endpoints
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/resources/endpoints/danbooru.json DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/resources/endpoints)



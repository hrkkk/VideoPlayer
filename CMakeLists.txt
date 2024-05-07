cmake_minimum_required(VERSION 3.5)

project(VideoPlayer VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(D:/Environment/FFmpeg/include)
link_directories(D:/Environment/FFmpeg/lib)

include_directories("D:/Environment/SDL/SDL2-2.30.1/include")
link_directories("D:/Environment/SDL/SDL2-2.30.1/lib/x64")


find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

# 添加OpenGL库链接
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS OpenGLWidgets REQUIRED)


set(PROJECT_SOURCES
        main.cpp
        MainWindow.cpp
        MainWindow.h
        MainWindow.ui
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(VideoPlayer
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        PlayerContext.h PlayerContext.cpp
        Player.h Player.cpp
        AudioPlayThread.h AudioPlayThread.cpp
        DemuxThread.h DemuxThread.cpp
        VideoDecodeThread.h VideoDecodeThread.cpp
        AudioDecodeThread.h AudioDecodeThread.cpp
        BaseThread.h BaseThread.cpp
        CustomProgressBar.h CustomProgressBar.cpp
        res.qrc
        Utils.h Utils.cpp
        CustomOpenGLWidget.h CustomOpenGLWidget.cpp
        VideoPlayThread.h VideoPlayThread.cpp
        FileAnalyse.h FileAnalyse.cpp
        InfoDialog.h InfoDialog.cpp InfoDialog.ui
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET VideoPlayer APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(VideoPlayer SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(VideoPlayer
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(VideoPlayer PRIVATE Qt${QT_VERSION_MAJOR}::Widgets avcodec avdevice avfilter avformat avutil swresample swscale SDL2.lib)
target_link_libraries(VideoPlayer PRIVATE Qt6::OpenGLWidgets)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.VideoPlayer)
endif()
set_target_properties(VideoPlayer PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS VideoPlayer
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(VideoPlayer)
endif()

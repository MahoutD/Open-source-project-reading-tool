cmake_minimum_required(VERSION 3.20)

project(OpenSourceCodeReader LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS
    Core
    Gui
    Qml
    Quick
    QuickControls2
    Network
    Svg
    Concurrent
)

set(SOURCES
    src/main.cpp
    src/Logger.h
    src/Logger.cpp
    src/NetworkTester.h
    src/NetworkTester.cpp
    src/DependencyAnalyzer.h
    src/DependencyAnalyzer.cpp
    src/GraphVisualizer.h
    src/GraphVisualizer.cpp
    src/InteractiveGraphView.h
    src/InteractiveGraphView.cpp
    src/CodeSyntaxHighlighter.h
    src/CodeSyntaxHighlighter.cpp
    src/CodeEditorBridge.h
    src/CodeEditorBridge.cpp
    src/ProjectController.h
    src/ProjectController.cpp
    resources.qrc
)

add_executable(OpenSourceCodeReader WIN32 ${SOURCES})

target_link_libraries(OpenSourceCodeReader PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Network
    Qt6::Svg
    Qt6::Concurrent
)

if(MSVC)
    target_compile_options(OpenSourceCodeReader PRIVATE /utf-8 /W3 /permissive- /Zc:__cplusplus)
endif()

cmake_minimum_required(VERSION 3.10)

# Project Name
project(LinearSeq VERSION 0.1 LANGUAGES CXX)

# Standard C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -----------------------------------------------------------------------------
# Find Dependencies
# -----------------------------------------------------------------------------

# 1. ALSA (Audio/MIDI)
find_package(ALSA REQUIRED)

# 2. FLTK (GUI)
find_package(FLTK REQUIRED)

# -----------------------------------------------------------------------------
# Source Files
# -----------------------------------------------------------------------------

# Add all your source files here
set(SOURCES
    src/main.cpp
    src/ui/MainWindow.cpp
    src/ui/MainToolbar.cpp
    src/ui/LseqMenuButton.cpp
    src/ui/EventList.cpp
    src/ui/TrackView.cpp
    src/ui/TrackRowView.cpp
    src/core/Clock.cpp
    src/core/Sequencer.cpp
    src/audio/AlsaDriver.cpp
    src/utils/SongJson.cpp
)

# -----------------------------------------------------------------------------
# Build Executable
# -----------------------------------------------------------------------------

add_executable(${PROJECT_NAME} ${SOURCES})

# -----------------------------------------------------------------------------
# Link Libraries
# -----------------------------------------------------------------------------

# Link ALSA and FLTK to the executable
target_link_libraries(${PROJECT_NAME} 
    PRIVATE 
    ALSA::ALSA 
    fltk
)

# Include Directories (allows #include "core/Clock.h" etc.)
target_include_directories(${PROJECT_NAME} PRIVATE src)
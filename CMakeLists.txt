cmake_minimum_required(VERSION 3.20)
project(cafe-in CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Locate wxWidgets package and required components
find_package(wxWidgets REQUIRED COMPONENTS core base)
include(${wxWidgets_USE_FILE})

# Add libraries
add_library(Logic STATIC app/logic.h app/logic.cpp)
# Define your source files and executable
add_executable(cafe-in main.cpp)
add_executable(test_logic app/main.cpp)

# Link against the resolved wxWidgets libraries
target_link_libraries(cafe-in PRIVATE ${wxWidgets_LIBRARIES})

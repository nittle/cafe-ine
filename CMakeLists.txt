cmake_minimum_required(VERSION 3.20)
project(cafe-in C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Locate wxWidgets package and required components
find_package(wxWidgets REQUIRED COMPONENTS core base)
include(${wxWidgets_USE_FILE})

find_package(Threads REQUIRED)

enable_testing()

# Embedded SQLite used by the database layer.
add_library(sqlite3 STATIC
    SQLite/sqlite3.c
    SQLite/sqlite3.h
)
target_include_directories(sqlite3 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/SQLite)

# Application logic is independent of the GUI and can be tested on its own.
add_library(Logic STATIC app/logic.cpp app/logic.h)
target_include_directories(Logic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/app)

if(MSVC)
    # Source files contain Russian UTF-8 string literals.
    target_compile_options(Logic PUBLIC /utf-8)
endif()

# GUI executable (the GUI is being developed by another team member).
add_executable(cafe-in main.cpp)
target_link_libraries(cafe-in PRIVATE
    Logic
    sqlite3
    ${wxWidgets_LIBRARIES}
    Threads::Threads
    ${CMAKE_DL_LIBS}
)

# Console self-test / demo for the application logic.
add_executable(test_logic app/main.cpp)
target_link_libraries(test_logic PRIVATE Logic)

add_test(NAME logic_self_test COMMAND test_logic)

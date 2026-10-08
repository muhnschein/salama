# harbour-salama — Silica web browser for Sailfish OS.
# Two flavours:
#   * device RPM (rpm/harbour-salama.spec): SALAMA_BUILD_TESTS=OFF, sailfishapp required.
#   * host `make check`: tests on, sailfishapp stubbed by tests/stubs/sailfishapp.
cmake_minimum_required(VERSION 3.13)
project(harbour-salama VERSION 0.8.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(SALAMA_BUILD_TESTS "Build the host test suite" ON)
option(SALAMA_COVERAGE "Instrument with gcov" OFF)
option(SALAMA_REQUIRE_SAILFISHAPP "Fail instead of falling back to the host stub" OFF)
set(SALAMA_VERSION "${PROJECT_VERSION}" CACHE STRING "Version stamped into the binary")

find_package(Qt5 5.6 REQUIRED COMPONENTS Core DBus Gui Qml Quick Sql)

add_compile_options(-Wall -Wextra -Wpedantic -Werror)
if(SALAMA_COVERAGE)
    add_compile_options(--coverage -O0 -g)
    add_link_options(--coverage)
endif()

add_subdirectory(src)
add_subdirectory(translations)

if(SALAMA_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

install(DIRECTORY qml DESTINATION share/${PROJECT_NAME})
# Non-QML pictures QML draws: ci/harbour-check.sh allows only QML in qml/, and cover
# icon must be file Image can resolve.
install(DIRECTORY art DESTINATION share/${PROJECT_NAME})
install(FILES ${PROJECT_NAME}.desktop DESTINATION share/applications)
# Readability compiled in (src/reader/reader.qrc): licence + notice ship with package (Apache-2.0 §4).
install(FILES third_party/readability/LICENSE.md third_party/readability/NOTICE
        DESTINATION share/${PROJECT_NAME}/licenses/readability)
foreach(size 86x86 108x108 128x128 172x172)
    install(FILES icons/${size}/${PROJECT_NAME}.png DESTINATION share/icons/hicolor/${size}/apps)
endforeach()

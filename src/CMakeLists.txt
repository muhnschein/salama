# Engine-independent core: models exposed to QML, plus the application entry point.
add_library(salama-core STATIC
    Core.cpp
    Core.h
    QmlTypes.cpp
    QmlTypes.h
    bookmarks/BookmarkModel.cpp
    bookmarks/BookmarkModel.h
    downloads/DownloadModel.cpp
    downloads/DownloadModel.h
    engine/EngineData.cpp
    engine/EngineData.h
    engine/EngineMessages.cpp
    engine/EngineMessages.h
    engine/PageActivity.cpp
    engine/PageActivity.h
    history/HistoryModel.cpp
    history/HistoryModel.h
    reader/Reader.cpp
    reader/Reader.h
    reader/reader.qrc
    settings/Settings.cpp
    settings/Settings.h
    storage/Storage.cpp
    storage/Storage.h
    tabs/ClosedTabModel.cpp
    tabs/ClosedTabModel.h
    tabs/GroupTabModel.cpp
    tabs/GroupTabModel.h
    tabs/Tab.h
    tabs/TabGroupModel.cpp
    tabs/TabGroupModel.h
    tabs/TabModel.cpp
    tabs/TabModel.h
    tabs/TabPersistence.cpp
    tabs/TabPersistence.h
    tabs/TabSearchModel.cpp
    tabs/TabSearchModel.h
)
target_include_directories(salama-core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# The reader view's scripts and style sheet, compiled in (src/reader/reader.qrc).
set_target_properties(salama-core PROPERTIES AUTORCC ON)
# Gui, for QColor: a page's theme-color is a CSS colour, and reading one is what
# QColor is for (src/engine/EngineMessages.cpp).
target_link_libraries(salama-core PUBLIC Qt5::Core Qt5::Gui Qt5::Qml Qt5::Sql)
target_compile_definitions(salama-core PUBLIC
    QT_NO_CAST_FROM_ASCII
    QT_NO_CAST_TO_ASCII
    QT_NO_URL_CAST_FROM_STRING
    SALAMA_VERSION="${SALAMA_VERSION}"
)

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE salama-core Qt5::Gui Qt5::Quick)
# The Silica booster dlopen()s the binary and calls its exported main(): -pie -rdynamic
# is what libsailfishapp's qmake feature adds, reproduced here for CMake.
target_link_options(${PROJECT_NAME} PRIVATE -pie -rdynamic)

find_package(PkgConfig)
if(PkgConfig_FOUND)
    if(SALAMA_REQUIRE_SAILFISHAPP)
        pkg_check_modules(SAILFISHAPP REQUIRED IMPORTED_TARGET sailfishapp)
    else()
        pkg_check_modules(SAILFISHAPP IMPORTED_TARGET sailfishapp)
    endif()
endif()

if(SAILFISHAPP_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::SAILFISHAPP)
else()
    if(SALAMA_REQUIRE_SAILFISHAPP)
        message(FATAL_ERROR "sailfishapp is required for a device build")
    endif()
    message(STATUS "sailfishapp not found: linking ${PROJECT_NAME} against tests/stubs/sailfishapp (host build)")
    target_sources(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/tests/stubs/sailfishapp/sailfishapp.cpp)
    target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/tests/stubs/sailfishapp)
    target_compile_definitions(${PROJECT_NAME} PRIVATE SALAMA_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
endif()

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)

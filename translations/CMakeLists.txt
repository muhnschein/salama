# Compile the .ts catalogs with lrelease. The Qt5LinguistTools CMake package is not
# guaranteed on the device SDK image, so the binary is located directly.
get_target_property(_qmake_location Qt5::qmake IMPORTED_LOCATION)
get_filename_component(_qt_bin_dir "${_qmake_location}" DIRECTORY)
find_program(LRELEASE_EXECUTABLE NAMES lrelease lrelease-qt5 HINTS "${_qt_bin_dir}")
if(NOT LRELEASE_EXECUTABLE)
    message(FATAL_ERROR "lrelease not found; install qt5-qttools-linguist")
endif()

# The source catalog harbour-salama.ts is not compiled: lupdate reads the strings
# from it, and it is never loaded as a translation. Every language catalog beside
# it is, one .qm each, which is what QTranslator loads at startup.
file(GLOB TS_FILES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/harbour-salama-*.ts
)
if(NOT TS_FILES)
    message(FATAL_ERROR "no translations/harbour-salama-*.ts found")
endif()

set(QM_FILES)
foreach(ts ${TS_FILES})
    get_filename_component(base "${ts}" NAME_WE)
    set(qm "${CMAKE_CURRENT_BINARY_DIR}/${base}.qm")
    add_custom_command(
        OUTPUT "${qm}"
        COMMAND "${LRELEASE_EXECUTABLE}" -silent "${ts}" -qm "${qm}"
        DEPENDS "${ts}"
        COMMENT "Compiling ${base}.qm"
        VERBATIM
    )
    list(APPEND QM_FILES "${qm}")
endforeach()

add_custom_target(translations ALL DEPENDS ${QM_FILES})
install(FILES ${QM_FILES} DESTINATION share/${PROJECT_NAME}/translations)

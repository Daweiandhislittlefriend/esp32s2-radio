
add_compile_options(-fdiagnostics-color=always)

list(APPEND EXTRA_COMPONENT_DIRS 
                                "${CMAKE_CURRENT_LIST_DIR}/components"
                                "${CMAKE_CURRENT_LIST_DIR}/components/audio"
                                "${CMAKE_CURRENT_LIST_DIR}/examples/common_components"
                                "${CMAKE_CURRENT_LIST_DIR}/components/bus"
                                "${CMAKE_CURRENT_LIST_DIR}/components/button"
                                "${CMAKE_CURRENT_LIST_DIR}/components/display"
                                "${CMAKE_CURRENT_LIST_DIR}/components/display/digital_tube"
                                "${CMAKE_CURRENT_LIST_DIR}/components/expander/io_expander"
                                "${CMAKE_CURRENT_LIST_DIR}/components/gui"
                                "${CMAKE_CURRENT_LIST_DIR}/components/led"
                                "${CMAKE_CURRENT_LIST_DIR}/components/motor"
                                "${CMAKE_CURRENT_LIST_DIR}/components/sensors"
                                "${CMAKE_CURRENT_LIST_DIR}/components/sensors/gesture"
                                "${CMAKE_CURRENT_LIST_DIR}/components/sensors/humiture"
                                "${CMAKE_CURRENT_LIST_DIR}/components/sensors/imu"
                                "${CMAKE_CURRENT_LIST_DIR}/components/sensors/light_sensor"
                                "${CMAKE_CURRENT_LIST_DIR}/components/sensors/pressure"
                                "${CMAKE_CURRENT_LIST_DIR}/components/storage"
                                "${CMAKE_CURRENT_LIST_DIR}/components/storage/eeprom"
                                )


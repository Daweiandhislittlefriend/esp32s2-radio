# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)



include($ENV{IDF_PATH}/tools/cmake/project.cmake)
include($ENV{ADF_PATH}/CMakeLists.txt)
include(${CMAKE_CURRENT_LIST_DIR}/components/esp-iot-solution/component.cmake)
add_definitions(-DCONFIG_LVGL_TFT_DISPLAY_MONOCHROME)


project(esp32s2-radio)
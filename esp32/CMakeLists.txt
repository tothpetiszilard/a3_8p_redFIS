# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp32_a3_8p_redFIS)

# idf_build_set_property(COMPILE_OPTIONS "-DREDFIS_SINGLE_THREAD" APPEND)
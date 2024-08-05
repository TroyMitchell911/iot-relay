# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)
set(PYTHON "/home/troy/.espressif/python_env/idf5.3_py3.10_env/bin/python")
set(python "/home/troy/.espressif/python_env/idf5.3_py3.10_env/bin/python")
# 不加这句默认为 'esp32'
set(IDF_TARGET "esp32c3")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(relay)

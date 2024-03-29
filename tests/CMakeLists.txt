cmake_minimum_required(VERSION 3.18)
project(tests VERSION 1.0 LANGUAGES CXX)

add_subdirectory(_cloak)

file(GLOB test_SRC "*.cpp" "*.h")
file(GLOB_RECURSE components_SRC "${CMAKE_CURRENT_SOURCE_DIR}/../components/*.cpp")
if(EX_TEST_FILTER)
  list(FILTER components_SRC EXCLUDE REGEX "${EX_TEST_FILTER}")
endif()
list(APPEND test_SRC ${components_SRC})
if(EX_TEST_SOURCES)
  foreach(ex_src_item ${EX_TEST_SOURCES})
    file(GLOB ex_SRC "${ex_src_item}")
    list(APPEND test_SRC ${ex_SRC})
  endforeach(ex_src_item)
endif()

# add_link_options(-v)
# set(CMAKE_EXE_LINKER_FLAGS -v)

add_executable(${PROJECT_NAME} ${test_SRC})
target_link_libraries(${PROJECT_NAME} cloak)
target_include_directories(${PROJECT_NAME} PUBLIC "${EX_TEST_INCLUDES}")
target_compile_definitions(${PROJECT_NAME} PUBLIC "${EX_TEST_DEFINES}")

set(ESPHOME_LIB_INCLUDE_DIR "${CMAKE_BINARY_DIR}/include/esphome")
make_directory(${ESPHOME_LIB_INCLUDE_DIR})
foreach(ex_include ${EX_TEST_SOURCES_ESPHOME})
  message(STATUS "Configuring esphome include directory ${ex_include}")
  file(GLOB ex_include_files RELATIVE "${ex_include}/../.." "${ex_include}/*.h")
  foreach(ex_include_file ${ex_include_files})
    message(STATUS "Configuring file ${ex_include_file}")
    # message(STATUS "Configuring file " "${ex_include}/../../${ex_include_file}" " ${ESPHOME_LIB_INCLUDE_DIR}/${ex_include_file}")
    configure_file("${ex_include}/../../${ex_include_file}" "${ESPHOME_LIB_INCLUDE_DIR}/${ex_include_file}" COPYONLY)
  endforeach(ex_include_file)
endforeach(ex_include)

target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_BINARY_DIR}/include")
# set(CMAKE_INCLUDE_CURRENT_DIR ON)

IF(CMAKE_BUILD_TYPE MATCHES Debug)
  message("Debug build.")
ELSEIF(CMAKE_BUILD_TYPE MATCHES Release)
  message("Release build.")
ELSE()
  message("Some other build type.")
ENDIF()

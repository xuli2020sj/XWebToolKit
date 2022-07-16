cmake_minimum_required(VERSION 3.22)
project(XLToolKit)

set(CMAKE_CXX_STANDARD 20)

set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
set(INSTALL_PATH_LIB lib)
set(INSTALL_PATH_INCLUDE include)
set(THREADS_PREFER_PTHREAD_FLAG ON)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH}")


#设置子目录
set(SUB_DIR_LIST "Network" "Poller" "Thread" "Utility")

foreach(SUB_DIR ${SUB_DIR_LIST})
    aux_source_directory(src/${SUB_DIR} SRC_LIST)
    install(DIRECTORY src/${SUB_DIR} DESTINATION ${INSTALL_PATH_INCLUDE}/${CMAKE_PROJECT_NAME} FILES_MATCHING PATTERN "*.h")
endforeach(SUB_DIR ${SUB_DIR_LIST})


include_directories(${PROJECT_SOURCE_DIR}/src)

#编译动态库
add_library(${CMAKE_PROJECT_NAME}_shared SHARED ${SRC_LIST})
target_link_libraries(${CMAKE_PROJECT_NAME}_shared ${LINK_LIB_LIST})
set_target_properties(${CMAKE_PROJECT_NAME}_shared PROPERTIES OUTPUT_NAME "${CMAKE_PROJECT_NAME}")
install(TARGETS ${CMAKE_PROJECT_NAME}_shared  ARCHIVE DESTINATION ${INSTALL_PATH_LIB} LIBRARY DESTINATION ${INSTALL_PATH_LIB})


#编译静态库
add_library(${CMAKE_PROJECT_NAME}_static STATIC ${SRC_LIST})
set_target_properties(${CMAKE_PROJECT_NAME}_static PROPERTIES OUTPUT_NAME "${CMAKE_PROJECT_NAME}")
#安装静态库至系统目录
install(TARGETS ${CMAKE_PROJECT_NAME}_static ARCHIVE DESTINATION ${INSTALL_PATH_LIB})

add_executable(WebServer main.cpp)

add_subdirectory(tests)


# 编译生成的二进制文件输出目录
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin/test)

add_subdirectory(pool_util)
add_subdirectory(timer)
add_subdirectory(macro)
add_subdirectory(uuid)
add_subdirectory(logger)
add_subdirectory(attribute)
add_subdirectory(filehelper)
add_subdirectory(lua)
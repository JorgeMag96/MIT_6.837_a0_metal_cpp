set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ..) # This will place the executable in the root folder of the build path, instead of having it under the src/

add_executable(a0_metal main.cpp)
target_link_libraries(a0_metal METAL_CPP)

# --------------------------------------------------------------------
# ------ Define the source and destination directories ------
set(SOURCE_DIR "${CMAKE_SOURCE_DIR}/resources")
set(DESTINATION_DIR "${CMAKE_BINARY_DIR}/resources")

# ------ Create a custom target to copy the resources folder ------
add_custom_target(copy_resources ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${SOURCE_DIR} ${DESTINATION_DIR})
# --------------------------------------------------------------------

# Add a dependency on the custom target, ensuring it runs before the main target
add_dependencies(a0_metal copy_resources)
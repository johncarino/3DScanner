# Build the app, using the HAL

include_directories(include)
file(GLOB MY_SOURCES "src/*.c")
add_executable(3DScanner ${MY_SOURCES})

# Tell the 3DScanner target where to find OpenCV's header files
target_include_directories(3DScanner PUBLIC ${OpenCV_INCLUDE_DIRS})

# Make use of the HAL library
target_link_libraries(3DScanner LINK_PRIVATE hal)

# target_link_libraries(3DScanner LINK_PRIVATE v4l2)
# target_link_libraries(3DScanner LINK_PRIVATE grabber)

# Copy executable to final location (change `3DScanner` to project name as needed)
add_custom_command(TARGET 3DScanner POST_BUILD 
  COMMAND "${CMAKE_COMMAND}" -E copy 
     "$<TARGET_FILE:3DScanner>"
     "~/ensc351/public/myApps/3DScanner"
  COMMENT "Copying ARM executable to public NFS directory")

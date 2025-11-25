# CMakeList.txt for HAL
#   Build a library (`hal`) which exposes the header files as "hal/*.h"
include_directories(hal/include)
file(GLOB MY_SOURCES "src/*.c")

add_library(hal STATIC ${MY_SOURCES})

target_include_directories(hal PUBLIC include)

# Link the libraries that this HAL module needs.
# 'PUBLIC' means that any target linking 'hal' (like our 3DScanner app)
# will also be linked against these.
target_link_libraries(hal PUBLIC 
    gpiod
    v4l2
    pthread
)
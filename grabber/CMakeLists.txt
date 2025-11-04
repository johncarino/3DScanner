# This is the content for grabber/CMakeLists.txt
# This code comes directly from Step 3f-ii of the guide 

add_library(grabber STATIC grabber.c) 

# Remove extra compiler options that were set in base. [cite: 121]
# Ref: https://discourse.cmake.org/t/how-to-disable-pedantic-compiler-opti on-for-a-specific-library/1575 [cite: 123]
get_target_property(target_options grabber COMPILE_OPTIONS) 
list(REMOVE_ITEM target_options "-Wpedantic")
list(REMOVE_ITEM target_options "-Wextra") 
list(REMOVE_ITEM target_options "-Werror") 
list(REMOVE_ITEM target_options "-Wall")
set_target_properties(grabber PROPERTIES COMPILE_OPTIONS "${target_options}")

# Setup the include paths for where modules that use this library will search. [cite: 131]
target_include_directories(grabber PUBLIC .)
include_directories(grabber)
add_compile_options(-D_BSD_SOURCE)
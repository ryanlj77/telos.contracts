add_contract(trail trail ${CMAKE_CURRENT_SOURCE_DIR}/src/trail.cpp)

target_compile_options(trail PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/resources)
target_compile_options(trail PUBLIC -Wunknown-pragmas)
target_include_directories(trail
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(trail
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

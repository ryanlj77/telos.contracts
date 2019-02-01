add_contract(telos.canopy telos.canopy ${CMAKE_CURRENT_SOURCE_DIR}/src/telos.canopy.cpp)

target_compile_options(telos.canopy.wasm PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/resources)
target_compile_options(telos.canopy.wasm PUBLIC -Wunknown-pragmas)
target_include_directories(telos.canopy.wasm
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/../telos.canopy/include
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(telos.canopy.wasm
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

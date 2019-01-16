add_contract(telos.snaps telos.snaps ${CMAKE_CURRENT_SOURCE_DIR}/src/telos.snaps.cpp)
target_include_directories(telos.snaps.wasm
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(telos.snaps.wasm
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

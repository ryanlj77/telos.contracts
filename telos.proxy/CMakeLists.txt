add_contract(telos.proxy telos.proxy ${CMAKE_CURRENT_SOURCE_DIR}/src/telos.proxy.cpp)
target_include_directories(telos.proxy.wasm
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(telos.proxy.wasm
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

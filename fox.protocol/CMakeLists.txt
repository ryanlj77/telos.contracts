add_contract(fox.protocol fox.protocol ${CMAKE_CURRENT_SOURCE_DIR}/src/fox.protocol.cpp) #NOTE: this path needs to change

target_compile_options(fox.protocol.wasm PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/resources)

target_compile_options(fox.protocol.wasm PUBLIC -Wunknown-pragmas)

target_include_directories(fox.protocol.wasm
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/../fox.protocol/include
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(fox.protocol.wasm
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

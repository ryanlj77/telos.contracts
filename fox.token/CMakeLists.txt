add_contract(fox.token fox.token ${CMAKE_CURRENT_SOURCE_DIR}/src/fox.token.cpp) #NOTE: this path needs to change

target_compile_options(fox.token.wasm PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/resources)

target_compile_options(fox.token.wasm PUBLIC -Wunknown-pragmas)

target_include_directories(fox.token.wasm
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/../fox.token/include
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(fox.token.wasm
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

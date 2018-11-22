add_contract(update.auth update.auth ${CMAKE_CURRENT_SOURCE_DIR}/src/update.auth.cpp)
target_include_directories(update.auth.wasm
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(update.auth.wasm
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

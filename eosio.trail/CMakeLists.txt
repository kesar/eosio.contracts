add_contract(eosio.trail eosio.trail ${CMAKE_CURRENT_SOURCE_DIR}/src/eosio.trail.cpp) #NOTE: this path needs to change

target_compile_options(eosio.trail.wasm PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/resources)
target_compile_options(eosio.trail.wasm PUBLIC -Wunknown-pragmas)
target_include_directories(eosio.trail.wasm
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(eosio.trail.wasm
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

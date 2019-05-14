cmake_minimum_required( VERSION 3.5 )

set(EOSIO_VERSION_MIN "1.6")
set(EOSIO_VERSION_SOFT_MAX "1.6")
set(MY_LIB_DIR "/Users/mwilliams/Projects/eos/libraries")
#set(EOSIO_VERSION_HARD_MAX "")

find_package(eosio)

### Check the version of eosio
set(VERSION_MATCH_ERROR_MSG "")
EOSIO_CHECK_VERSION(VERSION_OUTPUT "${EOSIO_VERSION}"
                                   "${EOSIO_VERSION_MIN}"
                                   "${EOSIO_VERSION_SOFT_MAX}"
                                   "${EOSIO_VERSION_HARD_MAX}"
                                   VERSION_MATCH_ERROR_MSG)



enable_testing()

#include(${MY_LIB_DIR}/softfloat/source/include/softfloat.hpp)

include_directories(${CMAKE_SOURCE_DIR}/libraries/softfloat/source)

configure_file(${CMAKE_SOURCE_DIR}/contracts.hpp.in ${CMAKE_BINARY_DIR}/contracts.hpp)

include_directories(${CMAKE_BINARY_DIR})

#FIND_PATH(MY_LIB_DIR softfloat.hpp "./include" DOC "Freetype includes")

file(GLOB UNIT_TESTS "*.cpp" "*.hpp")

add_eosio_test( unit_test ${UNIT_TESTS} )

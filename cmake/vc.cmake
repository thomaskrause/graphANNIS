set(Vc_PREFIX ${GLOBAL_OUTPUT_PATH}/vc)

set(Vc_SOURCE_DIR ${CMAKE_SOURCE_DIR}/ext/Vc-1.3.1)

ExternalProject_Add(
  Vc

  UPDATE_COMMAND ""
  PATCH_COMMAND ""

  SOURCE_DIR "${Vc_SOURCE_DIR}"
  CMAKE_ARGS -BUILD_SHARED_LIBS=OFF -DBUILD_TESTING=OFF -DCMAKE_POSITION_INDEPENDENT_CODE=True -DCMAKE_INSTALL_PREFIX=${Vc_PREFIX}

  TEST_COMMAND ""
)

include(${Vc_SOURCE_DIR}/cmake/VcMacros.cmake)

set(Vc_STATIC_LIBRARY "${Vc_PREFIX}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}Vc${CMAKE_STATIC_LIBRARY_SUFFIX}")
include_directories(SYSTEM ${Vc_PREFIX}/include)


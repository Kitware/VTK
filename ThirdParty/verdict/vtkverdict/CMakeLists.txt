set(headers
  verdict.h
  VerdictVector.hpp
  verdict_defines.hpp)

set(sources
  V_EdgeMetric.cpp
  V_GaussIntegration.cpp
  V_GaussIntegration.hpp
  V_HexMetric.cpp
  V_KnifeMetric.cpp
  V_PyramidMetric.cpp
  V_QuadMetric.cpp
  V_TetMetric.cpp
  V_TriMetric.cpp
  V_WedgeMetric.cpp
  VerdictVector.cpp)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/verdict_config.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/verdict_config.h
  @ONLY)

vtk_module_add_module(VTK::verdict
  HEADERS ${headers}
  SOURCES ${sources})

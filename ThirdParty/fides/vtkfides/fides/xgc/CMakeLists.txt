set(xgc_headers
  XGCCommon.h
)

target_sources(fides PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/XGCCommon.cxx)

viskores_add_target_information(fides
    DROP_UNUSED_SYMBOLS
    DEVICE_SOURCES XGCCommon.cxx)

target_include_directories(fides PRIVATE ${CMAKE_CURRENT_LIST_DIR})

install(FILES ${xgc_headers}
  DESTINATION ${FIDES_INSTALL_INCLUDE_DIR}/xgc)

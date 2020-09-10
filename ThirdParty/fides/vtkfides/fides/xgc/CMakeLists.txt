set(xgc_headers
  ArrayHandleXGCField.h
  ArrayHandleXGCCoords.h
  StorageXGC.h
  XGCCommon.h
)

target_sources(fides PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/XGCCommon.cxx)

target_include_directories(fides PRIVATE ${CMAKE_CURRENT_LIST_DIR})

install(FILES ${xgc_headers}
  DESTINATION ${FIDES_INSTALL_INCLUDE_DIR}/xgc)

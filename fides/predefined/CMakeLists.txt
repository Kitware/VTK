set(headers
  DataModelHelperFunctions.h
  DataModelFactory.h
  InternalMetadataSource.h
  PredefinedDataModel.h
  SupportedDataModels.h
)

target_sources(fides PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/DataModelHelperFunctions.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/DataModelFactory.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/InternalMetadataSource.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/PredefinedDataModel.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/SupportedDataModels.cxx
  )

target_include_directories(fides PRIVATE ${CMAKE_CURRENT_LIST_DIR})

install(FILES ${headers}
  DESTINATION ${FIDES_INSTALL_INCLUDE_DIR}/predefined)

add_library(fides_rapidjson INTERFACE)

fides_get_kit_name(kit_name kit_dir)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/fides_rapidjson.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/fides_rapidjson.h")

target_include_directories(fides_rapidjson INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  $<INSTALL_INTERFACE:${FIDES_INSTALL_INCLUDE_DIR}/thirdparty/rapidjson>)

if (FIDES_USE_EXTERNAL_RAPIDJSON)
  target_link_libraries(fides_rapidjson INTERFACE RapidJSON::RapidJSON)
endif()

install(TARGETS fides_rapidjson
  EXPORT ${FIDES_EXPORT_NAME})

if (NOT FIDES_USE_EXTERNAL_RAPIDJSON)
  install(DIRECTORY fidesrapidjson
    DESTINATION ${FIDES_INSTALL_INCLUDE_DIR}/${kit_dir}/)
endif()
install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/fides_rapidjson.h
  DESTINATION ${FIDES_INSTALL_INCLUDE_DIR}/${kit_dir}/)


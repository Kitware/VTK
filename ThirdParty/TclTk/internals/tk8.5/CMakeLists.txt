IF(TK_INTERNAL_PATH)
  IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
    FILE(GLOB tkHeaderFiles "*.h")
    INSTALL(FILES
      ${tkHeaderFiles}
      DESTINATION ${VTK_INSTALL_INCLUDE_DIR}/TclTk/internals/tk8.5
      COMPONENT Development
      )
  ENDIF()
ENDIF()

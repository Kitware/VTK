#
# Build the documentation
#
OPTION(BUILD_DOC "Build the Doxygen documentation." OFF)

IF (BUILD_DOC)

  #
  # Check for the mandatory tools
  #
  INCLUDE(${CMAKE_ROOT}/Modules/FindCygwin.cmake)
  INCLUDE(${CMAKE_ROOT}/Modules/FindDoxygen.cmake)
  INCLUDE(${CMAKE_ROOT}/Modules/FindHhc.cmake)
  INCLUDE(${CMAKE_ROOT}/Modules/FindPerl.cmake)
  INCLUDE(${CMAKE_ROOT}/Modules/FindWget.cmake)

  IF (HHC)
    OPTION(BUILD_DOC_HTML_HELP "Build the HTML Help file (CHM)." OFF)
  ELSE (HHC)
    SET(BUILD_DOC_HTML_HELP OFF)
  ENDIF (HHC)

  OPTION(BUILD_DOC_HTML_TARZ 
	 "Build a compressed tar archive of the HTML doc." OFF)

  FIND_PROGRAM(BASH bash)

  IF (BASH)
    IF (DOXYGEN)
      IF (MV)
        IF (PERL)
          IF (RM)

	    #
            # Configure the script and the doxyfile, then add target
            #
            CONFIGURE_FILE(
              ${VTK_SOURCE_DIR}/Utilities/Doxygen/doxyfile.in
	      ${VTK_BINARY_DIR}/Utilities/Doxygen/doxyfile)

            CONFIGURE_FILE(
              ${VTK_SOURCE_DIR}/Utilities/Doxygen/doc_makeall.sh.in
	      ${VTK_BINARY_DIR}/Utilities/Doxygen/doc_makeall.sh)

            ADD_CUSTOM_TARGET(DoxygenDoc
              ${BASH}
              ${VTK_BINARY_DIR}/Utilities/Doxygen/doc_makeall.sh)

          ENDIF (RM)
        ENDIF (PERL)
      ENDIF (MV)
    ENDIF (DOXYGEN)
  ENDIF (BASH)

ENDIF (BUILD_DOC)


# Test "make install"
if("x${CMAKE_INSTALL_PREFIX}" MATCHES "^x${VTK_BINARY_DIR}/InstallTest$")
  add_test(NAME Install
    COMMAND ${CMAKE_COMMAND} -DCONFIGURATION=$<CONFIGURATION>
                             -DVTK_BINARY_DIR=${VTK_BINARY_DIR}
                             -P ${CMAKE_CURRENT_SOURCE_DIR}/InstallTest.cmake
    )
endif()

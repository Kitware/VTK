find_program(OpenVDB_VDB_PRINT
  vdb_print vdb_print.exe
  HINTS "${OPENVDB_CMAKE_PATH}/../../../bin"
  DOC "vdb_print executable from the OpenVDB library"
  )

if (OpenVDB_VDB_PRINT)
  set(SaveOpenVDBFile_ARGS "--VDB_PRINT_EXE" "${OpenVDB_VDB_PRINT}")

  set(PY_TESTS
    NO_VALID NO_RT
    SaveOpenVDBFile.py
    )

  vtk_add_test_python(
    ${PY_TESTS}
    )
endif()

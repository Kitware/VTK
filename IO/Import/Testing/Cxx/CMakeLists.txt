file(DOWNLOAD https://www.dropbox.com/s/510ttbkyk5453uj/MiG-35.obj
              ${CMAKE_BINARY_DIR}/bin/MiG-35.obj
              EXPECTED_MD5 c2030caad41bb3e09f8d85510c3850df
              SHOW_PROGRESS TIMEOUT  20)
file(DOWNLOAD https://www.dropbox.com/s/wonunhzotw9sghr/MiG-35.obj.mtl
              ${CMAKE_BINARY_DIR}/bin/MiG-35.obj.mtl
              EXPECTED_MDF5 c775628b70ff63ea81a56e9bfe91fa62
              SHOW_PROGRESS TIMEOUT  20)

vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestOBJImporter.cxx,NO_VALID,NO_DATA,NO_OUTPUT
  TestVRMLNormals.cxx
  TestVRMLImporter.cxx,NO_VALID
  )

vtk_test_cxx_executable(${vtk-module}CxxTests tests)

if(NOT WIN32)
    add_test(Test_vtkOBJImporter ${CMAKE_BINARY_DIR}/bin/vtkIOImportCxxTests
                                 TestOBJImporter MiG-35.obj MiG-35.obj.mtl)
    set_tests_properties(Test_vtkOBJImporter PROPERTIES WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin )
endif()

project(VTKExamples)

if (VTK_BINARY_DIR)
  function (add_example dir)
    add_test(
      NAME    "VTKExample-${dir}"
      COMMAND "${CMAKE_CTEST_COMMAND}"
              --build-generator
                "${CMAKE_GENERATOR}"
              --build-and-test
                "${CMAKE_CURRENT_SOURCE_DIR}/${dir}"
                "${CMAKE_CURRENT_BINARY_DIR}/${dir}"
              --build-options
                "-DBUILD_TESTING=ON"
                "-DVTK_DIR=${vtk_cmake_build_dir}"
              --test-command
                "${CMAKE_CTEST_COMMAND}"
                  -C $<CONFIGURATION>)
  endfunction ()
else ()
  macro (add_example dir)
    add_subdirectory("${dir}")
  endmacro ()
endif ()

if (ANDROID)
  add_subdirectory(Android)
elseif (APPLE_IOS)
  add_subdirectory(iOS)
else()
  if (NOT vtkRenderingCore_LOADED)
    message(STATUS "vtkRenderingCore not found. No examples will be built")
  else()
    if (vtkTestingCore_LOADED)
      if (NOT VTK_RENDERING_BACKEND STREQUAL "None")
        add_subdirectory(AMR/Cxx)
        add_subdirectory(ImageProcessing/Cxx)
        add_subdirectory(Medical/Cxx)
        add_subdirectory(Modelling/Cxx)
        add_subdirectory(MultiBlock/Cxx)
        add_subdirectory(VisualizationAlgorithms/Cxx)
        add_subdirectory(Widgets/Cxx)
      endif ()
      add_subdirectory(IO/Cxx)
    endif()

    if (NOT VTK_RENDERING_BACKEND STREQUAL "None")
      add_subdirectory(Annotation/Cxx/LabeledMesh)
      add_subdirectory(DataManipulation/Cxx)
      add_subdirectory(Rendering/Cxx)
      add_subdirectory(Tutorial/Step1/Cxx)
      add_subdirectory(Tutorial/Step2/Cxx)
      add_subdirectory(Tutorial/Step3/Cxx)
      add_subdirectory(Tutorial/Step4/Cxx)
      add_subdirectory(Tutorial/Step5/Cxx)
      add_subdirectory(Tutorial/Step6/Cxx)
      add_subdirectory(VolumeRendering/Cxx)
    endif ()

    if(vtkParallelMPI_LOADED AND vtkRenderingParallel_LOADED)
      add_subdirectory(ParallelProcessing/Generic)
    endif()

    # This may have been already built as part of the VTK build.
    # If so we cannot create the target "vtkLocalExample" because
    # an imported target of the same name already exists in the
    # VTK build.
    if(NOT vtkLocalExample_LOADED)
      add_subdirectory(Build/vtkLocal)
    endif()
    if(TARGET vtkGUISupportQt AND vtkTestingCore_LOADED)
      add_subdirectory(GUI/Qt)
    endif()
  endif()


   add_subdirectory(Build/vtkMy)

   if (NOT VTK_RENDERING_BACKEND STREQUAL "None")
     add_subdirectory(Infovis/Cxx)
   endif ()

   if (vtkChartsCore_LOADED)
    add_subdirectory(Charts/Cxx)
   endif()
#
#  IF(VTK_USE_INFOVIS)
#    IF(VTK_USE_N_WAY_ARRAYS)
#      add_subdirectory(Array/Cxx)
#    ENDIF()
#  ENDIF()
#   IF(WIN32)
#    add_subdirectory(GUI/Win32/SimpleCxx)
#     IF(VTK_USE_MFC)
#      add_subdirectory(GUI/Win32/SampleMFC)
#      add_subdirectory(GUI/Win32/vtkMFC)
#    ENDIF()
#  ENDIF()
endif()

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
                "-DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}"
                "-DBUILD_SHARED_LIBS:BOOL=${BUILD_SHARED_LIBS}"
                "-DVTK_DIR=${vtk_cmake_build_dir}"
                "-DQt5_DIR=${Qt5_DIR}"
              --test-command
                "${CMAKE_CTEST_COMMAND}"
                  -C $<CONFIGURATION>)
    if (WIN32)
      set(example_path "$ENV{PATH};${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
      string(REPLACE ";" "\;" example_path "${example_path}")
      set_property(TEST "VTKExample-${dir}" APPEND
        PROPERTY
          ENVIRONMENT "PATH=${example_path}")
    endif ()
  endfunction ()
else ()
  macro (add_example dir)
    add_subdirectory("${dir}")
  endmacro ()
endif ()

if (ANDROID)
  #add_example(Android)
elseif (APPLE_IOS)
  #add_example(iOS)
else ()
  add_example(AMR/Cxx)
  add_example(Annotation/Cxx/LabeledMesh)
  #add_example(Array/Cxx)
  #add_example(Build/vtkLocal)
  add_example(Build/vtkMy)
  add_example(Charts/Cxx)
  add_example(DataManipulation/Cxx)
  add_example(GUI/Qt/FourPaneViewer)
  add_example(GUI/Qt/GraphicsView)
  add_example(GUI/Qt/ImageViewer)
  add_example(GUI/Qt/SimpleView)
  add_example(GUI/Qt/TouchGestureViewer)
  add_example(IO/Cxx)
  add_example(ImageProcessing/Cxx)
  add_example(Infovis/Cxx)
  add_example(Medical/Cxx)
  add_example(Modelling/Cxx)
  add_example(MultiBlock/Cxx)
  add_example(ParallelProcessing/Generic/Cxx)
  #add_example(Rendering/Cxx)
  #add_example(Tutorial/Step1/Cxx)
  #add_example(Tutorial/Step2/Cxx)
  #add_example(Tutorial/Step3/Cxx)
  #add_example(Tutorial/Step4/Cxx)
  #add_example(Tutorial/Step5/Cxx)
  #add_example(Tutorial/Step6/Cxx)
  #add_example(VisualizationAlgorithms/Cxx)
  #add_example(VolumeRendering/Cxx)
  #add_example(Widgets/Cxx)

  if (WIN32)
    #add_example(GUI/Win32/SimpleCxx)
    #add_example(GUI/Win32/SampleMFC)
    #add_example(GUI/Win32/vtkMFC)
  endif ()
endif ()

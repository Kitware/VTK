cmake_minimum_required(VERSION 3.12...3.16 FATAL_ERROR)
project(VTKExamples)

if (VTK_BINARY_DIR)
  function (add_example dir)
    if (vtk_cmake_build_dir)
      add_test(
        NAME    "VTKExample-${dir}"
        COMMAND "${CMAKE_COMMAND}"
                "-Dconfig=$<CONFIGURATION>"
                "-Dgenerator=${CMAKE_GENERATOR}"
                "-Dsource=${CMAKE_CURRENT_SOURCE_DIR}"
                "-Dbinary=${CMAKE_CURRENT_BINARY_DIR}"
                "-Dexample_dir=${dir}"
                "-Dbuild_type=${CMAKE_BUILD_TYPE}"
                "-Dshared=${BUILD_SHARED_LIBS}"
                "-Dvtk_dir=${vtk_cmake_build_dir}"
                "-Dctest=${CMAKE_CTEST_COMMAND}"
                "-Dplatform=${CMAKE_GENERATOR_PLATFORM}"
                "-Dtoolset=${CMAKE_GENERATOR_TOOLSET}"
                "-Dvtk_binary_dir=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
                -P "${CMAKE_CURRENT_LIST_DIR}/RunExample.cmake")
      set_property(TEST "VTKExample-${dir}" APPEND
        PROPERTY
          SKIP_REGULAR_EXPRESSION "Skipping example")
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
elseif (EMSCRIPTEN)
  if (CMAKE_VERSION VERSION_GREATER_EQUAL 3.13)
    add_example(Emscripten/Cxx/Cone)
    add_example(Emscripten/Cxx/ConeFullScreen)
    add_example(Emscripten/Cxx/MultiCone)
    add_example(Emscripten/Cxx/WrappedMace)
  endif()
else ()
  add_example(AMR/Cxx)
  add_example(Annotation/Cxx/LabeledMesh)
  add_example(Build/vtkMy)
  add_example(Charts/Cxx)
  add_example(DataManipulation/Cxx)
  #add_example(GUI/Cocoa) # xcode project
  add_example(GUI/Qt/FourPaneViewer)
  add_example(GUI/Qt/ImageViewer)
  add_example(GUI/Qt/SimpleView)
  add_example(GUI/Qt/TouchGestureViewer)
  add_example(GUI/Qt/MinimalQtVTKApp)
  add_example(IO/Cxx)
  add_example(ImageProcessing/Cxx)
  add_example(Infovis/Cxx)
  add_example(Medical/Cxx)
  add_example(Modelling/Cxx)
  add_example(MultiBlock/Cxx)
  add_example(ParallelProcessing/Generic/Cxx)

  add_example(Modules/UsingVTK)
  add_example(Modules/Wrapping)
endif ()

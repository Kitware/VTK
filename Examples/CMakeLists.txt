PROJECT(VTKExamples)
INCLUDE_REGULAR_EXPRESSION("^.*$")

# VTK examples now require CMake 2.4 or higher.
CMAKE_MINIMUM_REQUIRED(VERSION 2.6.3)

IF(NOT VTK_BINARY_DIR)
FIND_PACKAGE(VTK REQUIRED)
INCLUDE(${VTK_USE_FILE})
ENDIF(NOT VTK_BINARY_DIR)

# Most examples require rendering support.
IF(VTK_USE_RENDERING)
  SUBDIRS(
    Build/vtkLocal
    Build/vtkMy
    AMR/Cxx
    Annotation/Cxx/LabeledMesh
    MultiBlock/Cxx
    GUI/Motif
    DataManipulation/Cxx
    Hybrid/Cxx
    ImageProcessing/Cxx
    IO/Cxx
    Medical/Cxx
    Modelling/Cxx
    Rendering/Cxx
    Tutorial/Step1/Cxx
    Tutorial/Step2/Cxx
    Tutorial/Step3/Cxx
    Tutorial/Step4/Cxx
    Tutorial/Step5/Cxx
    Tutorial/Step6/Cxx
    VisualizationAlgorithms/Cxx
    VolumeRendering/Cxx
    Widgets/Cxx
    )
   IF(VTK_USE_PARALLEL)
    SUBDIRS(ParallelProcessing)
  ENDIF(VTK_USE_PARALLEL)
  
  IF(VTK_USE_INFOVIS)
    SUBDIRS(Charts/Cxx)
    SUBDIRS(Infovis/Cxx)
    IF(VTK_USE_N_WAY_ARRAYS)
      SUBDIRS(Array/Cxx)
    ENDIF(VTK_USE_N_WAY_ARRAYS)
  ENDIF(VTK_USE_INFOVIS)
   IF(VTK_USE_QVTK)
    SUBDIRS(GUI/Qt)
  ENDIF(VTK_USE_QVTK)
   IF(WIN32)
    SUBDIRS(GUI/Win32/SimpleCxx)
     IF(VTK_USE_MFC)
      SUBDIRS(GUI/Win32/SampleMFC)
      SUBDIRS(GUI/Win32/vtkMFC)
    ENDIF(VTK_USE_MFC)
  ENDIF(WIN32)
ENDIF(VTK_USE_RENDERING)

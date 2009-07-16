IF(NOT VTK_USE_N_WAY_ARRAYS)
  MESSAGE(SEND_ERROR "VTK_USE_TEXT_ANALYSIS requires VTK_USE_N_WAY_ARRAYS")
ENDIF(NOT VTK_USE_N_WAY_ARRAYS)

IF(NOT VTK_USE_BOOST)
  MESSAGE(SEND_ERROR "VTK_USE_TEXT_ANALYSIS requires VTK_USE_BOOST")
ENDIF(NOT VTK_USE_BOOST)

SET(KIT TextAnalysis)
SET(UKIT TEXT_ANALYSIS)

SET(KIT_TCL_LIBS
  vtkIOTCL
  vtkFilteringTCL
  )

SET(KIT_PYTHON_LIBS
  vtkIOPythonD
  vtkFilteringPythonD
  )

SET(KIT_JAVA_LIBS
  vtkIOJava
  vtkFilteringJava
  )

SET(KIT_CS_LIBS
  vtkIOCS
  vtkFilteringCS
  )

SET(KIT_LIBS
  vtkIO
  vtkFiltering
  )

SET( Kit_SRCS
vtkAssignMimeType.cxx
vtkDocumentReader.cxx
vtkDocumentTextExtraction.cxx
vtkFileExtensionMimeTypeStrategy.cxx
vtkFoldCase.cxx
vtkMimeTypes.cxx
vtkMimeTypeStrategy.cxx
vtkNGramExtraction.cxx
vtkTermDictionary.cxx
vtkTextAnalysisUtility.cxx
vtkTokenizer.cxx
vtkTokenLengthFilter.cxx
vtkTokenValueFilter.cxx
)

SET_SOURCE_FILES_PROPERTIES(
  vtkMimeTypeStrategy.cxx
  ABSTRACT
  )

SET_SOURCE_FILES_PROPERTIES(
  vtkTextAnalysisUtility.cxx
  WRAP_EXCLUDE
  )

SET(Kit_EXTRA_SRCS)
SET(Kit_EXTRA_CMDS)
SET(Kit_TCL_EXTRA_SRCS)
SET(Kit_PYTHON_EXTRA_SRCS)
SET(Kit_JAVA_EXTRA_SRCS)
SET(KIT_TCL_DEPS)
SET(KIT_PYTHON_DEPS)
SET(KIT_JAVA_DEPS)

#-----------------------------------------------------------------------------
# Include CMake code common to all kits.
INCLUDE(${VTK_CMAKE_DIR}/KitCommonBlock.cmake)
#-----------------------------------------------------------------------------

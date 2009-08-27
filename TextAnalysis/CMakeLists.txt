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
vtkFileExtensionMimeTypeStrategy.cxx
vtkFoldCase.cxx
vtkMimeTypes.cxx
vtkMimeTypeStrategy.cxx
vtkNGramExtraction.cxx
vtkPlainTextExtractionStrategy.cxx
vtkFeatureDictionary.cxx
vtkTextAnalysisUtility.cxx
vtkTextExtraction.cxx
vtkTextExtractionStrategy.cxx
vtkTokenizer.cxx
vtkTokenLengthFilter.cxx
vtkTokenValueFilter.cxx
)

SET_SOURCE_FILES_PROPERTIES(
  vtkMimeTypeStrategy.cxx
  vtkTextExtractionStrategy.cxx
  ABSTRACT
  )

SET_SOURCE_FILES_PROPERTIES(
  vtkTextAnalysisUtility.cxx
  WRAP_EXCLUDE
  )

IF(VTK_USE_QT)
  IF( (DESIRED_QT_VERSION MATCHES 4) AND (QT_VERSION_MINOR GREATER 5 OR QT_VERSION_MINOR EQUAL 5 ) )

  SET(QT_USE_QTXMLPATTERNS 1)

  SET(QT_DONT_USE_QTGUI)
  INCLUDE(${QT_USE_FILE})

  SET(Kit_SRCS
    ${Kit_SRCS}
    vtkQtXMLProcessor.cxx
    )
  
  # On the Mac, make the QT_LIBRARIES KIT_INTERFACE_LIBRARIES. This is a
  # workaround a problem with the Qt binaries from Trolltech. These libraries
  # use install names that are relative. Then when the linker tries to pull them
  # indirectly (through vtkRendering's list of libraries), it fails to find them.
  # This does not happen when Qt is build from source because it uses absolute
  # path in install names. This workaround fixes the problem by forcing all libraries
  # and executables that depend on vtkRendering to also directly link to Qt libraries.
  IF (APPLE)
    LIST (APPEND KIT_INTERFACE_LIBRARIES ${QT_LIBRARIES})
  ELSE (APPLE)
    LIST (APPEND KIT_LIBS ${QT_LIBRARIES})
  ENDIF (APPLE)
  ENDIF( (DESIRED_QT_VERSION MATCHES 4) AND (QT_VERSION_MINOR GREATER 5 OR QT_VERSION_MINOR EQUAL 5 ) )
ENDIF(VTK_USE_QT)

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

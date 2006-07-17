# ---------------------------------------------------------------------------
# VTK_GET_SOURCE_REVISION_AND_DATE
# Get vtkVTKVersion's source revision and date and store them in
# ${revision_varname} and ${date_varname} respectively.
#
# This macro can be used to require a specific revision of the VTK
# library in between version changes.
# For example:
#   INCLUDE("${VTK_CMAKE_DIR}/VTKVersionMacros.cmake")
#   VTK_GET_SOURCE_REVISION_AND_DATE(source_revision source_date)
#   IF(source_revision LESS 1.4)
#    MESSAGE(FATAL_ERROR "Sorry, your VTK library was last updated on ${source_date}. Its source revision, according to vtkVTKVersion.h, is ${source_revision}. Please update to a newer revision.")
#   ENDIF(source_revision LESS 1.4)

MACRO(VTK_GET_SOURCE_REVISION_AND_DATE
    revision_varname
    date_varname)

  SET(${revision_varname})
  SET(${date_varname})
  SET(___vtk_version_found)
  FOREACH(dir ${VTK_INCLUDE_DIRS} ${VTK_INCLUDE_PATH})
    IF(NOT ___vtk_version_found)
      SET(file "${dir}/vtkVersion.h")
      IF(EXISTS ${file})
        FILE(READ ${file} file_contents)
        STRING(REGEX REPLACE "(.*Revision: )([0-9]+\\.[0-9]+)( .*)" "\\2" 
          ${revision_varname} "${file_contents}")
        STRING(REGEX REPLACE "(.*Date: )(.+)( \\$.*)" "\\2" 
          ${date_varname} "${file_contents}")
        SET(___vtk_version_found 1)
      ENDIF(EXISTS ${file})
    ENDIF(NOT ___vtk_version_found)
  ENDFOREACH(dir)

  IF(NOT ${revision_varname} OR NOT ${date_varname})
    MESSAGE(FATAL_ERROR "Sorry, vtkVTKVersion's source revision could not be found, either because vtkVTKVersion.h is nowhere in sight, or its contents could not be parsed successfully.")
  ENDIF(NOT ${revision_varname} OR NOT ${date_varname})

ENDMACRO(VTK_GET_SOURCE_REVISION_AND_DATE)

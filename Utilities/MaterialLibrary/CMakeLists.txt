ADD_EXECUTABLE(ProcessShader ProcessShader.cxx)
TARGET_LINK_LIBRARIES(ProcessShader
  vtkCommon vtksys)

# Build the ShaderCodes Library.
SET (CgCodes
  ${CMAKE_CURRENT_SOURCE_DIR}/CgShaders/FragmentTexture.cg
  ${CMAKE_CURRENT_SOURCE_DIR}/CgShaders/VertTexPassThrough.cg
  ${CMAKE_CURRENT_SOURCE_DIR}/CgShaders/VertPassThrough.cg
  ${CMAKE_CURRENT_SOURCE_DIR}/CgShaders/FragmentIsotropicTorranceSparrow.cg
  ${CMAKE_CURRENT_SOURCE_DIR}/CgShaders/cg_sinewave.cg
  )

SET (GLSLCodes
  ${CMAKE_CURRENT_SOURCE_DIR}/GLSLShaders/TestAppVarFrag.glsl
  ${CMAKE_CURRENT_SOURCE_DIR}/GLSLShaders/TestVertex.glsl
  ${CMAKE_CURRENT_SOURCE_DIR}/GLSLShaders/TestVtkPropertyFrag.glsl
  ${CMAKE_CURRENT_SOURCE_DIR}/GLSLShaders/TestMatrixFrag.glsl
  ${CMAKE_CURRENT_SOURCE_DIR}/GLSLShaders/TestScalarVectorFrag.glsl
  )

SET(CMD ${EXECUTABLE_OUTPUT_PATH}${CFG_INIT}/ProcessShader${EXE_EXT})

SET (ShaderLibraryDependencies)
SET (ShaderLibraryHeaders)
SET (ShaderLibraryModules)

# Macro to obtain the Module names from filenames.
MACRO(get_modules modules prefix files )
  FOREACH(file ${files})
    STRING(REGEX REPLACE ".*[\\\\/]([^./\\\\]+)\\.(cg|glsl|xml)$" "\\1" module "${file}")
    SET(${modules} ${${modules}} "${prefix}${module}")
  ENDFOREACH(file)
ENDMACRO(get_modules)

IF (VTK_USE_CG_SHADERS)
  # Create the Cg library.
  ADD_CUSTOM_COMMAND(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/vtkCgShaderLibrary.h
    DEPENDS ${CgCodes}
    ProcessShader
    COMMAND ${CMD}
    ARGS ${CMAKE_CURRENT_BINARY_DIR}/vtkCgShaderLibrary.h
    vtkShaderCG Code GetCode
    ${CgCodes})
  
  SET (ShaderLibraryHeaders ${ShaderLibraryHeaders}
    vtkCgShaderLibrary.h)
  SET (ShaderLibraryDependencies ${ShaderLibraryDependencies}
    ${CMAKE_CURRENT_BINARY_DIR}/vtkCgShaderLibrary.h)

  get_modules(ShaderLibraryModules "CG" "${CgCodes}")
ENDIF (VTK_USE_CG_SHADERS)

IF (VTK_USE_GLSL_SHADERS)
  # Create the GLSL library
  ADD_CUSTOM_COMMAND(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/vtkGLSLShaderLibrary.h
    DEPENDS ${GLSLCodes}
    ProcessShader
    COMMAND ${CMD}
    ARGS ${CMAKE_CURRENT_BINARY_DIR}/vtkGLSLShaderLibrary.h
    vtkShaderGLSL Code GetCode
    ${GLSLCodes})
  SET (ShaderLibraryHeaders ${ShaderLibraryHeaders}
    vtkGLSLShaderLibrary.h)
  SET (ShaderLibraryDependencies ${ShaderLibraryDependencies}
    ${CMAKE_CURRENT_BINARY_DIR}/vtkGLSLShaderLibrary.h)

  get_modules(ShaderLibraryModules "GLSL" "${GLSLCodes}")
ENDIF (VTK_USE_GLSL_SHADERS)

# Write the vtkShaderCodeLibraryMacro header file.
SET (VTK_SHADER_CODE_LIBRARY_MACRO)
FOREACH(file ${ShaderLibraryHeaders})
  SET (VTK_SHADER_CODE_LIBRARY_MACRO 
    "${VTK_SHADER_CODE_LIBRARY_MACRO}#include \"${file}\"\n")
ENDFOREACH(file)

SET (VTK_SHADER_CODE_LIBRARY_MACRO "${VTK_SHADER_CODE_LIBRARY_MACRO}
#define vtkShaderCodeLibraryMacro(name) \\\n")
FOREACH (module ${ShaderLibraryModules})
  # get the module name.
  SET(VTK_SHADER_CODE_LIBRARY_MACRO 
    "${VTK_SHADER_CODE_LIBRARY_MACRO} if (strcmp(name, \"${module}\") == 0)\\
        {\\
        return vtkShader${module}GetCode()\;\\
        }\\\n")
ENDFOREACH(module)

WRITE_FILE(${CMAKE_CURRENT_BINARY_DIR}/vtkShaderCodeLibraryMacro.h
  ${VTK_SHADER_CODE_LIBRARY_MACRO})

# Build the Material library.
SET (MaterialXMLs)

IF (VTK_USE_CG_SHADERS)
  SET (MaterialXMLs ${MaterialXMLs}
    ${CMAKE_CURRENT_SOURCE_DIR}/Materials/CgSinewave.xml
    )
ENDIF (VTK_USE_CG_SHADERS)

IF (VTK_USE_GLSL_SHADERS)
  SET (MaterialXMLs ${MaterialXMLs}
    )
ENDIF (VTK_USE_GLSL_SHADERS)

ADD_CUSTOM_COMMAND(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/vtkMaterialXMLLibrary.h
  DEPENDS ${MaterialXMLs}
    ProcessShader
  COMMAND ${CMD}
  ARGS ${CMAKE_CURRENT_BINARY_DIR}/vtkMaterialXMLLibrary.h
  vtkMaterial XML GetXML
  ${MaterialXMLs})

# Write the MaterialLibraryMacro header file.
SET (VTK_MATERIAL_LIBRARY_MACRO "#include \"vtkMaterialXMLLibrary.h\"\n")
SET (VTK_MATERIAL_LIBRARY_MACRO "${VTK_MATERIAL_LIBRARY_MACRO}
#define vtkMaterialLibraryMacro(name) \\\n")

SET (MaterialModules)
get_modules(MaterialModules "" "${MaterialXMLs}")
FOREACH(module ${MaterialModules})
  SET (VTK_MATERIAL_LIBRARY_MACRO
    "${VTK_MATERIAL_LIBRARY_MACRO} if (strcmp(name, \"${module}\") == 0)\\
      {\\
      return vtkMaterial${module}GetXML()\;\\
      }\\\n")
ENDFOREACH(module)

WRITE_FILE(${CMAKE_CURRENT_BINARY_DIR}/vtkMaterialLibraryMacro.h
  ${VTK_MATERIAL_LIBRARY_MACRO})


# Since we have configured header files that
# this library needs.
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

ADD_LIBRARY(vtkMaterialLibrary
  vtkShaderCodeLibrary.cxx
  vtkMaterialLibrary.cxx)

TARGET_LINK_LIBRARIES(vtkMaterialLibrary
  vtkCommon)

# The target vtkMaterialLibrary requires that the header files
# with the shader/material code have been written out.
ADD_DEPENDENCIES(vtkMaterialLibrary
  ${ShaderLibraryDependencies}
  ${CMAKE_CURRENT_BINARY_DIR}/vtkShaderCodeLibraryMacro.h
  ${CMAKE_CURRENT_BINARY_DIR}/vtkMaterialXMLLibrary.h
  ${CMAKE_CURRENT_BINARY_DIR}/vtkMaterialLibraryMacro.h)

IF (NOT VTK_INSTALL_NO_LIBRARIES)
  INSTALL_TARGETS(${VTK_INSTALL_LIB_DIR} vtkMaterialLibrary)
ENDIF (NOT VTK_INSTALL_NO_LIBRARIES)

SET(VTK_MATERIAL_LIBRARY_INCLUDE_PATH ${CMAKE_CURRENT_SOURCE_DIR}
  CACHE INTERNAL "Include path for vtkMaterialLibrary.h and vtkShaderCodeLibrary.h")


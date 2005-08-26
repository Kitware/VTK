PROJECT(MaterialLibrary)

# TODO: Handle no RPATH support.
ADD_EXECUTABLE(ProcessShader ProcessShader.cxx)
TARGET_LINK_LIBRARIES(ProcessShader
  vtksys)

# Build the ShaderCodes Library.
SET (CgCodes
  ${MaterialLibrary_SOURCE_DIR}/CgShaders/FragmentTexture.cg
  ${MaterialLibrary_SOURCE_DIR}/CgShaders/VertTexPassThrough.cg
  ${MaterialLibrary_SOURCE_DIR}/CgShaders/VertPassThrough.cg
  ${MaterialLibrary_SOURCE_DIR}/CgShaders/FragmentIsotropicTorranceSparrow.cg
  ${MaterialLibrary_SOURCE_DIR}/CgShaders/cg_sinewave.cg
  )

SET (GLSLCodes
  ${MaterialLibrary_SOURCE_DIR}/GLSLShaders/TestAppVarFrag.glsl
  ${MaterialLibrary_SOURCE_DIR}/GLSLShaders/TestVertex.glsl
  ${MaterialLibrary_SOURCE_DIR}/GLSLShaders/TestVtkPropertyFrag.glsl
  ${MaterialLibrary_SOURCE_DIR}/GLSLShaders/TestMatrixFrag.glsl
  ${MaterialLibrary_SOURCE_DIR}/GLSLShaders/TestScalarVectorFrag.glsl
  )

SET(CFG_INIT "/${CMAKE_CFG_INTDIR}")
IF(MAKE_SYSTEM OR UNIX)
  SET(CFG_INIT "")
ENDIF(MAKE_SYSTEM OR UNIX)
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
    OUTPUT ${MaterialLibrary_BINARY_DIR}/vtkCgShaderLibrary.h
    DEPENDS ${CgCodes}
    ProcessShader
    COMMAND ${CMD}
    ARGS ${MaterialLibrary_BINARY_DIR}/vtkCgShaderLibrary.h
    vtkShaderCG Code GetCode
    ${CgCodes})
  
  SET (ShaderLibraryHeaders ${ShaderLibraryHeaders}
    vtkCgShaderLibrary.h)
  SET (ShaderLibraryDependencies ${ShaderLibraryDependencies}
    ${MaterialLibrary_BINARY_DIR}/vtkCgShaderLibrary.h)

  get_modules(ShaderLibraryModules "CG" "${CgCodes}")
ENDIF (VTK_USE_CG_SHADERS)

IF (VTK_USE_GLSL_SHADERS)
  # Create the GLSL library
  ADD_CUSTOM_COMMAND(
    OUTPUT ${MaterialLibrary_BINARY_DIR}/vtkGLSLShaderLibrary.h
    DEPENDS ${GLSLCodes}
    ProcessShader
    COMMAND ${CMD}
    ARGS ${MaterialLibrary_BINARY_DIR}/vtkGLSLShaderLibrary.h
    vtkShaderGLSL Code GetCode
    ${GLSLCodes})
  SET (ShaderLibraryHeaders ${ShaderLibraryHeaders}
    vtkGLSLShaderLibrary.h)
  SET (ShaderLibraryDependencies ${ShaderLibraryDependencies}
    ${MaterialLibrary_BINARY_DIR}/vtkGLSLShaderLibrary.h)

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

WRITE_FILE(${MaterialLibrary_BINARY_DIR}/vtkShaderCodeLibraryMacro.h
  ${VTK_SHADER_CODE_LIBRARY_MACRO})

# Build the Material library.
SET (MaterialXMLs)

IF (VTK_USE_CG_SHADERS)
  # Add Materials using Cg Shaders.
  SET (MaterialXMLs ${MaterialXMLs}
    ${MaterialLibrary_SOURCE_DIR}/Materials/CgSinewave.xml
    )
ENDIF (VTK_USE_CG_SHADERS)

IF (VTK_USE_GLSL_SHADERS)
  # Add Materials using GLSL Shaders.
  SET (MaterialXMLs ${MaterialXMLs}
    )
ENDIF (VTK_USE_GLSL_SHADERS)

# Add the Materials that don't use Cg/GLSL.
SET (Materials ${MaterialXMLs}
  )

ADD_CUSTOM_COMMAND(
  OUTPUT ${MaterialLibrary_BINARY_DIR}/vtkMaterialXMLLibrary.h
  DEPENDS ${MaterialXMLs}
    ProcessShader
  COMMAND ${CMD}
  ARGS ${MaterialLibrary_BINARY_DIR}/vtkMaterialXMLLibrary.h
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

WRITE_FILE(${MaterialLibrary_BINARY_DIR}/vtkMaterialLibraryMacro.h
  ${VTK_MATERIAL_LIBRARY_MACRO})

ADD_CUSTOM_TARGET(vtkMaterialLibraryConfiguredFiles ALL DEPENDS
  ${ShaderLibraryDependencies}
  ${MaterialLibrary_BINARY_DIR}/vtkMaterialXMLLibrary.h)

ADD_DEPENDENCIES(vtkMaterialLibraryConfiguredFiles
  ${MaterialLibrary_BINARY_DIR}/vtkShaderCodeLibraryMacro.h
  ${MaterialLibrary_BINARY_DIR}/vtkMaterialLibraryMacro.h)
 
SET(MATERIAL_LIBRARY_SHADER_MACRO_HEADER
  ${MaterialLibrary_BINARY_DIR}/vtkShaderCodeLibraryMacro.h
  CACHE INTERNAL "Path to vtkShaderCodeLibraryMacro.h")

SET(MATERIAL_LIBRARY_MATERIAL_MACRO_HEADER
  ${MaterialLibrary_BINARY_DIR}/vtkMaterialLibraryMacro.h
  CACHE INTERNAL "Path to vtkMaterialLibraryMacro.h")
  


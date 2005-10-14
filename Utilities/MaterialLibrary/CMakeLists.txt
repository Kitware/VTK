PROJECT(MaterialLibrary)

# Enable shared link forwarding support it needed.
SET (ML_EXE_SUFFIX)
SET (ML_EXE_INSTALL ${VTK_INSTALL_BIN_DIR})
IF(BUILD_SHARED_LIBS AND CMAKE_SKIP_RPATH AND NOT WIN32)
  SET (NEED_SHARED_FORWARD 1)
  SET (ML_EXE_SUFFIX -real)
  SET (ML_EXE_INSTALL ${VTK_INSTALL_LIB_DIR})
  SET (ML_FORWARD_DIR_BUILD "${EXECUTABLE_OUTPUT_PATH}")
  SET (ML_FORWARD_DIR_INSTALL "..${ML_EXE_INSTALL}")
  SET (ML_FORWARD_PATH_BUILD "\"${ML_FORWARD_DIR_BUILD}\"")
  SET (ML_FORWARD_PATH_INSTALL "\"${ML_FORWARD_DIR_INSTALL}\"")
ENDIF(BUILD_SHARED_LIBS AND CMAKE_SKIP_RPATH AND NOT WIN32)


ADD_EXECUTABLE(ProcessShader${ML_EXE_SUFFIX} ProcessShader.cxx)
TARGET_LINK_LIBRARIES(ProcessShader${ML_EXE_SUFFIX}
  vtksys)

# Add shared link forwarding executable if necessary.
IF (NEED_SHARED_FORWARD)
  SET (ML_FORWARD_EXE ProcessShader${ML_EXE_SUFFIX})
  CONFIGURE_FILE(
    ${MaterialLibrary_SOURCE_DIR}/vtk-forward.c.in
    ${MaterialLibrary_BINARY_DIR}/ProcessShader-forward.c
    @ONLY IMMEDIATE)
  ADD_EXECUTABLE(ProcessShader ${MaterialLibrary_BINARY_DIR}/ProcessShader-forward.c)
  ADD_DEPENDENCIES(ProcessShader ProcessShader${ML_EXE_SUFFIX})
ENDIF (NEED_SHARED_FORWARD)

#Macro to copy files over
MACRO(COPY_FILES_TO_DIR src_dir target_dir)
  IF (NOT EXISTS ${target_dir})
    MAKE_DIRECTORY(${target_dir})
  ENDIF(NOT EXISTS ${target_dir})

  FILE (GLOB files_to_copy1 ${src_dir}/*.*)
  FOREACH(file ${files_to_copy1})
    GET_FILENAME_COMPONENT(file_name "${file}" NAME)
    CONFIGURE_FILE(${file} "${target_dir}/${file_name}" COPYONLY IMMEDIATE)
  ENDFOREACH(file)
ENDMACRO(COPY_FILES_TO_DIR)

# Shaders/Materials provided in Utilities/MaterialLibrary/Repository
# are copied over to the binary dir.
IF (NOT "${VTK_BINARY_DIR}" MATCHES "^${VTK_SOURCE_DIR}$")
  # Ensure that the Source and Build dir are not same.
  # In that case we don't need to copy anything.
  COPY_FILES_TO_DIR(${MaterialLibrary_SOURCE_DIR}/Repository
    ${VTK_DEFAULT_SHADERS_DIR})

  # copy materials and shaders that test VTK-Cg interface
  # copy the directory stucture (not just the files) to avoid name clashes
  COPY_FILES_TO_DIR(${MaterialLibrary_SOURCE_DIR}/Repository/TestCg
    ${VTK_DEFAULT_SHADERS_DIR}/TestCg)

  # copy materials and shaders that test VTK-GLSL interface
  # copy the directory stucture (not just the files) to avoid name clashes
  COPY_FILES_TO_DIR(${MaterialLibrary_SOURCE_DIR}/Repository/TestGLSL
    ${VTK_DEFAULT_SHADERS_DIR}/TestGLSL)
ENDIF (NOT "${VTK_BINARY_DIR}" MATCHES "^${VTK_SOURCE_DIR}$")

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
        return vtkShader${module}GetCode();\\
        }\\\n")
ENDFOREACH(module)

CONFIGURE_FILE(${MaterialLibrary_SOURCE_DIR}/vtkShaderCodeLibraryMacro.h.in
  ${MaterialLibrary_BINARY_DIR}/vtkShaderCodeLibraryMacro.h
  @ONLY)

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

CONFIGURE_FILE(
  ${MaterialLibrary_SOURCE_DIR}/vtkMaterialXMLLibrary.h.forcebuild.in
  ${MaterialLibrary_BINARY_DIR}/vtkMaterialXMLLibrary.h.forcebuild
  @ONLY)
  

ADD_CUSTOM_COMMAND(
  OUTPUT ${MaterialLibrary_BINARY_DIR}/vtkMaterialXMLLibrary.h
  DEPENDS ${MaterialXMLs} 
    ProcessShader
    ${MaterialLibrary_BINARY_DIR}/vtkMaterialXMLLibrary.h.forcebuild
  COMMAND ${CMD}
  ARGS ${MaterialLibrary_BINARY_DIR}/vtkMaterialXMLLibrary.h
  vtkMaterial XML GetXML
  ${MaterialXMLs}
  )

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
      return vtkMaterial${module}GetXML();\\
      }\\\n")
ENDFOREACH(module)

CONFIGURE_FILE(
  ${MaterialLibrary_SOURCE_DIR}/vtkMaterialLibraryMacro.h.in
  ${MaterialLibrary_BINARY_DIR}/vtkMaterialLibraryMacro.h)

ADD_CUSTOM_TARGET(vtkMaterialLibraryConfiguredFiles ALL DEPENDS
  ${ShaderLibraryDependencies}
  ${MaterialLibrary_BINARY_DIR}/vtkMaterialXMLLibrary.h
  ${MaterialLibrary_BINARY_DIR}/vtkShaderCodeLibraryMacro.h
  ${MaterialLibrary_BINARY_DIR}/vtkMaterialLibraryMacro.h)
 
SET(MATERIAL_LIBRARY_SHADER_MACRO_HEADER
  ${MaterialLibrary_BINARY_DIR}/vtkShaderCodeLibraryMacro.h
  CACHE INTERNAL "Path to vtkShaderCodeLibraryMacro.h")

SET(MATERIAL_LIBRARY_MATERIAL_MACRO_HEADER
  ${MaterialLibrary_BINARY_DIR}/vtkMaterialLibraryMacro.h
  CACHE INTERNAL "Path to vtkMaterialLibraryMacro.h")
  


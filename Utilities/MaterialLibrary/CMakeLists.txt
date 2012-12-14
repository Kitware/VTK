PROJECT(MaterialLibrary)

vtk_module_export_info()

# FIXME: Provide a reasonable default for this value in MaterialLibrary?
set(VTK_DEFAULT_SHADERS_DIR
  "${VTK_BINARY_DIR}/Utilities/MaterialLibrary/Repository"
  CACHE INTERNAL
  "The directory that code for shaders is stored in.")
set(VTK_MATERIALS_DIRS
  ${VTK_DEFAULT_SHADERS_DIR}
  CACHE STRING
  "A ; separated list of directories to search for materials/shaders.")
mark_as_advanced(VTK_MATERIALS_DIRS)

include_directories("${VTK_SOURCE_DIR}/Common/Core"
  "${VTK_BINARY_DIR}/Common/Core")

IF(NOT CMAKE_CROSSCOMPILING)
  ADD_EXECUTABLE(ProcessShader ProcessShader.cxx)
  TARGET_LINK_LIBRARIES(ProcessShader vtksys)
  vtk_compile_tools_target(ProcessShader)
ENDIF(NOT CMAKE_CROSSCOMPILING)

# Macro to obtain the relative path of a file.
MACRO(GET_RELATIVE_PATH __path __file __result)
  STRING(REGEX REPLACE "/" ";" __dirfilelist "${__file}")
  SET(__found)
  SET(__cfile "${__file}")
  SET(__res "")
  FOREACH(__dir ${__dirfilelist})
    IF(NOT __found)
      STRING(REGEX REPLACE ".*/(.*)" "\\1" __lineres "${__cfile}")
      STRING(REGEX REPLACE "(.*)/.*" "\\1" __cfile "${__cfile}")
      SET(__res "${__lineres}/${__res}")
      STRING(COMPARE EQUAL "${__cfile}" "${__path}" __flag)
      IF(__flag)
        SET(__found 1)
      ENDIF(__flag)
    ENDIF(NOT __found)
  ENDFOREACH(__dir)
  STRING(REGEX REPLACE "/$" "" __res "${__res}")
  #MESSAGE("Dir: ${__cfile}")
  #MESSAGE("File: ${__res}")
  SET(${__result} "${__res}")
ENDMACRO(GET_RELATIVE_PATH)

#Macro to copy files over. Copies the entire subtree.
MACRO(COPY_FILES_TO_DIR src_dir target_dir)
  IF (NOT EXISTS ${target_dir})
    FILE(MAKE_DIRECTORY ${target_dir})
  ENDIF(NOT EXISTS ${target_dir})

  FILE (GLOB_RECURSE files_to_copy1 ${src_dir}/*.*)

  FOREACH (src_file ${files_to_copy1})
    GET_RELATIVE_PATH( ${src_dir} ${src_file} rel_file )
    GET_FILENAME_COMPONENT(src_path "${rel_file}" PATH)
    GET_FILENAME_COMPONENT(filename "${rel_file}" NAME)
    STRING(REGEX REPLACE "/" ";" dirlist "${src_path}")

    # Ensure that the tree structure is identical on the destination side.
    SET (path ${target_dir})
    FOREACH(subdir ${dirlist})
      SET(path "${path}/${subdir}")
      IF (NOT EXISTS ${path})
        FILE(MAKE_DIRECTORY ${path})
      ENDIF (NOT EXISTS ${path})
    ENDFOREACH(subdir)

    # Now configure the file over.
    CONFIGURE_FILE(${src_file} "${path}/${filename}" COPYONLY IMMEDIATE)
  ENDFOREACH (src_file)
ENDMACRO(COPY_FILES_TO_DIR)

# Shaders/Materials provided in Utilities/MaterialLibrary/Repository
# are copied over to the binary dir.
IF (NOT "${VTK_BINARY_DIR}" MATCHES "^${VTK_SOURCE_DIR}$")
  # Ensure that the Source and Build dir are not same.
  # In that case we don't need to copy anything.
  COPY_FILES_TO_DIR(${MaterialLibrary_SOURCE_DIR}/Repository
    ${VTK_DEFAULT_SHADERS_DIR})

ENDIF (NOT "${VTK_BINARY_DIR}" MATCHES "^${VTK_SOURCE_DIR}$")

# Build the ShaderCodes Library.
SET (CgCodes
  ${MaterialLibrary_SOURCE_DIR}/CgShaders/FragmentTexture.cg
  ${MaterialLibrary_SOURCE_DIR}/CgShaders/VertTexPassThrough.cg
  ${MaterialLibrary_SOURCE_DIR}/CgShaders/VertPassThrough.cg
  ${MaterialLibrary_SOURCE_DIR}/CgShaders/FragmentIsotropicTorranceSparrow.cg
  ${MaterialLibrary_SOURCE_DIR}/CgShaders/cg_sinewave.cg
  ${MaterialLibrary_SOURCE_DIR}/CgShaders/VertexLighting.cg
  ${MaterialLibrary_SOURCE_DIR}/CgShaders/Twisted.cg
  )

SET (GLSLCodes
  ${MaterialLibrary_SOURCE_DIR}/GLSLShaders/TestAppVarFrag.glsl
  ${MaterialLibrary_SOURCE_DIR}/GLSLShaders/TestVertex.glsl
  ${MaterialLibrary_SOURCE_DIR}/GLSLShaders/TestVtkPropertyFrag.glsl
  ${MaterialLibrary_SOURCE_DIR}/GLSLShaders/TestMatrixFrag.glsl
  ${MaterialLibrary_SOURCE_DIR}/GLSLShaders/TestScalarVectorFrag.glsl
  ${MaterialLibrary_SOURCE_DIR}/GLSLShaders/Twisted.glsl
  )

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

SET(CMD_DEP ProcessShader)
SET(CMD ProcessShader)

IF (VTK_USE_CG_SHADERS)
  # Create the Cg library.
  ADD_CUSTOM_COMMAND(
    OUTPUT ${MaterialLibrary_BINARY_DIR}/vtkCgShaderLibrary.h
    DEPENDS ${CgCodes}
    ${CMD_DEP}
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
    ${CMD_DEP}
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
SET (VTK_SHADER_CODE_LIBRARY_LIST)
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
  SET(VTK_SHADER_CODE_LIBRARY_LIST
    "${VTK_SHADER_CODE_LIBRARY_LIST}
    \"${module}\",")
ENDFOREACH(module)

CONFIGURE_FILE(${MaterialLibrary_SOURCE_DIR}/vtkShaderCodeLibraryMacro.h.in
  ${MaterialLibrary_BINARY_DIR}/vtkShaderCodeLibraryMacro.h
  @ONLY)

# Build the Material library.
SET (MaterialXMLs)

IF (VTK_USE_CG_SHADERS)
# # Add Materials using Cg Shaders.
  SET (MaterialXMLs ${MaterialXMLs}
    ${MaterialLibrary_SOURCE_DIR}/Materials/CgSinewave.xml
    ${MaterialLibrary_SOURCE_DIR}/Materials/CgLighting.xml
    ${MaterialLibrary_SOURCE_DIR}/Materials/CgTwisted.xml
    )
ENDIF (VTK_USE_CG_SHADERS)

IF (VTK_USE_GLSL_SHADERS)
  # Add Materials using GLSL Shaders.
  SET (MaterialXMLs ${MaterialXMLs}
    ${MaterialLibrary_SOURCE_DIR}/Materials/GLSLTwisted.xml
    )
ENDIF (VTK_USE_GLSL_SHADERS)

# Add the Materials that don't use Cg/GLSL.
SET (Materials ${MaterialXMLs}
  )

CONFIGURE_FILE(
  ${MaterialLibrary_SOURCE_DIR}/vtkMaterialXMLLibrary.h.forcebuild.in
  ${MaterialLibrary_BINARY_DIR}/vtkMaterialXMLLibrary.h.forcebuild
  @ONLY)

SET (VTK_MATERIAL_LIBRARY_MACRO "")
# materials_exist is variable to indicate if there are any materials.
# if not we can reduce our work by not trying to build the headers at
# all.
SET (materials_exist 0)
FOREACH(material ${Materials})
  SET (materials_exist 1)
ENDFOREACH(material ${Materials})

SET(vtkMaterialLibraryConfiguredFilesDependencies
  ${ShaderLibraryDependencies}
  ${MaterialLibrary_BINARY_DIR}/vtkShaderCodeLibraryMacro.h
  ${MaterialLibrary_BINARY_DIR}/vtkMaterialLibraryMacro.h)

IF (${materials_exist})
  ADD_CUSTOM_COMMAND(
    OUTPUT ${MaterialLibrary_BINARY_DIR}/vtkMaterialXMLLibrary.h
    DEPENDS ${MaterialXMLs}
      ${CMD_DEP}
      ${MaterialLibrary_BINARY_DIR}/vtkMaterialXMLLibrary.h.forcebuild
    COMMAND ${CMD}
    ARGS ${MaterialLibrary_BINARY_DIR}/vtkMaterialXMLLibrary.h
    vtkMaterial XML GetXML
    ${MaterialXMLs}
    )

  SET (VTK_MATERIAL_LIBRARY_MACRO "#include \"vtkMaterialXMLLibrary.h\"\n")

  # Add a dependency on the vtkMaterialXMLLibrary.h file for the target that generates all
  # the configured files.
  SET(vtkMaterialLibraryConfiguredFilesDependencies
    ${vtkMaterialLibraryConfiguredFilesDependencies}
    ${MaterialLibrary_BINARY_DIR}/vtkMaterialXMLLibrary.h)
ENDIF (${materials_exist})

# Write the MaterialLibraryMacro header file.
SET (VTK_MATERIAL_LIBRARY_LIST)
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
  SET (VTK_MATERIAL_LIBRARY_LIST
    "${VTK_MATERIAL_LIBRARY_LIST}
    \"${module}\",")
ENDFOREACH(module)

CONFIGURE_FILE(
  ${MaterialLibrary_SOURCE_DIR}/vtkMaterialLibraryMacro.h.in
  ${MaterialLibrary_BINARY_DIR}/vtkMaterialLibraryMacro.h)

ADD_CUSTOM_TARGET(vtkMaterialLibraryConfiguredFiles ALL DEPENDS
  ${vtkMaterialLibraryConfiguredFilesDependencies}
  )


SET(MATERIAL_LIBRARY_SHADER_MACRO_HEADER
  ${MaterialLibrary_BINARY_DIR}/vtkShaderCodeLibraryMacro.h
  CACHE INTERNAL "Path to vtkShaderCodeLibraryMacro.h")

SET(MATERIAL_LIBRARY_MATERIAL_MACRO_HEADER
  ${MaterialLibrary_BINARY_DIR}/vtkMaterialLibraryMacro.h
  CACHE INTERNAL "Path to vtkMaterialLibraryMacro.h")



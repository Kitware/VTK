# Create an executable to encode source files into C string literals.
ADD_EXECUTABLE(vtkVREncodeString
  ${VTK_SOURCE_DIR}/VolumeRendering/vtkVREncodeString.cxx
  )
GET_TARGET_PROPERTY(ENCODE_EXE vtkVREncodeString LOCATION)

# The set of source files to be encoded.
SET(asm_files
  vtkVolumeTextureMapper3D_FourDependentNoShadeFP
  vtkVolumeTextureMapper3D_FourDependentShadeFP
  vtkVolumeTextureMapper3D_OneComponentNoShadeFP
  vtkVolumeTextureMapper3D_OneComponentShadeFP
  vtkVolumeTextureMapper3D_TwoDependentNoShadeFP
  vtkVolumeTextureMapper3D_TwoDependentShadeFP
  )

# Create custom commands to encode each assembly file into a C string
# literal in a header file.
SET(ASM_HEADERS)
FOREACH(file ${asm_files})
  SET(src ${VTK_SOURCE_DIR}/VolumeRendering/${file}.asm)
  SET(res ${VTK_BINARY_DIR}/VolumeRendering/${file}.h)
  ADD_CUSTOM_COMMAND(
    OUTPUT ${res}
    DEPENDS ${src} vtkVREncodeString
    COMMAND ${ENCODE_EXE}
    ARGS ${res} ${src} ${file}
    )
  SET(ASM_HEADERS ${ASM_HEADERS} ${res})
ENDFOREACH(file)

# Add dependencies to drive the above custom commands.  See comments
# in VTK/VolumeRendering/CMakeLists.txt near where this file is
# included for details.
IF(VTKVRASM_SOURCE_DIR)
  # We are being included from the VTKVRASM project.  Drive the custom
  # commands with a custom target.
  ADD_CUSTOM_TARGET(vtk_vr_generate_asm_headers ALL DEPENDS ${ASM_HEADERS})
ELSE(VTKVRASM_SOURCE_DIR)
  # We are being included from the main VTK project.  Just add the
  # dependencies to the file that includes the generated headers.
  SET_SOURCE_FILES_PROPERTIES(vtkOpenGLVolumeTextureMapper3D.cxx
    PROPERTIES OBJECT_DEPENDS "${ASM_HEADERS}")
ENDIF(VTKVRASM_SOURCE_DIR)

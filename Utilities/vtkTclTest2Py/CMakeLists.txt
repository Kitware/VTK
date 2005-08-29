# Handle out-of-source builds correctly.
#
#  1. Create a list of Python files to be installed/copied.
#  2. Copy them to VTK_BINARY_DIR if it is different from VTK_SOURCE_DIR.
#  3. Use Python's compileall to compile the copied files.
#
# *** Step 1 has to be done carefully to avoid missing out files ***

IF(PYTHON_EXECUTABLE AND VTK_WRAP_PYTHON)
  SET(CMD ${PYTHON_EXECUTABLE})

  # Make the necessary directories.
  FOREACH(dir
    ${VTK_BINARY_DIR}/Utilities/vtkTclTest2Py
    )
    MAKE_DIRECTORY(${dir})
  ENDFOREACH(dir)

  # Now create a list of Python files.
  # tcl2py testing related files. These are for testing tcl-translated scripts.
  SET(VTK_PYTHON_FILES
    __init__
    vtkTclParser
    vtkTclToPyConvertor
    backdrop
    catch
    expr
    file
    info
    mccases
    rtImageTest
    )
  # Done listing of files.

  # Now copy these files if necessary.
  SET(VTK_PYTHON_SOURCE_FILES)
  SET(VTK_PYTHON_OUTPUT_FILES)
  FOREACH(file ${VTK_PYTHON_FILES})
    SET(src "${VTK_BINARY_DIR}/Utilities/vtkTclTest2Py/${file}.py")
    SET(VTK_PYTHON_SOURCE_FILES ${VTK_PYTHON_SOURCE_FILES} ${src})
  ENDFOREACH(file)

  IF ("${VTK_BINARY_DIR}" MATCHES "^${VTK_SOURCE_DIR}$")
    #MESSAGE("In source build -- no need to copy Python files.")
  ELSE ("${VTK_BINARY_DIR}" MATCHES "^${VTK_SOURCE_DIR}$")
    FOREACH(file ${VTK_PYTHON_FILES})
      SET(src "${VTK_SOURCE_DIR}/Utilities/vtkTclTest2Py/${file}.py")
      SET(tgt "${VTK_BINARY_DIR}/Utilities/vtkTclTest2Py/${file}.py")
      ADD_CUSTOM_COMMAND(DEPENDS ${src}
                         COMMAND ${CMAKE_COMMAND}
                         ARGS -E copy ${src} ${tgt}
                         OUTPUT ${tgt}
                         COMMENT "source copy")
    ENDFOREACH(file)
  ENDIF ("${VTK_BINARY_DIR}" MATCHES "^${VTK_SOURCE_DIR}$")

  # Byte compile the Python files.
  CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/compile_all_vtk.py.in
    ${CMAKE_CURRENT_BINARY_DIR}/compile_all_vtk.py
    @ONLY IMMEDIATE)

  ADD_CUSTOM_COMMAND(
    COMMAND ${CMD}
    ARGS -- ${VTK_BINARY_DIR}/Utilities/vtkTclTest2Py/compile_all_vtk.py
      -A "${VTK_BINARY_DIR}/Wrapping/Python"
    DEPENDS ${VTK_PYTHON_SOURCE_FILES} ${VTK_BINARY_DIR}/Utilities/vtkTclTest2Py/compile_all_vtk.py vtkpython
    OUTPUT "${VTK_BINARY_DIR}/Utilities/vtkTclTest2Py/vtk_compile_complete"
    )

  #ADD_DEPENDENCIES(vtktcltest2py_pyc vtkpython_pyc)

  # Create a python module having the list of all VTK Classes.
  SET(kits
    Common
    Filtering
    Imaging
    Graphics
    GenericFiltering
    IO)
  
  IF(VTK_USE_RENDERING)
    SET(kits ${kits}
      Rendering
      VolumeRendering
      Hybrid
      Widgets)
  ENDIF(VTK_USE_RENDERING)
  IF(VTK_USE_PARALLEL)
    SET(kits ${kits}
      Parallel)
  ENDIF(VTK_USE_PARALLEL)

  SET(VTK_CLASSES "\"___list_begins___\"")
  
  MACRO(APPEND_CLASSES varname)
    #SET(VTK_CLASSES ${VTK_CLASSES} ${${varname}})
    FOREACH (class ${${varname}})
      SET(VTK_CLASSES "${VTK_CLASSES},\n\"${class}\"")
    ENDFOREACH (class)
  ENDMACRO(APPEND_CLASSES)
    
  FOREACH(kit ${kits})
    STRING(TOUPPER ${kit} ukit)
    INCLUDE(${VTK_BINARY_DIR}/Utilities/vtk${kit}Kit.cmake)
    SET(var VTK_${ukit}_CLASSES)
    APPEND_CLASSES(${var})
  ENDFOREACH(kit)

  CONFIGURE_FILE(${VTK_SOURCE_DIR}/Utilities/vtkTclTest2Py/vtkClassList.py.in
    ${VTK_BINARY_DIR}/Utilities/vtkTclTest2Py/vtkClassList.py
    @ONLY)

  ADD_CUSTOM_TARGET(vtktcltest2py_pyc ALL echo "..."
    DEPENDS 
    ${VTK_BINARY_DIR}/Utilities/vtkTclTest2Py/vtk_compile_complete
    ${VTK_BINARY_DIR}/Utilities/vtkTclTest2Py/vtkClassList.py
    )
    
ENDIF(PYTHON_EXECUTABLE AND VTK_WRAP_PYTHON)



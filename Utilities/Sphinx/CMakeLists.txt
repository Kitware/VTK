# Build the sphinx documentation hosted at docs.vtk.org
# Make sure sphinx is installed as well as the packages listed in /Documentation/docs/requirements.txt

find_program(SPHINX_BUILD_EXECUTABLE
  NAMES sphinx-build
  DOC "Sphinx Documentation Builder (sphinx-doc.org)"
)

if(SPHINX_BUILD_EXECUTABLE)
  set(doc_version "${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}-${VTK_BUILD_VERSION}")
  # Set output directory
  set(pkg vtk-${doc_version})
  set(docdir "${CMAKE_BINARY_DIR}/share/doc/${pkg}")


  # Generate documentation
  add_custom_target(SphinxDoc
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${docdir}"
    COMMAND "${SPHINX_BUILD_EXECUTABLE}"
            ${CMAKE_CURRENT_SOURCE_DIR}/../../Documentation/docs
            "${docdir}"
    )
else()
  message(WARNING "Warning: sphinx-build not found:"
                  " Sphinx documentation will not be created")
endif(  )

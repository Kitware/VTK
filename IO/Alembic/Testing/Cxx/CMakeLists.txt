vtk_add_test_cxx(vtkIOAlembicCxxTests tests
  TestAlembicExporter.cxx,NO_DATA,NO_VALID
  )
vtk_test_cxx_executable(vtkIOAlembicCxxTests tests
  DISABLE_FLOATING_POINT_EXCEPTIONS
  RENDERING_FACTORY
  )
# TestAlembicExporter reads back the exported archive with Alembic's own
# reader API (Alembic::Abc::IArchive) to verify time samples were written,
# so the test executable needs direct access to Alembic, not just the
# VTK::IOAlembic module (which links it privately).
find_package(Alembic REQUIRED)
target_link_libraries(vtkIOAlembicCxxTests PRIVATE Alembic::Alembic)

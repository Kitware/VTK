include(vtkTestingRenderingDriver)
CREATE_TEST_SOURCELIST(Tests ${KIT}CxxTests.cxx ${MyTests}
                       EXTRA_INCLUDE ${vtkTestingRendering_SOURCE_DIR}/vtkTestingObjectFactory.h)


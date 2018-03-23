vtk_module(vtkIOMotionFX
# leaving this out of StandAlone for now since
# vtkpegtl doesn't support VS2013. Once we drop VS2013 we can make this
# module a StandAlone module.
#  GROUPS
#  StandAlone
  DEPENDS
    vtkCommonExecutionModel
  PRIVATE_DEPENDS
    vtkCommonDataModel
    vtkCommonMisc
    vtkIOGeometry
    vtksys
    vtkpegtl
  TEST_DEPENDS
    vtkTestingCore
    vtkInteractionStyle
    vtkRenderingOpenGL2
    vtkTestingRendering
)

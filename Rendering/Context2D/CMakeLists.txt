set(module_SRCS
  vtkAbstractContextBufferId.cxx
  vtkAbstractContextItem.cxx
  vtkBlockItem.cxx
  vtkBrush.cxx
  vtkContext2D.cxx
  vtkContext3D.cxx
  vtkContextActor.cxx
  vtkContextClip.cxx
  vtkContextDevice2D.cxx
  vtkContextDevice3D.cxx
  vtkContextItem.cxx
  vtkContextKeyEvent.cxx
  vtkContextMapper2D.cxx
  vtkContextMouseEvent.cxx
  vtkContextScene.cxx
  vtkContextTransform.cxx
  vtkImageItem.cxx
  vtkMarkerUtilities.cxx
  vtkPen.cxx
#  vtkPiecewisePointHandleItem.cxx
  vtkPolyDataItem.cxx
  vtkPropItem.cxx
  vtkTooltipItem.cxx
  )

vtk_module_library(vtkRenderingContext2D ${module_SRCS})

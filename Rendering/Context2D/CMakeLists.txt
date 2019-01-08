set(classes
  vtkAbstractContextBufferId
  vtkAbstractContextItem
  vtkBlockItem
  vtkBrush
  vtkContext2D
  vtkContext3D
  vtkContextActor
  vtkContextClip
  vtkContextDevice2D
  vtkContextDevice3D
  vtkContextItem
  vtkContextKeyEvent
  vtkContextMapper2D
  vtkContextMouseEvent
  vtkContextScene
  vtkContextTransform
  vtkImageItem
  vtkLabeledContourPolyDataItem
  vtkMarkerUtilities
  vtkPen
  vtkPolyDataItem
  vtkPropItem
  vtkTooltipItem)

vtk_module_add_module(VTK::RenderingContext2D
  CLASSES ${classes})

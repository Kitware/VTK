# Rework PolyData Bounds Computation

 vtkPolyData::ComputeBounds() used to ignore points that do not belong to any cell.
 That was not consistent with other vtkPointSet subclasses, and thus lead to some
 error case, as some points could be outside the given bounds. See this [ ParaView issue ]( https://gitlab.kitware.com/paraview/paraview/-/issues/20354)
 for instance, where a point locator was initialized with smaller bounds but adding points
 outside bounds was unexpected.

 Now vtkPolyData defers to vtkPointSet::ComputeBounds() so vtkPolyData::GetBounds() may
 not return the same bounds as before. This behavior is probably the one you want
 when using bounds.

 The previous behavior is still availble through vtkPolyData::ComputeCellsBounds()
 and vtkPolyData::GetCellsBounds(). This is mainly used for rendering purpose and can
 be used for retrocompatibility.

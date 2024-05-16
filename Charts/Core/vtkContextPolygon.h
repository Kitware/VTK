// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkContextPolygon_h
#define vtkContextPolygon_h

#include "vtkChartsCoreModule.h"
#include "vtkType.h"          // For vtkIdType
#include "vtkVector.h"        // For vtkVector2f
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkTransform2D;
class vtkContextPolygonPrivate;

class VTKCHARTSCORE_EXPORT VTK_MARSHALAUTO vtkContextPolygon
{
public:
  // Description:
  // Creates a new, empty polygon.
  vtkContextPolygon();

  // Description:
  // Creates a new copy of \p polygon.
  vtkContextPolygon(const vtkContextPolygon& polygon);

  // Description:
  // Destroys the polygon.
  ~vtkContextPolygon();

  // Description:
  // Adds a point to the polygon.
  void AddPoint(const vtkVector2f& point);

  // Description:
  // Adds a point to the polygon.
  void AddPoint(float x, float y);

  // Description:
  // Returns the point at index.
  vtkVector2f GetPoint(vtkIdType index) const;

  // Description:
  // Returns the number of points in the polygon.
  vtkIdType GetNumberOfPoints() const;

  // Description:
  // Clears all the points from the polygon.
  void Clear();

  // Description:
  // Returns \c true if the polygon contains \p point.
  bool Contains(const vtkVector2f& point) const;

  // Description:
  // Returns a new polygon with each point transformed by \p transform.
  vtkContextPolygon Transformed(vtkTransform2D* transform) const;

  // Description:
  // Copies the values from \p other to this polygon.
  vtkContextPolygon& operator=(const vtkContextPolygon& other);

private:
  vtkContextPolygonPrivate* const d;
};

VTK_ABI_NAMESPACE_END
#endif // vtkContextPolygon_h
// VTK-HeaderTest-Exclude: vtkContextPolygon.h

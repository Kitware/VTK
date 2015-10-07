/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContextPolygon.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkContextPolygon_h
#define vtkContextPolygon_h

#include "vtkChartsCoreModule.h"
#include "vtkVector.h" // For vtkVector2f
#include "vtkType.h" // For vtkIdType

class vtkTransform2D;
class vtkContextPolygonPrivate;

class VTKCHARTSCORE_EXPORT vtkContextPolygon
{
public:
  // Description:
  // Creates a new, empty polygon.
  vtkContextPolygon();

  // Description:
  // Creates a new copy of \p polygon.
  vtkContextPolygon(const vtkContextPolygon &polygon);

  // Description:
  // Destroys the polygon.
  ~vtkContextPolygon();

  // Description:
  // Adds a point to the polygon.
  void AddPoint(const vtkVector2f &point);

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
  bool Contains(const vtkVector2f &point) const;

  // Description:
  // Returns a new polygon with each point transformed by \p transform.
  vtkContextPolygon Transformed(vtkTransform2D *transform) const;

  // Description:
  // Copies the values from \p other to this polygon.
  vtkContextPolygon& operator=(const vtkContextPolygon &other);

private:
  vtkContextPolygonPrivate* const d;
};

#endif // vtkContextPolygon_h
// VTK-HeaderTest-Exclude: vtkContextPolygon.h

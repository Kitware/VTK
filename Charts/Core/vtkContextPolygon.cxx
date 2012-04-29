/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContextPolygon.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkContextPolygon.h"

#include <vector>

#include "vtkTransform2D.h"

//-----------------------------------------------------------------------------
class vtkContextPolygonPrivate
{
public:
  std::vector<vtkVector2f> points;
};

//-----------------------------------------------------------------------------
vtkContextPolygon::vtkContextPolygon()
  : d(new vtkContextPolygonPrivate)
{
}

//-----------------------------------------------------------------------------
vtkContextPolygon::vtkContextPolygon(const vtkContextPolygon &polygon)
  : d(new vtkContextPolygonPrivate)
{
  d->points = polygon.d->points;
}

//-----------------------------------------------------------------------------
vtkContextPolygon::~vtkContextPolygon()
{
  delete d;
}

//-----------------------------------------------------------------------------
void vtkContextPolygon::AddPoint(const vtkVector2f &point)
{
  d->points.push_back(point);
}

//-----------------------------------------------------------------------------
void vtkContextPolygon::AddPoint(float x, float y)
{
  this->AddPoint(vtkVector2f(x, y));
}

//-----------------------------------------------------------------------------
vtkVector2f vtkContextPolygon::GetPoint(vtkIdType index) const
{
  return d->points[index];
}

//-----------------------------------------------------------------------------
vtkIdType vtkContextPolygon::GetNumberOfPoints() const
{
  return d->points.size();
}

//-----------------------------------------------------------------------------
void vtkContextPolygon::Clear()
{
  d->points.clear();
}

//-----------------------------------------------------------------------------
vtkContextPolygon vtkContextPolygon::Transformed(vtkTransform2D *transform) const
{
  vtkContextPolygon transformed;
  transformed.d->points.resize(d->points.size());
  transform->TransformPoints(reinterpret_cast<float *>(&d->points[0]),
                             reinterpret_cast<float *>(&transformed.d->points[0]),
                             d->points.size());
  return transformed;
}

//-----------------------------------------------------------------------------
vtkContextPolygon& vtkContextPolygon::operator=(const vtkContextPolygon &other)
{
  if(this != &other)
    {
    d->points = other.d->points;
    }

  return *this;
}

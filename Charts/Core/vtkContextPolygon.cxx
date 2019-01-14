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

#include <algorithm>
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
  return static_cast<vtkIdType>(d->points.size());
}

//-----------------------------------------------------------------------------
void vtkContextPolygon::Clear()
{
  d->points.clear();
}

//-----------------------------------------------------------------------------
bool vtkContextPolygon::Contains(const vtkVector2f &point) const
{
  float x = point.GetX();
  float y = point.GetY();

  // http://en.wikipedia.org/wiki/Point_in_polygon RayCasting method
  // shooting the ray along the x axis
  bool inside = false;
  float xintersection;
  for(size_t i = 0; i < d->points.size(); i++)
  {
    const vtkVector2f &p1 = d->points[i];
    const vtkVector2f &p2 = d->points[(i+1) % d->points.size()];

    if (y > std::min(p1.GetY(), p2.GetY()) &&
        y <= std::max(p1.GetY(),p2.GetY()) &&
        p1.GetY() != p2.GetY())
    {
      if (x <= std::max(p1.GetX(), p2.GetX()) )
      {
        xintersection = (y - p1.GetY())*(p2.GetX() - p1.GetX())/(p2.GetY() - p1.GetY()) + p1.GetX();
        if ( p1.GetX() == p2.GetX() || x <= xintersection)
        {
          // each time we intersect we switch if we are in side or not
          inside = !inside;
        }
      }
    }
  }

  return inside;
}

//-----------------------------------------------------------------------------
vtkContextPolygon vtkContextPolygon::Transformed(vtkTransform2D *transform) const
{
  vtkContextPolygon transformed;
  transformed.d->points.resize(d->points.size());
  transform->TransformPoints(reinterpret_cast<float *>(&d->points[0]),
                             reinterpret_cast<float *>(&transformed.d->points[0]),
                             static_cast<int>(d->points.size()));
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

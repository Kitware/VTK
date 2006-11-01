/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBoundingBox.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkBoundingBox.h"

void vtkBoundingBox::AddPoint(double px, double py, double pz)
{
  double p[3];
  p[0] = px;
  p[1] = py;
  p[2] = pz;
  this->AddPoint(p);
}

void vtkBoundingBox::AddPoint(double p[3])
{
  int i;
  for (i = 0; i < 3; i++)
    {
    if (p[i] < this->MinPnt[i])
      {
      this->MinPnt[i] = p[i];
      }

    if (p[i] > this->MaxPnt[i])
      {
      this->MaxPnt[i] = p[i];
      }
    }
}

void vtkBoundingBox::AddBox(const vtkBoundingBox &bbox)
{
  int i;
  for (i = 0; i < 3; i++)
    {
    if (bbox.MinPnt[i] < this->MinPnt[i])
      {
      this->MinPnt[i] = bbox.MinPnt[i];
      }

    if (bbox.MaxPnt[i] > this->MaxPnt[i])
      {
      this->MaxPnt[i] = bbox.MaxPnt[i];
      }
    }
}


void vtkBoundingBox::AddBounds(double bounds[6])
{
  if (bounds[0] < this->MinPnt[0])
    {
    this->MinPnt[0] = bounds[0];
    }

  if (bounds[1] > this->MaxPnt[0])
    {
    this->MaxPnt[0] = bounds[1];
    }

  if (bounds[2] < this->MinPnt[1])
    {
    this->MinPnt[1] = bounds[2];
    }

  if (bounds[3] > this->MaxPnt[1])
    {
    this->MaxPnt[1] = bounds[3];
    }

  if (bounds[4] < this->MinPnt[2])
    {
    this->MinPnt[2] = bounds[4];
    }

  if (bounds[5] > this->MaxPnt[2])
    {
    this->MaxPnt[2] = bounds[5];
    }
}
    
void vtkBoundingBox::SetBounds(double xMin, double xMax,
                              double yMin, double yMax,
                              double zMin, double zMax)
{
  this->MinPnt[0] = xMin;
  this->MaxPnt[0] = xMax;
  this->MinPnt[1] = yMin;
  this->MaxPnt[1] = yMax;
  this->MinPnt[2] = zMin;
  this->MaxPnt[2] = zMax;

  int i;
  for (i = 0; i < 3; i++)
    {
    if (this->MaxPnt[i] < this->MinPnt[i])
      {
      this->MaxPnt[i] = this->MinPnt[i];
      }
    }
}

void vtkBoundingBox::SetMinPoint(double x, double y, double z)
{
  this->MinPnt[0] = x;
  if (x > this->MaxPnt[0])
    {
    this->MaxPnt[0] = x;
    }
  
  this->MinPnt[1] = y;
  if (y > this->MaxPnt[1])
    {
    this->MaxPnt[1] = y;
    }
  
  this->MinPnt[2] = z;
  if (z > this->MaxPnt[2])
    {
    this->MaxPnt[2] = z;
    }
  
}

void vtkBoundingBox::SetMaxPoint(double x, double y, double z)
{
  this->MaxPnt[0] = x;
  if (x < this->MinPnt[0])
    {
    this->MinPnt[0] = x;
    }
    
  this->MaxPnt[1] = y;
  if (y < this->MinPnt[1])
    {
    this->MinPnt[1] = y;
    }
    
  this->MaxPnt[2] = z;
  if (z < this->MinPnt[2])
    {
    this->MinPnt[2] = z;
    }  
}

int vtkBoundingBox::IntersectBox(const vtkBoundingBox &bbox)
{
  // if either box is not valid don't do the opperation
  if (!(this->IsValid() && bbox.IsValid()))
    {
    return 0;
    }

  int i, intersects;
  double pMin[3], pMax[3];
  for (i = 0; i < 3; i++)
    {
    intersects = 0;
    if ((bbox.MinPnt[i] >= this->MinPnt[i]) &&
        (bbox.MinPnt[i] <= this->MaxPnt[i]))
      {
      intersects = 1;
      pMin[i] = bbox.MinPnt[i];
      }
    else if ((this->MinPnt[i] >= bbox.MinPnt[i]) &&
             (this->MinPnt[i] <= bbox.MaxPnt[i]))
      {
      intersects = 1;
      pMin[i] = this->MinPnt[i];      
      }
    if ((bbox.MaxPnt[i] >= this->MinPnt[i]) &&
        (bbox.MaxPnt[i] <= this->MaxPnt[i]))
      {
      intersects = 1;
      pMax[i] = bbox.MaxPnt[i];
      }
    else if ((this->MaxPnt[i] >= bbox.MinPnt[i]) &&
             (this->MaxPnt[i] <= bbox.MaxPnt[i]))
      {
      intersects = 1;
      pMax[i] = this->MaxPnt[i];
      }
    if (!intersects)
      {
      return 0;  
      }
    }

  // OK they did intersect - set the box to be the result
  for (i = 0; i < 3; i++)
    {
    this->MinPnt[i] = pMin[i];
    this->MaxPnt[i] = pMax[i];
    }
  return 1;
}

int vtkBoundingBox::Intersects(const vtkBoundingBox &bbox) const
{
  // if either box is not valid they don't intersect
  if (!(this->IsValid() && bbox.IsValid()))
    {
    return 0;
    }
  int i;
  for (i = 0; i < 3; i++)
    {
    if ((bbox.MinPnt[i] >= this->MinPnt[i]) &&
        (bbox.MinPnt[i] <= this->MaxPnt[i]))
      {
      continue;
      }
    if ((this->MinPnt[i] >= bbox.MinPnt[i]) &&
             (this->MinPnt[i] <= bbox.MaxPnt[i]))
      {
      continue;
      }
    if ((bbox.MaxPnt[i] >= this->MinPnt[i]) &&
        (bbox.MaxPnt[i] <= this->MaxPnt[i]))
      {
      continue;
      }
    if ((this->MaxPnt[i] >= bbox.MinPnt[i]) &&
             (this->MaxPnt[i] <= bbox.MaxPnt[i]))
      {
      continue;
      }
    return 0;  
    }
  return 1;
}


double vtkBoundingBox::GetMaxLength() const
{
  double l[3];
  this->GetLengths(l);
  if (l[0] > l[1])
    {
    if (l[0] > l[2])
      {
      return l[0];
      }
    return l[2];
    }
  else if (l[1] > l[2])
    {
    return l[1];
    }
  return l[2];
}

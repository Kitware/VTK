/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVector.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkRect - templated base type for storage of 2D rectangles.
//
// .SECTION Description
// This class is a templated data type for storing and manipulating rectangles.
// The memory layout is a contiguous array of the specified type, such that a
// float[4] can be cast to a vtkRectf and manipulated. Also a float[12] could
// be cast and used as a vtkRectf[3].

#ifndef vtkRect_h
#define vtkRect_h

#include "vtkVector.h"

template<typename T>
class vtkRect : public vtkVector<T, 4>
{
public:
  vtkRect()
  {
  }

  vtkRect(const T& x, const T& y, const T& width, const T& height)
  {
    this->Data[0] = x;
    this->Data[1] = y;
    this->Data[2] = width;
    this->Data[3] = height;
  }

  explicit vtkRect(const T* init) : vtkVector<T, 4>(init) { }

  // Description:
  // Set the x, y components of the rectangle, and the width/height.
  void Set(const T& x, const T& y, const T& width, const T& height)
  {
    this->Data[0] = x;
    this->Data[1] = y;
    this->Data[2] = width;
    this->Data[3] = height;
  }

  // Description:
  // Set the x component of the rectangle bottom corner, i.e. element 0.
  void SetX(const T& x) { this->Data[0] = x; }

  // Description:
  // Get the x component of the rectangle bottom corner, i.e. element 0.
  const T& GetX() const { return this->Data[0]; }

  // Description:
  // Set the y component of the rectangle bottom corner, i.e. element 1.
  void SetY(const T& y) { this->Data[1] = y; }

  // Description:
  // Get the y component of the rectangle bottom corner, i.e. element 1.
  const T& GetY() const { return this->Data[1]; }

  // Description:
  // Set the width of the rectanle, i.e. element 2.
  void SetWidth(const T& width) { this->Data[2] = width; }

  // Description:
  // Get the width of the rectangle, i.e. element 2.
  const T& GetWidth() const { return this->Data[2]; }

  // Description:
  // Set the height of the rectangle, i.e. element 3.
  void SetHeight(const T& height) { this->Data[3] = height; }

  // Description:
  // Get the height of the rectangle, i.e. element 3.
  const T& GetHeight() const { return this->Data[3]; }

  // Description:
  // Legacy method for getting the x position.
  VTK_LEGACY(const T& X() const);

  // Description:
  // Legacy method for getting the y position.
  VTK_LEGACY(const T& Y() const);

  // Description:
  // Legacy method for getting the width.
  VTK_LEGACY(const T& Width() const);

  // Description:
  // Legacy method for getting the height.
  VTK_LEGACY(const T& Height() const);
};

class vtkRecti : public vtkRect<int>
{
public:
  vtkRecti() {}
  vtkRecti(int x, int y, int width, int height)
    : vtkRect<int>(x, y, width, height) {}
  explicit vtkRecti(const int *init) : vtkRect<int>(init) {}
};

class vtkRectf : public vtkRect<float>
{
public:
  vtkRectf() {}
  vtkRectf(float x, float y, float width, float height)
    : vtkRect<float>(x, y, width, height) {}
  explicit vtkRectf(const float *init) : vtkRect<float>(init) {}
};

class vtkRectd : public vtkRect<double>
{
public:
  vtkRectd() {}
  vtkRectd(double x, double y, double width, double height)
    : vtkRect<double>(x, y, width, height) {}
  explicit vtkRectd(const double *init) : vtkRect<double>(init) {}
};

#ifndef VTK_LEGACY_REMOVE
template<typename T>
const T& vtkRect<T>::X() const
{
  VTK_LEGACY_REPLACED_BODY(vtkRect::X, "VTK 6.0", vtkRect::GetX);
  return this->GetX();
}

template<typename T>
const T& vtkRect<T>::Y() const
{
  VTK_LEGACY_REPLACED_BODY(vtkRect::Y, "VTK 6.0", vtkRect::GetY);
  return this->GetY();
}

template<typename T>
const T& vtkRect<T>::Width() const
{
  VTK_LEGACY_REPLACED_BODY(vtkRect::Width, "VTK 6.0", vtkRect::GetWidth);
  return this->GetWidth();
}

template<typename T>
const T& vtkRect<T>::Height() const
{
  VTK_LEGACY_REPLACED_BODY(vtkRect::Height, "VTK 6.0", vtkRect::GetHeight);
  return this->GetHeight();
}

#endif // VTK_LEGACY_REMOVE


#endif // vtkRect_h
// VTK-HeaderTest-Exclude: vtkRect.h

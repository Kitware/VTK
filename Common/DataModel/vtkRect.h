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

/**
 * @class   vtkRect
 * @brief   templated base type for storage of 2D rectangles.
 *
 *
 * This class is a templated data type for storing and manipulating rectangles.
 * The memory layout is a contiguous array of the specified type, such that a
 * float[4] can be cast to a vtkRectf and manipulated. Also a float[12] could
 * be cast and used as a vtkRectf[3].
*/

#ifndef vtkRect_h
#define vtkRect_h

#include "vtkVector.h"

#include "vtkMath.h" // for Min, Max

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

  //@{
  /**
   * Set the x, y components of the rectangle, and the width/height.
   */
  void Set(const T& x, const T& y, const T& width, const T& height)
  {
    this->Data[0] = x;
    this->Data[1] = y;
    this->Data[2] = width;
    this->Data[3] = height;
  }
  //@}

  /**
   * Set the x component of the rectangle bottom corner, i.e. element 0.
   */
  void SetX(const T& x) { this->Data[0] = x; }

  /**
   * Get the x component of the rectangle bottom corner, i.e. element 0.
   */
  const T& GetX() const { return this->Data[0]; }

  /**
   * Set the y component of the rectangle bottom corner, i.e. element 1.
   */
  void SetY(const T& y) { this->Data[1] = y; }

  /**
   * Get the y component of the rectangle bottom corner, i.e. element 1.
   */
  const T& GetY() const { return this->Data[1]; }

  /**
   * Set the width of the rectanle, i.e. element 2.
   */
  void SetWidth(const T& width) { this->Data[2] = width; }

  /**
   * Get the width of the rectangle, i.e. element 2.
   */
  const T& GetWidth() const { return this->Data[2]; }

  /**
   * Set the height of the rectangle, i.e. element 3.
   */
  void SetHeight(const T& height) { this->Data[3] = height; }

  /**
   * Get the height of the rectangle, i.e. element 3.
   */
  const T& GetHeight() const { return this->Data[3]; }

  /**
   * Get the left boundary of the rectangle along the X direction.
   */
  const T& GetLeft() const { return this->Data[0]; }

  /**
   * Get the right boundary of the rectangle along the X direction.
   */
  T GetRight() const { return this->Data[0] + this->Data[2]; }

  /**
   * Get the top boundary of the rectangle along the Y direction.
   */
  T GetTop() const { return this->Data[1] + this->Data[3]; }

  /**
   * Get the bottom boundary of the rectangle along the Y direction.
   */
  const T& GetBottom() const { return this->Data[1]; }

  /**
   * Get the bottom left corner of the rect as a vtkVector.
   */
  vtkVector2<T> GetBottomLeft() const
  {
    return vtkVector2<T>(this->GetLeft(), this->GetBottom());
  }

  /**
   * Get the top left corner of the rect as a vtkVector.
   */
  vtkVector<T, 2> GetTopLeft() const
  {
    return vtkVector2<T>(this->GetLeft(), this->GetTop());
  }

  /**
   * Get the bottom right corner of the rect as a vtkVector.
   */
  vtkVector<T, 2> GetBottomRight() const
  {
    return vtkVector2<T>(this->GetRight(), this->GetBottom());
  }

  /**
   * Get the bottom left corner of the rect as a vtkVector.
   */
  vtkVector<T, 2> GetTopRight() const
  {
    return vtkVector2<T>(this->GetRight(), this->GetTop());
  }

  //@{
  /**
   * Expand this rect to contain the point passed in.
   */
  void AddPoint(const T point[2])
  {
    // This code is written like this to ensure that adding a point gives
    // exactly the same result as AddRect(vtkRect(x,y,0,0)
    if (point[0] < this->GetX())
    {
      T dx = this->GetX() - point[0];
      this->SetX(point[0]);
      this->SetWidth(dx + this->GetWidth());
    }
    else if (point[0] > this->GetX())
    {
      // this->GetX() is already correct
      T dx = point[0] - this->GetX();
      this->SetWidth(vtkMath::Max(dx, this->GetWidth()));
    }
  //@}

    if (point[1] < this->GetY())
    {
      T dy = this->GetY() - point[1];
      this->SetY(point[1]);
      this->SetHeight(dy + this->GetHeight());
    }
    else if (point[1] > this->GetY())
    {
      // this->GetY() is already correct
      T dy = point[1] - this->GetY();
      this->SetHeight(vtkMath::Max(dy, this->GetHeight()));
    }
  }

  //@{
  /**
   * Expand this rect to contain the point passed in.
   */
  void AddPoint(T x, T y)
  {
    T point[2] = {x, y};
    this->AddPoint(point);
  }
  //@}

  //@{
  /**
   * Expand this rect to contain the rect passed in.
   */
  void AddRect(const vtkRect<T> & rect)
  {
    if (rect.GetX() < this->GetX())
    {
      T dx = this->GetX() - rect.GetX();
      this->SetX(rect.GetX());
      this->SetWidth(vtkMath::Max(dx + this->GetWidth(), rect.GetWidth()));
    }
    else if (rect.GetX() > this->GetX())
    {
      T dx = rect.GetX() - this->GetX();
      // this->GetX() is already correct
      this->SetWidth(vtkMath::Max(dx + rect.GetWidth(), this->GetWidth()));
    }
    else
    {
      // this->GetX() is already correct
      this->SetWidth(vtkMath::Max(rect.GetWidth(), this->GetWidth()));
    }
  //@}

    if (rect.GetY() < this->GetY())
    {
      T dy = this->GetY() - rect.GetY();
      this->SetY(rect.GetY());
      this->SetHeight(vtkMath::Max(dy + this->GetHeight(), rect.GetHeight()));
    }
    else if (rect.GetY() > this->GetY())
    {
      T dy = rect.GetY() - this->GetY();
      // this->GetY() is already correct
      this->SetHeight(vtkMath::Max(dy + rect.GetHeight(), this->GetHeight()));
    }
    else
    {
      // this->GetY() is already correct
      this->SetHeight(vtkMath::Max(rect.GetHeight(), this->GetHeight()));
    }
  }

  /**
   * Returns true if the rect argument overlaps this rect.
   * If the upper bound of one rect is equal to the lower bound of
   * the other rect, then this will return false (in that case, the
   * rects would be considered to be adjacent but not overlapping).
   */
  bool IntersectsWith(const vtkRect<T> & rect)
  {
    bool intersects = true;

    if (rect.GetX() < this->GetX())
    {
      T dx = this->GetX() - rect.GetX();
      intersects &= (dx < rect.GetWidth());
    }
    else if (rect.GetX() > this->GetX())
    {
      T dx = rect.GetX() - this->GetX();
      intersects &= (dx < this->GetWidth());
    }

    if (rect.GetY() < this->GetY())
    {
      T dy = this->GetY() - rect.GetY();
      intersects &= (dy < rect.GetHeight());
    }
    else if (rect.GetY() > this->GetY())
    {
      T dy = rect.GetY() - this->GetY();
      intersects &= (dy < this->GetHeight());
    }

    return intersects;
  }
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

#endif // vtkRect_h
// VTK-HeaderTest-Exclude: vtkRect.h

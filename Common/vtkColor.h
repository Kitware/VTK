/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkColor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkColor - templated type for storage of colors.
//
// .SECTION Description
// This class is a templated data type for storing and manipulating fixed size
// colors. It derives from the vtkVector templated data structure.

#ifndef __vtkColor_h
#define __vtkColor_h

#include "vtkVector.h"

// .NAME vtkColor3 - templated base type for storage of 3 component colors.
//
template<typename T>
class vtkColor3 : public vtkVector<T, 3>
{
public:
  vtkColor3(const T& red = 0, const T& green = 0, const T& blue = 0)
  {
    this->Data[0] = red;
    this->Data[1] = green;
    this->Data[2] = blue;
  }
  explicit vtkColor3(const T* init) : vtkVector<T, 3>(init)
  {
  }

  // Description:
  // Set the red, green and blue components of the color.
  void Set(const T& red, const T& green, const T& blue)
  {
    this->Data[0] = red;
    this->Data[1] = green;
    this->Data[2] = blue;
  }

  // Description:
  // Set the red component of the color, i.e. element 0.
  void SetRed(const T& red) { this->Data[0] = red; }

  // Description:
  // Get the red component of the color, i.e. element 0.
  const T& GetRed() const { return this->Data[0]; }
  const T& Red() const { return this->Data[0]; }

  // Description:
  // Set the green component of the color, i.e. element 1.
  void SetGreen(const T& green) { this->Data[1] = green; }

  // Description:
  // Get the green component of the color, i.e. element 1.
  const T& GetGreen() const { return this->Data[1]; }
  const T& Green() const { return this->Data[1]; }

  // Description:
  // Set the blue component of the color, i.e. element 2.
  void SetBlue(const T& blue) { this->Data[2] = blue; }

  // Description:
  // Get the blue component of the color, i.e. element 2.
  const T& GetBlue() const { return this->Data[2]; }
  const T& Blue() const { return this->Data[2]; }
};

// .NAME vtkColor4 - templated base type for storage of 4 component colors.
//
template<typename T>
class vtkColor4 : public vtkVector<T, 4>
{
public:
  vtkColor4(const T& red = 0, const T& green = 0, const T& blue = 0,
            const T& alpha = 0)
  {
    this->Data[0] = red;
    this->Data[1] = green;
    this->Data[2] = blue;
    this->Data[3] = alpha;
  }
  explicit vtkColor4(const T* init) : vtkVector<T, 4>(init)
  {
  }

  // Description:
  // Set the red, green and blue components of the color.
  void Set(const T& red, const T& green, const T& blue)
  {
    this->Data[0] = red;
    this->Data[1] = green;
    this->Data[2] = blue;
  }

  // Description:
  // Set the red, green, blue and alpha components of the color.
  void Set(const T& red, const T& green, const T& blue, const T& alpha)
  {
    this->Data[0] = red;
    this->Data[1] = green;
    this->Data[2] = blue;
    this->Data[3] = alpha;
  }

  // Description:
  // Set the red component of the color, i.e. element 0.
  void SetRed(const T& red) { this->Data[0] = red; }

  // Description:
  // Get the red component of the color, i.e. element 0.
  const T& GetRed() const { return this->Data[0]; }
  const T& Red() const { return this->Data[0]; }

  // Description:
  // Set the green component of the color, i.e. element 1.
  void SetGreen(const T& green) { this->Data[1] = green; }

  // Description:
  // Get the green component of the color, i.e. element 1.
  const T& GetGreen() const { return this->Data[1]; }
  const T& Green() const { return this->Data[1]; }

  // Description:
  // Set the blue component of the color, i.e. element 2.
  void SetBlue(const T& blue) { this->Data[2] = blue; }

  // Description:
  // Get the blue component of the color, i.e. element 2.
  const T& GetBlue() const { return this->Data[2]; }
  const T& Blue() const { return this->Data[2]; }

  // Description:
  // Set the alpha component of the color, i.e. element 3.
  void SetAlpha(const T& alpha) { this->Data[3] = alpha; }

  // Description:
  // Get the alpha component of the color, i.e. element 3.
  const T& GetAlpha() const { return this->Data[3]; }
  const T& Alpha() const { return this->Data[3]; }
};

// Description:
// Some derived classes for the different vectors and colors commonly used.
class vtkColor3ub : public vtkColor3<unsigned char>
{
public:
  vtkColor3ub(unsigned char r = 0, unsigned char g = 0,
              unsigned char b = 0) : vtkColor3<unsigned char>(r, g, b) {}
  explicit vtkColor3ub(const unsigned char* init)
    : vtkColor3<unsigned char>(init) {}
};

class vtkColor3f : public vtkColor3<float>
{
public:
  vtkColor3f(float r = 0.0, float g = 0.0, float b = 0.0)
    : vtkColor3<float>(r, g, b) {}
  explicit vtkColor3f(const float* init) : vtkColor3<float>(init) {}
};

class vtkColor3d : public vtkColor3<double>
{
public:
  vtkColor3d(double r = 0.0, double g = 0.0, double b = 0.0)
    : vtkColor3<double>(r, g, b) {}
  explicit vtkColor3d(const double* init) : vtkColor3<double>(init) {}
};

class vtkColor4ub : public vtkColor4<unsigned char>
{
public:
  vtkColor4ub(unsigned char r = 0, unsigned char g = 0,
              unsigned char b = 0, unsigned char a = 255)
                : vtkColor4<unsigned char>(r, g, b, a) {}
  explicit vtkColor4ub(const unsigned char* init)
    : vtkColor4<unsigned char>(init) {}
  vtkColor4ub(const vtkColor3ub &c) :
    vtkColor4<unsigned char>(c[0], c[1], c[2], 255) {}
};

class vtkColor4f : public vtkColor4<float>
{
public:
  vtkColor4f(float r = 0.0, float g = 0.0, float b = 0.0, float a = 1.0)
    : vtkColor4<float>(r, g, b, a) {}
  explicit vtkColor4f(const float* init) : vtkColor4<float>(init) {}
};

class vtkColor4d : public vtkColor4<double>
{
public:
  vtkColor4d(double r = 0.0, double g = 0.0, double b = 0.0, float a = 1.0)
    : vtkColor4<double>(r, g, b, a) {}
  explicit vtkColor4d(const double* init) : vtkColor4<double>(init) {}
};

#endif // __vtkColor_h

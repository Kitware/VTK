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

// .NAME vtkVector - templated base type for storage of vectors.
//
// .SECTION Description
// This class is a templated data type for storing and manipulating fixed size
// vectors, which can be used to represent two and three dimensional points. The
// memory layout is a contiguous array of the specified type, such that a
// float[2] can be cast to a vtkVector2f and manipulated. Also a float[6] could
// be cast and used as a vtkVector2f[3].

#ifndef __vtkVector_h
#define __vtkVector_h

template<typename T, int Size>
class vtkVector
{
public:
  vtkVector()
  {
    for (int i = 0; i < Size; ++i)
      {
      Data[i] = 0;
      }
  }

  // Description:
  // Get the size of the vtkVector.
  int GetSize() { return Size; }

  // Description:
  // Get a pointer to the underlying data of the vtkVector.
  T* GetData() { return this->Data; }

  // Description:
  // Get a reference to the underlying data element of the vtkVector. Can be
  // used in much the same way as vector[i] is used.
  T& operator[](int i) { return this->Data[i]; }

  // Description:
  // Get the value of the vector at the index speciifed. Does bounds checking,
  // used in much the same way as vector.at(i) is used.
  T operator()(int i) { return this->Data[i]; }

protected:
  // Description:
  // The only thing stored in memory!
  T Data[Size];
};

// .NAME vtkVector2 - templated base type for storage of 2D vectors.
//
template<typename T>
class vtkVector2 : public vtkVector<T, 2>
{
public:
  vtkVector2(const T& x = 0.0, const T& y = 0.0)
  {
    this->Data[0] = x;
    this->Data[1] = y;
  }

  // Description:
  // Set the x and y components of the vector.
  void Set(const T& x, const T& y)
  {
    this->Data[0] = x;
    this->Data[1] = y;
  }

  // Description:
  // Set the x component of the vector, i.e. element 0.
  void SetX(const T& x) { this->Data[0] = x; }

  // Description:
  // Get the x component of the vector, i.e. element 0.
  const T& GetX() const { return this->Data[0]; }
  const T& X() const { return this->Data[0]; }

  // Description:
  // Set the y component of the vector, i.e. element 1.
  void SetY(const T& y) { this->Data[1] = y; }

  // Description:
  // Get the y component of the vector, i.e. element 1.
  const T& GetY() const { return this->Data[1]; }
  const T& Y() const { return this->Data[1]; }
};

// .NAME vtkVector3 - templated base type for storage of 3D vectors.
//
template<typename T>
class vtkVector3 : public vtkVector<T, 3>
{
public:
  vtkVector3(const T& x = 0.0, const T y = 0.0, const T z = 0.0)
  {
    this->Data[0] = x;
    this->Data[1] = y;
    this->Data[2] = z;
  }

  // Description:
  // Set the x, y and z components of the vector.
  void Set(const T& x, const T& y, const T& z)
  {
    this->Data[0] = x;
    this->Data[1] = y;
    this->Data[2] = z;
  }

  // Description:
  // Set the x component of the vector, i.e. element 0.
  void SetX(const T& x) { this->Data[0] = x; }

  // Description:
  // Get the x component of the vector, i.e. element 0.
  const T& GetX() const { return this->Data[0]; }
  const T& X() const { return this->Data[0]; }

  // Description:
  // Set the y component of the vector, i.e. element 1.
  void SetY(const T& y) { this->Data[1] = y; }

  // Description:
  // Get the y component of the vector, i.e. element 1.
  const T& GetY() const { return this->Data[1]; }
  const T& Y() const { return this->Data[1]; }

  // Description:
  // Set the z component of the vector, i.e. element 2.
  void SetZ(const T& z) { this->Data[2] = z; }

  // Description:
  // Get the z component of the vector, i.e. element 2.
  const T& GetZ() const { return this->Data[2]; }
  const T& Z() const { return this->Data[2]; }

};

// .NAME vtkColor3 - templated base type for storage of 3 component colors.
//
template<typename T>
class vtkColor3 : public vtkVector<T, 3>
{
public:
  vtkColor3(const T& red = 0.0, const T& green = 0.0, const T& blue = 0.0)
  {
    this->Data[0] = red;
    this->Data[1] = green;
    this->Data[2] = blue;
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
  vtkColor4(const T& red = 0.0, const T& green = 0.0, const T& blue = 0.0,
            const T& alpha = 0.0)
  {
    this->Data[0] = red;
    this->Data[1] = green;
    this->Data[2] = blue;
    this->Data[3] = alpha;
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
class vtkVector2i : public vtkVector2<int>
{
public:
  vtkVector2i(int x = 0, int y = 0) : vtkVector2<int>(x, y) {}
};

class vtkVector2f : public vtkVector2<float>
{
public:
  vtkVector2f(float x = 0.0, float y = 0.0) : vtkVector2<float>(x, y) {}
};

class vtkVector2d : public vtkVector2<double>
{
public:
  vtkVector2d(double x = 0.0, double y = 0.0) : vtkVector2<double>(x, y) {}
};

class vtkVector3i : public vtkVector3<int>
{
public:
  vtkVector3i(int x = 0, int y = 0, int z = 0) : vtkVector3<int>(x, y, z) {}
};

class vtkVector3f : public vtkVector3<float>
{
public:
  vtkVector3f(float x = 0.0, float y = 0.0, float z = 0.0)
    : vtkVector3<float>(x, y, z) {}
};

class vtkVector3d : public vtkVector3<double>
{
public:
  vtkVector3d(double x = 0.0, double y = 0.0, double z = 0.0)
    : vtkVector3<double>(x, y, z) {}
};

class vtkColor3ub : public vtkColor3<unsigned char>
{
public:
  vtkColor3ub(unsigned char r = 0, unsigned char g = 0,
              unsigned char b = 0) : vtkColor3<unsigned char>(r, g, b) {}
};

class vtkColor3f : public vtkColor3<float>
{
public:
  vtkColor3f(float r = 0.0, float g = 0.0, float b = 0.0)
    : vtkColor3<float>(r, g, b) {}
};

class vtkColor3d : public vtkColor3<double>
{
public:
  vtkColor3d(double r = 0.0, double g = 0.0, double b = 0.0)
    : vtkColor3<double>(r, g, b) {}
};

class vtkColor4ub : public vtkColor4<unsigned char>
{
public:
  vtkColor4ub(unsigned char r = 0, unsigned char g = 0,
              unsigned char b = 0, unsigned char a = 255)
                : vtkColor4<unsigned char>(r, g, b, a) {}
};

class vtkColor4f : public vtkColor4<float>
{
public:
  vtkColor4f(float r = 0.0, float g = 0.0, float b = 0.0, float a = 1.0)
    : vtkColor4<float>(r, g, b, a) {}
};

class vtkColor4d : public vtkColor4<double>
{
public:
  vtkColor4d(double r = 0.0, double g = 0.0, double b = 0.0, float a = 1.0)
    : vtkColor4<double>(r, g, b, a) {}
};

#endif // __vtkVector_h

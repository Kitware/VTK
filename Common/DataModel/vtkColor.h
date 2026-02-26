// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkColor
 * @brief   templated type for storage of colors.
 *
 *
 * This class is a templated data type for storing and manipulating fixed size
 * colors. It derives from the vtkVector templated data structure.
 */

#ifndef vtkColor_h
#define vtkColor_h

#include "vtkObject.h" // for legacy macros
#include "vtkTuple.h"

#include <algorithm> // for std::clamp
#include <cmath>     // for std::round

// Forward declarations for conversion methods.
VTK_ABI_NAMESPACE_BEGIN
class vtkColor3ub;
class vtkColor3f;
class vtkColor3d;
class vtkColor4ub;
class vtkColor4f;
class vtkColor4d;

// .NAME vtkColor3 - templated base type for storage of 3 component colors.
//
template <typename T>
class vtkColor3 : public vtkTuple<T, 3>
{
public:
  vtkColor3() = default;

  explicit vtkColor3(const T& scalar)
    : vtkTuple<T, 3>(scalar)
  {
  }

  explicit vtkColor3(const T* init)
    : vtkTuple<T, 3>(init)
  {
  }

  vtkColor3(const T& red, const T& green, const T& blue)
  {
    this->Data[0] = red;
    this->Data[1] = green;
    this->Data[2] = blue;
  }

  ///@{
  /**
   * Set the red, green and blue components of the color.
   */
  void Set(const T& red, const T& green, const T& blue)
  {
    this->Data[0] = red;
    this->Data[1] = green;
    this->Data[2] = blue;
  }
  ///@}

  /**
   * Set the red component of the color, i.e. element 0.
   */
  void SetRed(const T& red) { this->Data[0] = red; }

  /**
   * Get the red component of the color, i.e. element 0.
   */
  const T& GetRed() const { return this->Data[0]; }

  /**
   * Set the green component of the color, i.e. element 1.
   */
  void SetGreen(const T& green) { this->Data[1] = green; }

  /**
   * Get the green component of the color, i.e. element 1.
   */
  const T& GetGreen() const { return this->Data[1]; }

  /**
   * Set the blue component of the color, i.e. element 2.
   */
  void SetBlue(const T& blue) { this->Data[2] = blue; }

  /**
   * Get the blue component of the color, i.e. element 2.
   */
  const T& GetBlue() const { return this->Data[2]; }
};

// .NAME vtkColor4 - templated base type for storage of 4 component colors.
//
template <typename T>
class vtkColor4 : public vtkTuple<T, 4>
{
public:
  vtkColor4() = default;

  explicit vtkColor4(const T& scalar)
    : vtkTuple<T, 4>(scalar)
  {
  }

  explicit vtkColor4(const T* init)
    : vtkTuple<T, 4>(init)
  {
  }

  vtkColor4(const T& red, const T& green, const T& blue, const T& alpha)
  {
    this->Data[0] = red;
    this->Data[1] = green;
    this->Data[2] = blue;
    this->Data[3] = alpha;
  }

  ///@{
  /**
   * Set the red, green and blue components of the color.
   */
  void Set(const T& red, const T& green, const T& blue)
  {
    this->Data[0] = red;
    this->Data[1] = green;
    this->Data[2] = blue;
  }
  ///@}

  ///@{
  /**
   * Set the red, green, blue and alpha components of the color.
   */
  void Set(const T& red, const T& green, const T& blue, const T& alpha)
  {
    this->Data[0] = red;
    this->Data[1] = green;
    this->Data[2] = blue;
    this->Data[3] = alpha;
  }
  ///@}

  /**
   * Set the red component of the color, i.e. element 0.
   */
  void SetRed(const T& red) { this->Data[0] = red; }

  /**
   * Get the red component of the color, i.e. element 0.
   */
  const T& GetRed() const { return this->Data[0]; }

  /**
   * Set the green component of the color, i.e. element 1.
   */
  void SetGreen(const T& green) { this->Data[1] = green; }

  /**
   * Get the green component of the color, i.e. element 1.
   */
  const T& GetGreen() const { return this->Data[1]; }

  /**
   * Set the blue component of the color, i.e. element 2.
   */
  void SetBlue(const T& blue) { this->Data[2] = blue; }

  /**
   * Get the blue component of the color, i.e. element 2.
   */
  const T& GetBlue() const { return this->Data[2]; }

  /**
   * Set the alpha component of the color, i.e. element 3.
   */
  void SetAlpha(const T& alpha) { this->Data[3] = alpha; }

  /**
   * Get the alpha component of the color, i.e. element 3.
   */
  const T& GetAlpha() const { return this->Data[3]; }
};

/**
 * Some derived classes for the different colors commonly used.
 */
class vtkColor3ub : public vtkColor3<unsigned char>
{
public:
  vtkColor3ub() = default;
  explicit vtkColor3ub(unsigned char scalar)
    : vtkColor3<unsigned char>(scalar)
  {
  }
  explicit vtkColor3ub(const unsigned char* init)
    : vtkColor3<unsigned char>(init)
  {
  }

  ///@{
  /**
   * Construct a color from a hexadecimal representation such as 0x0000FF (blue).
   */
  explicit vtkColor3ub(int hexSigned)
  {
    unsigned int hex = static_cast<unsigned int>(hexSigned);
    this->Data[2] = hex & 0xff;
    hex >>= 8;
    this->Data[1] = hex & 0xff;
    hex >>= 8;
    this->Data[0] = hex & 0xff;
  }
  ///@}

  vtkColor3ub(unsigned char r, unsigned char g, unsigned char b)
    : vtkColor3<unsigned char>(r, g, b)
  {
  }

  /**
   * Convert to vtkColor3f by scaling from [0,255] to [0.0,1.0].
   */
  inline vtkColor3f ToFloat() const;

  /**
   * Convert to vtkColor3d by scaling from [0,255] to [0.0,1.0].
   */
  inline vtkColor3d ToDouble() const;
};

class vtkColor3f : public vtkColor3<float>
{
public:
  vtkColor3f() = default;
  explicit vtkColor3f(float scalar)
    : vtkColor3<float>(scalar)
  {
  }
  explicit vtkColor3f(const float* init)
    : vtkColor3<float>(init)
  {
  }
  vtkColor3f(float r, float g, float b)
    : vtkColor3<float>(r, g, b)
  {
  }

  /**
   * Convert to vtkColor3ub by scaling from [0.0,1.0] to [0,255].
   */
  inline vtkColor3ub ToUnsignedChar() const;

  /**
   * Convert to vtkColor3d with a simple static_cast.
   */
  inline vtkColor3d ToDouble() const;
};

class vtkColor3d : public vtkColor3<double>
{
public:
  vtkColor3d() = default;
  explicit vtkColor3d(double scalar)
    : vtkColor3<double>(scalar)
  {
  }
  explicit vtkColor3d(const double* init)
    : vtkColor3<double>(init)
  {
  }
  vtkColor3d(double r, double g, double b)
    : vtkColor3<double>(r, g, b)
  {
  }

  /**
   * Convert to vtkColor3ub by scaling from [0.0,1.0] to [0,255].
   */
  inline vtkColor3ub ToUnsignedChar() const;

  /**
   * Convert to vtkColor3f with a simple static_cast.
   */
  inline vtkColor3f ToFloat() const;
};

class vtkColor4ub : public vtkColor4<unsigned char>
{
public:
  vtkColor4ub() = default;
  explicit vtkColor4ub(unsigned char scalar)
    : vtkColor4<unsigned char>(scalar)
  {
  }
  explicit vtkColor4ub(const unsigned char* init)
    : vtkColor4<unsigned char>(init)
  {
  }

  ///@{
  /**
   * Construct a color from a hexadecimal representation such as 0x0000FFAA
   * (opaque blue).
   */
  explicit vtkColor4ub(int hexSigned)
  {
    unsigned int hex = static_cast<unsigned int>(hexSigned);
    this->Data[3] = hex & 0xff;
    hex >>= 8;
    this->Data[2] = hex & 0xff;
    hex >>= 8;
    this->Data[1] = hex & 0xff;
    hex >>= 8;
    this->Data[0] = hex & 0xff;
  }
  ///@}

  vtkColor4ub(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255)
    : vtkColor4<unsigned char>(r, g, b, a)
  {
  }
  vtkColor4ub(const vtkColor3ub& c)
    : vtkColor4<unsigned char>(c[0], c[1], c[2], 255)
  {
  }

  /**
   * Convert to vtkColor4f by scaling from [0,255] to [0.0,1.0].
   */
  inline vtkColor4f ToFloat() const;

  /**
   * Convert to vtkColor4d by scaling from [0,255] to [0.0,1.0].
   */
  inline vtkColor4d ToDouble() const;
};

class vtkColor4f : public vtkColor4<float>
{
public:
  vtkColor4f() = default;
  explicit vtkColor4f(float scalar)
    : vtkColor4<float>(scalar)
  {
  }
  explicit vtkColor4f(const float* init)
    : vtkColor4<float>(init)
  {
  }
  vtkColor4f(float r, float g, float b, float a = 1.0)
    : vtkColor4<float>(r, g, b, a)
  {
  }

  /**
   * Convert to vtkColor4ub by scaling from [0.0,1.0] to [0,255].
   */
  inline vtkColor4ub ToUnsignedChar() const;

  /**
   * Convert to vtkColor4d with a simple static_cast.
   */
  inline vtkColor4d ToDouble() const;
};

class vtkColor4d : public vtkColor4<double>
{
public:
  vtkColor4d() = default;
  explicit vtkColor4d(double scalar)
    : vtkColor4<double>(scalar)
  {
  }
  explicit vtkColor4d(const double* init)
    : vtkColor4<double>(init)
  {
  }
  vtkColor4d(double r, double g, double b, double a = 1.0)
    : vtkColor4<double>(r, g, b, a)
  {
  }

  /**
   * Convert to vtkColor4ub by scaling from [0.0,1.0] to [0,255].
   */
  inline vtkColor4ub ToUnsignedChar() const;

  /**
   * Convert to vtkColor4f with a simple static_cast.
   */
  inline vtkColor4f ToFloat() const;
};

// ---------- vtkColor3ub conversions ----------

inline vtkColor3f vtkColor3ub::ToFloat() const
{
  return vtkColor3f(this->Data[0] / 255.0f, this->Data[1] / 255.0f, this->Data[2] / 255.0f);
}

inline vtkColor3d vtkColor3ub::ToDouble() const
{
  return vtkColor3d(this->Data[0] / 255.0, this->Data[1] / 255.0, this->Data[2] / 255.0);
}

// ---------- vtkColor3f conversions ----------

inline vtkColor3ub vtkColor3f::ToUnsignedChar() const
{
  return vtkColor3ub(
    static_cast<unsigned char>(std::round(std::clamp(this->Data[0] * 255.0f, 0.0f, 255.0f))),
    static_cast<unsigned char>(std::round(std::clamp(this->Data[1] * 255.0f, 0.0f, 255.0f))),
    static_cast<unsigned char>(std::round(std::clamp(this->Data[2] * 255.0f, 0.0f, 255.0f))));
}

inline vtkColor3d vtkColor3f::ToDouble() const
{
  return vtkColor3d(static_cast<double>(this->Data[0]), static_cast<double>(this->Data[1]),
    static_cast<double>(this->Data[2]));
}

// ---------- vtkColor3d conversions ----------

inline vtkColor3ub vtkColor3d::ToUnsignedChar() const
{
  return vtkColor3ub(
    static_cast<unsigned char>(std::round(std::clamp(this->Data[0] * 255.0, 0.0, 255.0))),
    static_cast<unsigned char>(std::round(std::clamp(this->Data[1] * 255.0, 0.0, 255.0))),
    static_cast<unsigned char>(std::round(std::clamp(this->Data[2] * 255.0, 0.0, 255.0))));
}

inline vtkColor3f vtkColor3d::ToFloat() const
{
  return vtkColor3f(static_cast<float>(this->Data[0]), static_cast<float>(this->Data[1]),
    static_cast<float>(this->Data[2]));
}

// ---------- vtkColor4ub conversions ----------

inline vtkColor4f vtkColor4ub::ToFloat() const
{
  return vtkColor4f(
    this->Data[0] / 255.0f, this->Data[1] / 255.0f, this->Data[2] / 255.0f, this->Data[3] / 255.0f);
}

inline vtkColor4d vtkColor4ub::ToDouble() const
{
  return vtkColor4d(
    this->Data[0] / 255.0, this->Data[1] / 255.0, this->Data[2] / 255.0, this->Data[3] / 255.0);
}

// ---------- vtkColor4f conversions ----------

inline vtkColor4ub vtkColor4f::ToUnsignedChar() const
{
  return vtkColor4ub(
    static_cast<unsigned char>(std::round(std::clamp(this->Data[0] * 255.0f, 0.0f, 255.0f))),
    static_cast<unsigned char>(std::round(std::clamp(this->Data[1] * 255.0f, 0.0f, 255.0f))),
    static_cast<unsigned char>(std::round(std::clamp(this->Data[2] * 255.0f, 0.0f, 255.0f))),
    static_cast<unsigned char>(std::round(std::clamp(this->Data[3] * 255.0f, 0.0f, 255.0f))));
}

inline vtkColor4d vtkColor4f::ToDouble() const
{
  return vtkColor4d(static_cast<double>(this->Data[0]), static_cast<double>(this->Data[1]),
    static_cast<double>(this->Data[2]), static_cast<double>(this->Data[3]));
}

// ---------- vtkColor4d conversions ----------

inline vtkColor4ub vtkColor4d::ToUnsignedChar() const
{
  return vtkColor4ub(
    static_cast<unsigned char>(std::round(std::clamp(this->Data[0] * 255.0, 0.0, 255.0))),
    static_cast<unsigned char>(std::round(std::clamp(this->Data[1] * 255.0, 0.0, 255.0))),
    static_cast<unsigned char>(std::round(std::clamp(this->Data[2] * 255.0, 0.0, 255.0))),
    static_cast<unsigned char>(std::round(std::clamp(this->Data[3] * 255.0, 0.0, 255.0))));
}

inline vtkColor4f vtkColor4d::ToFloat() const
{
  return vtkColor4f(static_cast<float>(this->Data[0]), static_cast<float>(this->Data[1]),
    static_cast<float>(this->Data[2]), static_cast<float>(this->Data[3]));
}

VTK_ABI_NAMESPACE_END
#endif // vtkColor_h
// VTK-HeaderTest-Exclude: vtkColor.h

// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDGArrayOutputAccessor
 * @brief   Store values interpolated from DG cells into a vtkDoubleArray instance.
 */
#ifndef vtkDGArrayOutputAccessor_h
#define vtkDGArrayOutputAccessor_h

#include "vtkDoubleArray.h"           // For API.
#include "vtkFiltersCellGridModule.h" // For export macro.
#include "vtkVector.h"                // For API.

#include <cassert>

VTK_ABI_NAMESPACE_BEGIN

class VTKFILTERSCELLGRID_EXPORT vtkDGArrayOutputAccessor
{
public:
  vtkDGArrayOutputAccessor(vtkDoubleArray* result);
  vtkDGArrayOutputAccessor(const vtkDGArrayOutputAccessor& other);
  ~vtkDGArrayOutputAccessor();

  vtkDGArrayOutputAccessor& operator=(const vtkDGArrayOutputAccessor& other);

  /// Expose a tuple in a vtkDoubleArray as an object with a size() method
  /// to satisfy requirements of the output iterator API.
  struct Tuple
  {
    Tuple() = default;
    Tuple(double* data, int size)
      : Data(data)
      , Size(size)
    {
    }
    Tuple(const Tuple&) = default;
    Tuple& operator=(const Tuple&) = default;

    double& operator[](int ii)
    {
      assert(ii < this->Size);
      return this->Data[ii];
    }
    const double* data() const { return this->Data; }
    double* data() { return this->Data; }
    int size() const { return this->Size; }

    /// If a tuple is "null", make it "falsy"; otherwise it is "truthy."
    operator bool() const { return !!this->Data && this->Size > 0; }

    double* Data{ nullptr };
    int Size{ 0 };
  };

  Tuple operator[](vtkTypeUInt64 tupleId);
  vtkTypeUInt64 GetKey() const { return this->Key; }
  Tuple GetTuple();
  void Restart();
  bool IsAtEnd() const;

  std::size_t size() const;

  vtkTypeUInt64 operator++();
  vtkTypeUInt64 operator++(int);

  vtkDGArrayOutputAccessor& operator+=(vtkTypeUInt64 count);

protected:
  vtkTypeUInt64 Key{ 0 };
  vtkDoubleArray* Result{ nullptr };
};

VTK_ABI_NAMESPACE_END

#endif // vtkDGArrayOutputAccessor_h

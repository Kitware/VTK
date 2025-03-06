// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDGArraysInputAccessor
 * @brief   Evaluate DG cells whose indices and parameters are provided by vtkDataArray instances.
 */
#ifndef vtkDGArraysInputAccessor_h
#define vtkDGArraysInputAccessor_h

#include "vtkFiltersCellGridModule.h" // For export macro.
#include "vtkVector.h"                // For API.

VTK_ABI_NAMESPACE_BEGIN

class vtkDataArray;

class VTKFILTERSCELLGRID_EXPORT vtkDGArraysInputAccessor
{
public:
  vtkDGArraysInputAccessor(vtkDataArray* cellIds, vtkDataArray* rst);
  vtkDGArraysInputAccessor(const vtkDGArraysInputAccessor& other);
  ~vtkDGArraysInputAccessor();

  vtkDGArraysInputAccessor& operator=(const vtkDGArraysInputAccessor& other);

  vtkIdType GetCellId(vtkTypeUInt64 iteration);
  vtkVector3d GetParameter(vtkTypeUInt64 iteration);
  vtkTypeUInt64 GetKey() const { return this->Key; }
  void Restart();
  bool IsAtEnd() const;

  std::size_t size() const;

  vtkTypeUInt64 operator++();
  vtkTypeUInt64 operator++(int);

  vtkDGArraysInputAccessor& operator+=(vtkTypeUInt64 count);

protected:
  vtkTypeUInt64 Key{ 0 };
  vtkDataArray* CellIds{ nullptr };
  vtkDataArray* RST{ nullptr };
};

VTK_ABI_NAMESPACE_END

#endif // vtkDGArraysInputAccessor_h

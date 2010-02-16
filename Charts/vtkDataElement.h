/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataElement.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkDataElement - Class to hold the data (table, array or scalar)
// and provide uniform way of accessing the data.
//
// .SECTION Description
//

#ifndef __vtkDataElement_h
#define __vtkDataElement_h

#include "vtkVariant.h"

// Forware declarations.
class vtkAbstractArray;
class vtkTable;

class VTK_CHARTS_EXPORT vtkDataElement
{
public:
  vtkDataElement();
  vtkDataElement(vtkVariant v);
  vtkDataElement(vtkTable* table);
  vtkDataElement(vtkTable* table, vtkIdType row);
  vtkDataElement(vtkAbstractArray* arr);
  vtkDataElement(vtkAbstractArray* arr, vtkIdType index, int type);
  vtkDataElement(const vtkDataElement &other);
  ~vtkDataElement();
  
  vtkDataElement &operator=(const vtkDataElement &other);
  
  enum {
    INVALID,
    TABLE,
    TABLE_ROW,
    ABSTRACT_ARRAY,
    ABSTRACT_ARRAY_TUPLE,
    ABSTRACT_ARRAY_COMPONENT,
    SCALAR
    };

  // Description:
  // Define access order in the case of a table or array.
  // This is a boolean value.
  // For a table, if Dimension is false, the access is per-row. If Dimension is
  // true, the access is per-column.
  // For an abstract array, the access is interleaved if Dimension is false.
  // If Dimension is true, the access is contiguous (ie. per tuple).
  // 
  void SetDimension(int dim){this->Dimension = dim;}

  // Description:
  // It depends on Dimension.
  // On a table, if Dimension is false, this is the number of rows. If it is true,
  // it is the number of columns.
  // On an array, if Dimension is false, this is the number of tuples. If it is
  // true, it is the number of components.
  vtkIdType GetNumberOfChildren();
  
  // Description:
  // According to GetNumberOfChildren(), on a table, if Dimension is false,
  // get row `i', if Dimension is true, get column `i'.
  // On an array, if Dimension is false, get tuple `i', if Dimension is true,
  // get component `i'.
  // It depends on the value of dimensions.
  vtkDataElement GetChild(vtkIdType i);

  // Description:
  // Number of items. It depends on the value of Dimension.
  // If Dimension is false on a table, it is the number of columns. If it is
  // true, it is the number of rows.
  // If Dimension is false on an array, it is the number of tuples. If it is
  // true, it is the number of components.
  vtkIdType GetSize();
  
  // Description:
  // Get the value of item `i'.
  // It depends on the value of Dimensions. See the definition of Dimension.
  vtkVariant GetValue(vtkIdType i = 0);
  vtkVariant GetValue(vtkstd::string str);
  bool IsValid();

protected:
  int Type;
  int Dimension;
  bool Valid;

  vtkVariant Scalar;
  vtkTable* Table;
  vtkAbstractArray* AbstractArray;
  vtkIdType Index;
};

#endif // __vtkDataElement_h

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

  enum {
    INVALID,
    TABLE,
    TABLE_ROW,
    ABSTRACT_ARRAY,
    ABSTRACT_ARRAY_TUPLE,
    ABSTRACT_ARRAY_COMPONENT,
    SCALAR
    };

  void SetDimension(int dim){this->Dimension = dim;}

  vtkIdType GetNumberOfChildren();
  vtkDataElement GetChild(vtkIdType i);

  vtkVariant GetValue(vtkIdType i = 0);
  vtkVariant GetValue(std::string str);
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

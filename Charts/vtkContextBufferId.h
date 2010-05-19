/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContextBufferId.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkContextBufferId - 2D array of ids, used for picking.
//
// .SECTION Description
// An 2D array where each element is the id of an entity drawn at the given
// pixel.

#ifndef __vtkContextBufferId_h
#define __vtkContextBufferId_h

#include "vtkObject.h"

class vtkIntArray;

class VTK_CHARTS_EXPORT vtkContextBufferId : public vtkObject
{
public:
  vtkTypeMacro(vtkContextBufferId, vtkObject);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Creates a 2D Painter object.
  static vtkContextBufferId *New();
  
  // Description:
  // Number of columns. Initial value is 0.
  vtkGetMacro(Width,int);
  
  // Description:
  // Set the number of columns. Initial value is 0.
  vtkSetMacro(Width,int);
  
  // Description:
  // Number of rows. Initial value is 0.
  vtkGetMacro(Height,int);
  
  // Description:
  // Set the number of rows. Initial value is 0.
  vtkSetMacro(Height,int);
  
  // Description:
  // Allocate the memory for at least Width*Height elements.
  // \pre positive_width: GetWidth()>0
  // \pre positive_height: GetHeight()>0
  void Allocate();
  
  // Description:
  // Tell if the buffer has been allocated.
  bool IsAllocated() const;
  
  // Description:
  // Set the value at index `i'.
  // \pre is_allocated: this->IsAllocated()
  // \pre valid_i: i>=0 i<this->GetWidth()*this->GetHeight()
  // \post is_set: this->GetValue(i)==value
  void SetValue(vtkIdType i,
                int value);
  
  // Description:
  // Get the value at index `i'.
  // \pre is_allocated: this->IsAllocated()
  // \pre valid_i: i>=0 i<this->GetWidth()*this->GetHeight()
  int GetValue(vtkIdType i);
  
  // Description:
  // Return item under abscissa x and ordinate y.
  // Abscissa go from left to right.
  // Ordinate go from bottom to top.
  // The return value is -1 if there is no item.
  // \pre is_allocated: IsAllocated()
  // \post valid_result: result>=-1
  vtkIdType GetPickedItem(int x, int y);
  
protected:
  vtkContextBufferId();
  virtual ~vtkContextBufferId();
  
  int Width;
  int Height;
  vtkIntArray *IdArray;
  
private:
  vtkContextBufferId(const vtkContextBufferId &); // Not implemented.
  void operator=(const vtkContextBufferId &);   // Not implemented.
};

#endif // #ifndef __vtkContextBufferId_h

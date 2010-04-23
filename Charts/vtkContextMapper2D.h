/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContextMapper2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkContextMapper2D - Abstract class for 2D context mappers.
//
// .SECTION Description
//
// This class provides an abstract base for 2D context mappers. They currently
// only accept vtkTable objects as input.

#ifndef __vtkContextMapper2D_h
#define __vtkContextMapper2D_h

#include "vtkAlgorithm.h"

class vtkContext2D;
class vtkTable;
class vtkDataArray;
class vtkAbstractArray;

class VTK_CHARTS_EXPORT vtkContextMapper2D : public vtkAlgorithm
{
public:
  vtkTypeMacro(vtkContextMapper2D, vtkAlgorithm);
  virtual void PrintSelf(ostream &os, vtkIndent indent);
  static vtkContextMapper2D *New();

  // Description:
  // Set/Get the input for this object - only accepts vtkTable as input.
  virtual void SetInput(vtkTable *input);
  virtual vtkTable * GetInput();

  // Description:
  // Make the arrays accessible to the plot objects.
  vtkDataArray *GetInputArrayToProcess(int idx,
                                       vtkDataObject* input)
    {
    return this->vtkAlgorithm::GetInputArrayToProcess(idx, input);
    }

  vtkAbstractArray *GetInputAbstractArrayToProcess(int idx,
                                       vtkDataObject* input)
    {
    return this->vtkAlgorithm::GetInputAbstractArrayToProcess(idx, input);
    }

//BTX
protected:
  vtkContextMapper2D();
  ~vtkContextMapper2D();

  // Description:
  // Specify the types of input we can handle.
  virtual int FillInputPortInformation(int port, vtkInformation *info);

private:
  vtkContextMapper2D(const vtkContextMapper2D &); // Not implemented.
  void operator=(const vtkContextMapper2D &); // Not implemented.
//ETX
};

#endif //__vtkContextMapper2D_h

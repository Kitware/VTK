/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageLogic.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageLogic - And, or, xor, nand, nor, not.
// .SECTION Description
// vtkImageLogic implements basic logic operations.
// SetOperation is used to select the filter's behavior.
// The filter can take two or one input. Inputs must have the same type.


#ifndef __vtkImageLogic_h
#define __vtkImageLogic_h


// Operation options.
#define VTK_AND          0
#define VTK_OR           1
#define VTK_XOR          2
#define VTK_NAND         3
#define VTK_NOR          4
#define VTK_NOT          5
#define VTK_NOP          6



#include "vtkImageTwoInputFilter.h"

class VTK_IMAGING_EXPORT vtkImageLogic : public vtkImageTwoInputFilter
{
public:
  static vtkImageLogic *New();
  vtkTypeRevisionMacro(vtkImageLogic,vtkImageTwoInputFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the Operation to perform.
  vtkSetMacro(Operation,int);
  vtkGetMacro(Operation,int);
  void SetOperationToAnd() {this->SetOperation(VTK_AND);};
  void SetOperationToOr() {this->SetOperation(VTK_OR);};
  void SetOperationToXor() {this->SetOperation(VTK_XOR);};
  void SetOperationToNand() {this->SetOperation(VTK_NAND);};
  void SetOperationToNor() {this->SetOperation(VTK_NOR);};
  void SetOperationToNot() {this->SetOperation(VTK_NOT);};

  // Description:
  // Set the value to use for true in the output.
  vtkSetMacro(OutputTrueValue, float);
  vtkGetMacro(OutputTrueValue, float);
  
protected:
  vtkImageLogic();
  ~vtkImageLogic() {};

  int Operation;
  float OutputTrueValue;
  
  void ThreadedExecute(vtkImageData **inDatas, vtkImageData *outData,
                       int extent[6], int id);
private:
  vtkImageLogic(const vtkImageLogic&);  // Not implemented.
  void operator=(const vtkImageLogic&);  // Not implemented.
};

#endif














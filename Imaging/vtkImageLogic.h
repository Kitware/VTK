/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageLogic.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
  vtkTypeMacro(vtkImageLogic,vtkImageTwoInputFilter);
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














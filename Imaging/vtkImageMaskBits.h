/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMaskBits.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

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
// .NAME vtkImageMaskBits - applies a bit-mask pattern to each component.
//
// .SECTION Description
// vtkImageMaskBits applies a bit-mask pattern to each component.  The bit-mask can be
// applied using a variety of boolean bitwise operators.


#ifndef __vtkImageMaskBits_h
#define __vtkImageMaskBits_h


#include "vtkImageToImageFilter.h"
#include "vtkImageLogic.h"     // included for the boolean operators

class VTK_IMAGING_EXPORT vtkImageMaskBits : public vtkImageToImageFilter
{
public:
  static vtkImageMaskBits *New();
  vtkTypeMacro(vtkImageMaskBits,vtkImageToImageFilter);
  void PrintSelf(ostream &, vtkIndent);
  
  // Description:
  // Set/Get the bit-masks. Default is 0xffffffff.
  vtkSetVector4Macro(Masks, unsigned int);
  void SetMask(unsigned int  mask)
    {this->SetMasks(mask, mask, mask, mask);}
  void SetMasks(unsigned int  mask1, unsigned int mask2)
    {this->SetMasks(mask1, mask2, 0xffffffff,  0xffffffff);}
  void SetMasks(unsigned int  mask1, unsigned int mask2, unsigned int mask3)
    {this->SetMasks(mask1, mask2, mask3,  0xffffffff);}
  vtkGetVector4Macro(Masks, unsigned int);

  // Description:
  // Set/Get the boolean operator. Default is AND.
  vtkSetMacro(Operation,int);
  vtkGetMacro(Operation,int);
  void SetOperationToAnd() {this->SetOperation(VTK_AND);};
  void SetOperationToOr() {this->SetOperation(VTK_OR);};
  void SetOperationToXor() {this->SetOperation(VTK_XOR);};
  void SetOperationToNand() {this->SetOperation(VTK_NAND);};
  void SetOperationToNor() {this->SetOperation(VTK_NOR);};
   
  
protected:
  vtkImageMaskBits();
  ~vtkImageMaskBits() {};

  void ThreadedExecute(vtkImageData *inData, vtkImageData *outData, 
		       int ext[6], int id);

  unsigned int Masks[4];
  int Operation;      
private:
  vtkImageMaskBits(const vtkImageMaskBits&);  // Not implemented.
  void operator=(const vtkImageMaskBits&);  // Not implemented.
};

#endif











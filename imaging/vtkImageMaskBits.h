/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMaskBits.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
// .NAME vtkImageMaskBits - applies a bit-mask pattern to each component.
//
// .SECTION Description
// vtkImageMaskBits applies a bit-mask pattern to each component.  The bit-mask can be
// applied using a variety of boolean bitwise operators.


#ifndef __vtkImageMaskBits_h
#define __vtkImageMaskBits_h


#include "vtkImageFilter.h"
#include "vtkImageLogic.h"     // included for the boolean operators

class VTK_EXPORT vtkImageMaskBits : public vtkImageFilter
{
 public:
  vtkImageMaskBits();
  static vtkImageMaskBits *New()
    {return new vtkImageMaskBits;};
  const char *GetClassName() {return "vtkImageMaskBits";};
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
  void ThreadedExecute(vtkImageData *inData, vtkImageData *outData, 
		       int ext[6], int id);

  unsigned int Masks[4];
  int Operation;      
};

#endif











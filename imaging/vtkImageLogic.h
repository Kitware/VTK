/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageLogic.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

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
// .NAME vtkImageLogic - And, or, xor, nand, nor, not.
// .SECTION Description
// vtkImageLogic implents basic logic operations.
// SetOperation is used to select the filteres behavior.
// The filter can take two or one input.  Output is always unsigned char,
// Inputs must have the same type.


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

class VTK_EXPORT vtkImageLogic : public vtkImageTwoInputFilter
{
public:
  vtkImageLogic();
  static vtkImageLogic *New() {return new vtkImageLogic;};
  const char *GetClassName() {return "vtkImageLogic";};

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
  vtkSetMacro(OutputTrueValue, unsigned char);
  vtkGetMacro(OutputTrueValue, unsigned char);
  
protected:
  int Operation;
  unsigned char OutputTrueValue;
  
  void Execute(vtkImageRegion *inRegion1, 
	       vtkImageRegion *inRegion2, 
	       vtkImageRegion *outRegion);
};

#endif














/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageArithmetic.h
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
// .NAME vtkImageArithmetic - Subtracts two images.
// .SECTION Description
// vtkImageArithmetic subtracts two images pixel by pixel.
// The two input and oputput data types all have to be the same.
//  (output = input1 - input2)


#ifndef __vtkImageArithmetic_h
#define __vtkImageArithmetic_h


// Operator options.
#define VTK_ADD          0
#define VTK_SUBTRACT     1
#define VTK_MULTIPLY     2
#define VTK_DIVIDE       3


#include "vtkImageTwoInputFilter.h"

class VTK_EXPORT vtkImageArithmetic : public vtkImageTwoInputFilter
{
public:
  vtkImageArithmetic();
  vtkImageArithmetic *New() {return new vtkImageArithmetic;};
  char *GetClassName() {return "vtkImageArithmetic";};

  // Description:
  // Set/Get the operator to perform.
  vtkSetMacro(Operator,int);
  vtkGetMacro(Operator,int);
  void SetOperatorAdd() {this->SetOperator(VTK_ADD);};
  void SetOperatorSubtract() {this->SetOperator(VTK_SUBTRACT);};
  void SetOperatorMultiply() {this->SetOperator(VTK_MULTIPLY);};
  void SetOperatorDivide() {this->SetOperator(VTK_DIVIDE);};

  
protected:
  int Operator;
  
  void Execute(vtkImageRegion *inRegion1, 
	       vtkImageRegion *inRegion2, 
	       vtkImageRegion *outRegion);
};

#endif




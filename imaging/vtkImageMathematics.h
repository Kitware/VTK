/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMathematics.h
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
// .NAME vtkImageMathematics - Add, subtract, multiply, divide, invert,
// sin, cos, exp, log.
// .SECTION Description
// vtkImageMathematics implents basic mathematic operations 
// SetOperation is used to select the filteres behavior.
// The filter can take two or one input.


#ifndef __vtkImageMathematics_h
#define __vtkImageMathematics_h


// Operation options.
#define VTK_ADD          0
#define VTK_SUBTRACT     1
#define VTK_MULTIPLY     2
#define VTK_DIVIDE       3
#define VTK_INVERT       4
#define VTK_SIN          5
#define VTK_COS          6
#define VTK_EXP          7
#define VTK_LOG          8


#include "vtkImageTwoInputFilter.h"

class VTK_EXPORT vtkImageMathematics : public vtkImageTwoInputFilter
{
public:
  vtkImageMathematics();
  static vtkImageMathematics *New() {return new vtkImageMathematics;};
  const char *GetClassName() {return "vtkImageMathematics";};

  // Description:
  // Set/Get the Operation to perform.
  vtkSetMacro(Operation,int);
  vtkGetMacro(Operation,int);
  void SetOperationToAdd() {this->SetOperation(VTK_ADD);};
  void SetOperationToSubtract() {this->SetOperation(VTK_SUBTRACT);};
  void SetOperationToMultiply() {this->SetOperation(VTK_MULTIPLY);};
  void SetOperationToDivide() {this->SetOperation(VTK_DIVIDE);};
  void SetOperationToInvert() {this->SetOperation(VTK_INVERT);};
  void SetOperationToSin() {this->SetOperation(VTK_SIN);};
  void SetOperationToCos() {this->SetOperation(VTK_COS);};
  void SetOperationToExp() {this->SetOperation(VTK_EXP);};
  void SetOperationToLog() {this->SetOperation(VTK_LOG);};

  
protected:
  int Operation;
  
  void Execute(vtkImageRegion *inRegion1, 
	       vtkImageRegion *inRegion2, 
	       vtkImageRegion *outRegion);
};

#endif














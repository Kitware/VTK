/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitDataSet.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkImplicitDataSet - treat a dataset as if it were an implicit function
// .SECTION Description
// vtkImplicitDataSet treats any type of dataset as if it were an
// implicit function. This means it computes a function value and 
// gradient. vtkImplicitDataSet is a concrete implementation of 
// vtkImplicitFunction.
//
// vtkImplicitDataSet computes the function (at the point x) by performing 
// cell interpolation. That is, it finds the cell containing x, and then
// uses the cell's interpolation functions to compute an interpolated
// scalar value at x. (A similar approach is used to find the
// gradient, if requested.) Points outside of the dataset are assigned 
// the value of the ivar OutValue, and the gradient value OutGradient.

// .SECTION Caveats
// Any type of dataset can be used as an implicit function as long as it
// has scalar data associated with it.

// .SECTION See Also
// vtkImplicitFunction vtkImplicitVolume vtkClipPolyData vtkCutter
// vtkImplicitWindowFunction

#ifndef __vtkImplicitDataSet_h
#define __vtkImplicitDataSet_h

#include "vtkImplicitFunction.h"
#include "vtkDataSet.h"

class VTK_EXPORT vtkImplicitDataSet : public vtkImplicitFunction
{
public:
  vtkImplicitDataSet();
  ~vtkImplicitDataSet();
  static vtkImplicitDataSet *New() {return new vtkImplicitDataSet;};
  char *GetClassName() {return "vtkImplicitDataSet";};
  void PrintSelf(ostream& os, vtkIndent indent);

  unsigned long int GetMTime();

  // ImplicitFunction interface
  float EvaluateFunction(float x[3]);
  void EvaluateGradient(float x[3], float n[3]);

  // Description:
  // Specify the dataset used for the implicit function evaluation.
  vtkSetObjectMacro(DataSet,vtkDataSet);
  vtkGetObjectMacro(DataSet,vtkDataSet);

  // Description:
  // Set the function value to use for points outside of the dataset.
  vtkSetMacro(OutValue,float);
  vtkGetMacro(OutValue,float);

  // Description:
  // Set the function gradient to use for points outside of the dataset.
  vtkSetVector3Macro(OutGradient,float);
  vtkGetVector3Macro(OutGradient,float);

protected:
  vtkDataSet *DataSet;
  float OutValue;
  float OutGradient[3];

  float *Weights; //used to compute interpolation weights
  int Size; //keeps track of length of weights array

};

#endif



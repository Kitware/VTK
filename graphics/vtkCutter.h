/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCutter.h
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
// .NAME vtkCutter - Cut vtkDataSet with user-specified implicit function
// .SECTION Description
// vtkCutter is a filter to cut through data using any subclass of 
// vtkImplicitFunction. That is, a polygonal surface is created
// corresponding to the implicit function F(x,y,z) = value(s), where
// you can specify one or more values used to cut with.
// .SECTION See Also
// vtkImplicitFunction vtkClipPolyData

#ifndef __vtkCutter_h
#define __vtkCutter_h

#include "vtkDataSetToPolyFilter.h"
#include "vtkImplicitFunction.h"

#define VTK_MAX_CONTOURS 256

class VTK_EXPORT vtkCutter : public vtkDataSetToPolyFilter
{
public:
  vtkCutter(vtkImplicitFunction *cf=NULL);
  vtkCutter *New() {return new vtkCutter;};
  char *GetClassName() {return "vtkCutter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  unsigned long int GetMTime();

  void SetValue(int i, float value);
  float GetValue(int i) {return this->Values[i];};

  // Description:
  // Return array of contour values (size of numContours).
  vtkGetVectorMacro(Values,float,VTK_MAX_CONTOURS);

  // Description:
  // Return the number of contour values. The number of values set (using SetValue)
  // should match the NumberOfContours ivar value.
  vtkSetMacro(NumberOfContours,int);
  vtkGetMacro(NumberOfContours,int);

  void GenerateValues(int numContours, float range[2]);
  void GenerateValues(int numContours, float range1, float range2);

  // Description
  // Specify the implicit function to perform the cutting.
  vtkSetObjectMacro(CutFunction,vtkImplicitFunction);
  vtkGetObjectMacro(CutFunction,vtkImplicitFunction);

  // Description:
  // If this flag is enabled, then the output scalar values will be interpolated
  // from the implicit function values, and not the input scalar data.
  vtkSetMacro(GenerateCutScalars,int);
  vtkGetMacro(GenerateCutScalars,int);
  vtkBooleanMacro(GenerateCutScalars,int);

  void SetLocator(vtkPointLocator *locator);
  void SetLocator(vtkPointLocator& locator) {this->SetLocator(&locator);};
  vtkGetObjectMacro(Locator,vtkPointLocator);

  // Description:
  // Create default locator. Used to create one when none is specified. The locator
  // is used to merge coincident points.
  void CreateDefaultLocator();

protected:
  void Execute();
  vtkImplicitFunction *CutFunction;
  
  vtkPointLocator *Locator;
  int SelfCreatedLocator;
  float Values[VTK_MAX_CONTOURS];
  int NumberOfContours;
  float Range[2];
  int GenerateCutScalars;
};

#endif



/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCleanPolyData.h
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
// .NAME vtkCleanPolyData - merge duplicate points and remove degenerate primitives
// .SECTION Description
// vtkCleanPolyData is a filter that takes polygonal data as input and 
// generates polygonal as output. vtkCleanPolyData merges duplicate 
// points (within specified tolerance), and transforms degenerate 
// topology into appropriate form (for example, triangle is converted
// into line if two points of triangle are merged).
//
// If tolerance is specified precisely=0.0, then this object will use
// the vtkMergePoints object to merge points (very fast). Otherwise the 
// slower vtkPointLocator is used.
// .SECTION Caveats
// Merging points can alter topology, including introducing non-manifold 
// forms. Tolerance should be chosen carefully to avoid these problems.

#ifndef __vtkCleanPolyData_h
#define __vtkCleanPolyData_h

#include "vtkPolyToPolyFilter.h"

class VTK_EXPORT vtkCleanPolyData : public vtkPolyToPolyFilter
{
public:
  vtkCleanPolyData();
  ~vtkCleanPolyData();
  static vtkCleanPolyData *New() {return new vtkCleanPolyData;};
  char *GetClassName() {return "vtkCleanPolyData";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify tolerance in terms of percentage of bounding box.
  vtkSetClampMacro(Tolerance,float,0.0,1.0);
  vtkGetMacro(Tolerance,float);

  void SetLocator(vtkPointLocator *locator);
  void SetLocator(vtkPointLocator& locator) {this->SetLocator(&locator);};
  vtkGetObjectMacro(Locator,vtkPointLocator);

  // Description:
  // Create default locator. Used to create one when none is specified.
  void CreateDefaultLocator();

protected:
  // Usual data generation method
  void Execute();

  float Tolerance;
  vtkPointLocator *Locator;
  int SelfCreatedLocator;
};

#endif



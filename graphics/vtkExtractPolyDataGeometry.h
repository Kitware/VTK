/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractPolyDataGeometry.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkExtractPolyDataGeometry - extract poly data that lies either entirely inside or outside of a specified implicit function

// .SECTION Description
// vtkExtractPolyDataGeometry extracts from its input vtkPolyData all cells
// that are either completely inside or outside of a specified implicit
// function. Any type of dataset can be input to this filter. On output the
// filter generates an unstructured grid.
//
// To use this filter you must specify an implicit function. You can also
// choose to extract the inside or outside data. (The inside of an implicit
// function is the negative values region.)

// .SECTION See Also
// vtkExtractGeometry vtkClipPolyData

#ifndef __vtkExtractPolyDataGeometry_h
#define __vtkExtractPolyDataGeometry_h

#include "vtkPolyDataToPolyDataFilter.h"
#include "vtkImplicitFunction.h"

class VTK_EXPORT vtkExtractPolyDataGeometry : public vtkPolyDataToPolyDataFilter
{
public:
  vtkTypeMacro(vtkExtractPolyDataGeometry,vtkPolyDataToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object with ExtractInside turned on.
  static vtkExtractPolyDataGeometry *New();

  // Description:
  // Return the MTime taking into account changes to the implicit function
  unsigned long GetMTime();

  // Description:
  // Specify the implicit function for inside/outside checks.
  vtkSetObjectMacro(ImplicitFunction,vtkImplicitFunction);
  vtkGetObjectMacro(ImplicitFunction,vtkImplicitFunction);

  // Description:
  // Boolean controls whether to extract cells that are inside of implicit 
  // function (ExtractInside == 1) or outside of implicit function 
  // (ExtractInside == 0).
  vtkSetMacro(ExtractInside,int);
  vtkGetMacro(ExtractInside,int);
  vtkBooleanMacro(ExtractInside,int);

protected:
  vtkExtractPolyDataGeometry(vtkImplicitFunction *f=NULL);
  ~vtkExtractPolyDataGeometry();
  vtkExtractPolyDataGeometry(const vtkExtractPolyDataGeometry&) {};
  void operator=(const vtkExtractPolyDataGeometry&) {};

  // Usual data generation method
  void Execute();

  vtkImplicitFunction *ImplicitFunction;
  int ExtractInside;
};

#endif



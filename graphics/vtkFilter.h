/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFilter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkFilter - abstract class for specifying filter behavior
// .SECTION Description
// vtkFilter is an abstract class that specifies the interface for data 
// filters. Each filter must have an Update() and Execute() method 
// that will cause the filter to execute if its input or the filter itself 
// has been modified since the last execution time.

// .SECTION See Also
// vtkSource
// vtkAppendPolyData vtkBooleanStructuredPoints vtkExtractVectorComponents
// vtkMergeFilter vtkDataSetFilter vtkPointSetFilter
// vtkPolyDataFilter vtkStructuredGridFilter vtkStructuredPointsFilter
// vtkUnstructuredGridFilter

#ifndef __vtkFilter_h
#define __vtkFilter_h

#include "vtkSource.h"
class vtkDataSet;

class VTK_EXPORT vtkFilter : public vtkSource
{
public:
  vtkFilter();
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkFilter *New() {return new vtkFilter;};
  const char *GetClassName() {return "vtkFilter";};

  // Description:
  // All filters must provide a method to update the visualization 
  // pipeline. (Method interface inherited from vtkSource.)
  void Update();

protected:
  vtkDataSet *Input;
  char Updating;

  // Every filter must have execute method.
  void Execute();
};

#endif



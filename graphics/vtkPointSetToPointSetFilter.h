/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointSetToPointSetFilter.h
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
// .NAME vtkPointSetToPointSetFilter - abstract filter class 
// .SECTION Description
// vtkPointSetToPointSetFilter is an abstract filter class whose subclasses
// take as input a point set and generates a point set on output.  At a
// minimum, the concrete subclasses of vtkPointSetToPointSetFilter modify
// their point coordinates. They never modify their topological form,
// however.
//
// This is an abstract filter type. What that means is that the output of the
// filter is an abstract type (i.e., vtkPointSet), no matter what the input
// of the filter is. This can cause problems connecting together filters due
// to the change in dataset type. (For example, in a series of filters
// processing vtkPolyData, when a vtkPointSetToPointSetFilter or subclass is
// introduced into the pipeline, if the filter downstream of it takes
// vtkPolyData as input, the pipeline connection cannot be made.) To get
// around this problem, use one of the convenience methods to return a
// concrete type (e.g., vtkGetPolyDataOutput(), GetStructuredGridOutput(),
// etc.).

// .SECTION See Also
// vtkTransformFilter vtkWarpScalar vtkWarpTo vtkWarpVector

#ifndef __vtkPointSetToPointSetFilter_h
#define __vtkPointSetToPointSetFilter_h

#include "vtkPointSetFilter.h"
#include "vtkPointSet.h"

class vtkPolyData;
class vtkStructuredGrid;
class vtkUnstructuredGrid;

class VTK_EXPORT vtkPointSetToPointSetFilter : public vtkPointSetFilter
{
public:
  vtkPointSetToPointSetFilter();
  ~vtkPointSetToPointSetFilter();
  static vtkPointSetToPointSetFilter *New() {
    return new vtkPointSetToPointSetFilter;};
  const char *GetClassName() {return "vtkPointSetToPointSetFilter";};
  
  // Description:
  // Specify the input data or filter.
  void SetInput(vtkPointSet *input);

  // Description:
  // Update input to this filter and the filter itself. Note that we are 
  // overloading this method because the output is an abstract dataset type.
  // This requires special treatment.
  void Update();

  // Description:
  // Get the output of this filter. If output is NULL, then input hasn't been
  // set, which is necessary for abstract filter objects.
  vtkPointSet *GetOutput();

  // Description:
  // Get the output as vtkPolyData. Performs run-time checking.
  vtkPolyData *GetPolyDataOutput();

  // Description:
  // Get the output as vtkStructuredGrid. Performs run-time checking.
  vtkStructuredGrid *GetStructuredGridOutput();

  // Description:
  // Get the output as vtkUnstructuredGrid. Performs run-time checking.
  vtkUnstructuredGrid *GetUnstructuredGridOutput();

protected:
  // objects used to support the retrieval of output
  vtkPolyData *PolyData;
  vtkStructuredGrid *StructuredGrid;
  vtkUnstructuredGrid *UnstructuredGrid;

};

#endif



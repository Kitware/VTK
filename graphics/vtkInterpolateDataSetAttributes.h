/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInterpolateDataSetAttributes.h
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
// .NAME vtkInterpolateDataSetAttributes - interpolate scalars, vectors, etc. and other dataset attributes
// .SECTION Description
// vtkInterpolateDataSetAttributes is a filter that interpolates data set
// attribute values between input data sets. The input to the filter
// must be datasets of the same type, same number of cells, and same 
// number of points. The output of the filter is a data set of the same
// type as the input dataset and whose attribute values have been 
// interpolated at the parametric value specified.
//
// The filter is used by specifying two or more input data sets (total of N),
// and a parametric value t (0 <= t <= N-1). The output will contain
// interpolated data set attributes common to all input data sets. (For
// example, if one input has scalars and vectors, and another has just
// scalars, then only scalars will be interpolated and output.)

#ifndef __vtkInterpolateDataSetAttributes_h
#define __vtkInterpolateDataSetAttributes_h

#include "vtkDataSetToDataSetFilter.h"
#include "vtkDataSetCollection.h"

class VTK_EXPORT vtkInterpolateDataSetAttributes : public vtkDataSetToDataSetFilter
{
public:
  static vtkInterpolateDataSetAttributes *New() {return new vtkInterpolateDataSetAttributes;};
  const char *GetClassName() {return "vtkInterpolateDataSetAttributes";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Add a dataset to the list of data to interpolate.
  void AddInput(vtkDataSet *in);

  // Description:
  // Return the list of inputs to this filter.
  vtkDataSetCollection *GetInputList();
  
  // Description:
  // Specify interpolation parameter t.
  vtkSetClampMacro(T,float,0.0,VTK_LARGE_FLOAT);
  vtkGetMacro(T,float);

protected:
  vtkInterpolateDataSetAttributes();
  ~vtkInterpolateDataSetAttributes();
  vtkInterpolateDataSetAttributes(const vtkInterpolateDataSetAttributes&) {};
  void operator=(const vtkInterpolateDataSetAttributes&) {};

  void Execute();
  int ComputeInputUpdateExtents(vtkDataObject *output);
  
  vtkDataSetCollection *InputList; // list of data sets to interpolate 
  float T; // interpolation parameter

};

#endif



/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInterpolateDataSetAttributes.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

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

class VTK_GRAPHICS_EXPORT vtkInterpolateDataSetAttributes : public vtkDataSetToDataSetFilter
{
public:
  static vtkInterpolateDataSetAttributes *New();
  vtkTypeRevisionMacro(vtkInterpolateDataSetAttributes,vtkDataSetToDataSetFilter);
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

  void Execute();
  
  vtkDataSetCollection *InputList; // list of data sets to interpolate 
  float T; // interpolation parameter

private:
  // hide the superclass' AddInput() from the user and the compiler
  void AddInput(vtkDataObject *)
    { vtkErrorMacro( << "AddInput() must be called with a vtkDataSet not a vtkDataObject."); };
  void RemoveInput(vtkDataObject *input)
    { this->vtkProcessObject::RemoveInput(input); };
private:
  vtkInterpolateDataSetAttributes(const vtkInterpolateDataSetAttributes&);  // Not implemented.
  void operator=(const vtkInterpolateDataSetAttributes&);  // Not implemented.
};

#endif



/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractVectorComponents.h
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
// .NAME vtkExtractVectorComponents - extract components of vector as separate scalars
// .SECTION Description
// vtkExtractVectorComponents is a filter that extracts vector components as
// separate scalars. This is accomplished by creating three different outputs.
// Each output is the same as the input, except that the scalar values will be
// one of the three components of the vector. These can be found in the
// VxComponent, VyComponent, and VzComponent.
// Alternatively, if the ExtractToFieldData flag is set, the filter will
// put all the components in the field data. The first component will be
// the scalar and the others will be non-attribute arrays.

// .SECTION Caveats
// This filter is unusual in that it creates multiple outputs. 
// If you use the GetOutput() method, you will be retrieving the x vector 
// component.

#ifndef __vtkExtractVectorComponents_h
#define __vtkExtractVectorComponents_h

#include "vtkSource.h"
#include "vtkDataSet.h"

class VTK_GRAPHICS_EXPORT vtkExtractVectorComponents : public vtkSource
{
public:
  static vtkExtractVectorComponents *New();
  vtkTypeRevisionMacro(vtkExtractVectorComponents,vtkSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the input data or filter.
  virtual void SetInput(vtkDataSet *input);

  // Description:
  // Get the input data or filter.
  vtkDataSet *GetInput();

  // Description:
  // Get the output dataset representing velocity x-component. If output is
  // NULL then input hasn't been set, which is necessary for abstract
  // objects. (Note: this method returns the same information as the
  // GetOutput() method with an index of 0.)
  vtkDataSet *GetVxComponent();

  // Description:
  // Get the output dataset representing velocity y-component. If output is
  // NULL then input hasn't been set, which is necessary for abstract
  // objects. (Note: this method returns the same information as the
  // GetOutput() method with an index of 1.)
  // Note that if ExtractToFieldData is true, this output will be empty.
  vtkDataSet *GetVyComponent();
  
  // Description:
  // Get the output dataset representing velocity z-component. If output is
  // NULL then input hasn't been set, which is necessary for abstract
  // objects. (Note: this method returns the same information as the
  // GetOutput() method with an index of 2.)
  // Note that if ExtractToFieldData is true, this output will be empty.
  vtkDataSet *GetVzComponent();

  // Description:
  // Get the output dataset containing the indicated component. The component
  // is specified by an index between (0,2) corresponding to the x, y, or z
  // vector component. By default, the x component is extracted.
  vtkDataSet *GetOutput(int i=0); //default extracts vector component.

  // Description:
  // Determines whether the vector components will be put
  // in separate outputs or in the first output's field data
  vtkSetMacro(ExtractToFieldData, int);
  vtkGetMacro(ExtractToFieldData, int);
  vtkBooleanMacro(ExtractToFieldData, int);

protected:
  vtkExtractVectorComponents();
  ~vtkExtractVectorComponents();

  void Execute();
  int ExtractToFieldData;
private:
  vtkExtractVectorComponents(const vtkExtractVectorComponents&);  // Not implemented.
  void operator=(const vtkExtractVectorComponents&);  // Not implemented.
};

#endif



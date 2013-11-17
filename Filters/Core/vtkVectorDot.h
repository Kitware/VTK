/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVectorDot.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkVectorDot - generate scalars from dot product of vectors and normals (e.g., show displacement plot)
// .SECTION Description
// vtkVectorDot is a filter to generate scalar values from a dataset.
// The scalar value at a point is created by computing the dot product
// between the normal and vector at that point. Combined with the appropriate
// color map, this can show nodal lines/mode shapes of vibration, or a
// displacement plot.

#ifndef __vtkVectorDot_h
#define __vtkVectorDot_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkDataSetAlgorithm.h"

class VTKFILTERSCORE_EXPORT vtkVectorDot : public vtkDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkVectorDot,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object with scalar range is (-1,1).
  static vtkVectorDot *New();

  // Description:
  // Specify range to map scalars into.
  vtkSetVector2Macro(ScalarRange,double);

  // Description:
  // Get the range that scalars map into.
  vtkGetVectorMacro(ScalarRange,double,2);

protected:
  vtkVectorDot();
  ~vtkVectorDot() {}

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  double ScalarRange[2];
private:
  vtkVectorDot(const vtkVectorDot&);  // Not implemented.
  void operator=(const vtkVectorDot&);  // Not implemented.
};

#endif

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetToPolyDataFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkDataSetToPolyDataFilter - abstract filter class
// .SECTION Description
// vtkDataSetToPolyDataFilter is an abstract filter class whose subclasses 
// take as input any dataset and generate polygonal data on output.

// .SECTION See Also
// vtkContourFilter vtkCutter vtkEdgePoints vtkExtractEdges
// vtkGeometryFilter vtkGlyph3D vtkHedgeHog vtkHyperStreamline
// vtkMaskPoints vtkOutlineFilter vtkStreamer vtkTensorGlyph
// vtkThresholdPoints vtkVectorTopology

#ifndef __vtkDataSetToPolyDataFilter_h
#define __vtkDataSetToPolyDataFilter_h

#include "vtkPolyDataSource.h"
 
class vtkDataSet;

class VTK_FILTERING_EXPORT vtkDataSetToPolyDataFilter : public vtkPolyDataSource
{
public:
  vtkTypeMacro(vtkDataSetToPolyDataFilter,vtkPolyDataSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set / get the input data or filter.
  virtual void SetInput(vtkDataSet *input);
  vtkDataSet *GetInput();
  
  // Description:
  // Do not let images return more than requested.
  virtual void ComputeInputUpdateExtents( vtkDataObject *output );

protected:
  vtkDataSetToPolyDataFilter();
  ~vtkDataSetToPolyDataFilter();

  virtual int FillInputPortInformation(int, vtkInformation*);

private:
  vtkDataSetToPolyDataFilter(const vtkDataSetToPolyDataFilter&);  // Not implemented.
  void operator=(const vtkDataSetToPolyDataFilter&);  // Not implemented.
};

#endif



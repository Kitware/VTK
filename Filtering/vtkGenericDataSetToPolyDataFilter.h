/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericDataSetToPolyDataFilter.h
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
// .NAME vtkGenericDataSetToPolyDataFilter - abstract filter class
// .SECTION Description
// vtkGenericDataSetToPolyDataFilter is an abstract filter class whose
// subclasses take as input any dataset and generate polygonal data on output.

// .SECTION See Also
// vtkContourFilter vtkCutter vtkEdgePoints vtkExtractEdges
// vtkGeometryFilter vtkGlyph3D vtkHedgeHog vtkHyperStreamline
// vtkMaskPoints vtkOutlineFilter vtkStreamer vtkTensorGlyph
// vtkThresholdPoints vtkVectorTopology

#ifndef __vtkGenericDataSetToPolyDataFilter_h
#define __vtkGenericDataSetToPolyDataFilter_h

#include "vtkPolyDataSource.h"

class vtkGenericDataSet;

class VTK_FILTERING_EXPORT vtkGenericDataSetToPolyDataFilter : public vtkPolyDataSource
{
public:
  vtkTypeRevisionMacro(vtkGenericDataSetToPolyDataFilter,vtkPolyDataSource);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Set / get the input data or filter.
  virtual void SetInput(vtkGenericDataSet *input);
  vtkGenericDataSet *GetInput();
  
  // Description:
  // Do not let images return more than requested.
  virtual void ComputeInputUpdateExtents( vtkDataObject *output );

protected:
  vtkGenericDataSetToPolyDataFilter();
  ~vtkGenericDataSetToPolyDataFilter() {};
#if VTK_MAJOR_VERSION>4 || (VTK_MAJOR_VERSION==4 && VTK_MINOR_VERSION>4)
  virtual int FillInputPortInformation(int, vtkInformation*);
#endif

  
private:
  vtkGenericDataSetToPolyDataFilter(const vtkGenericDataSetToPolyDataFilter&); // Not implemented
  void operator=(const vtkGenericDataSetToPolyDataFilter&); // Not implemented
};

#endif

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractRectilinearGrid.h
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
// .NAME vtkExtractRectilinearGrid - Extract a sub grid (VOI) from the structured rectilinear dataset.
// .SECTION Description
// vtkExtractRectilinearGrid rounds out the set of filters that extract
// a subgrid out of a larger structured data set.  RIght now, this filter
// only supports extracting a VOI.  In the future, it might support
// strides like the vtkExtract grid filter.

// .SECTION See Also
// vtkExtractGrid vtkImageClip vtkGeometryFilter vtkExtractGeometry vtkExtractVOI 
// vtkStructuredGridGeometryFilter

#ifndef __vtkExtractRectilinearGrid_h
#define __vtkExtractRectilinearGrid_h

#include "vtkRextilinearGridToRectilinearGridFilter.h"

class VTK_GRAPHICS_EXPORT vtkExtractRectilinearGrid : public vtkRectilinearGridToRectilinearGridFilter
{
public:
  static vtkExtractRectilinearGrid *New();
  vtkTypeRevisionMacro(vtkExtractRectilinearGrid,vtkRectilinearGridToRectilinearGridFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify i-j-k (min,max) pairs to extract. The resulting structured grid
  // dataset can be of any topological dimension (i.e., point, line, plane,
  // or 3D grid). 
  vtkSetVector6Macro(VOI,int);
  vtkGetVectorMacro(VOI,int,6);

  // Description:
  // Set the sampling rate in the i, j, and k directions. If the rate is > 1,
  // then the resulting VOI will be subsampled representation of the input.
  // For example, if the SampleRate=(2,2,2), every other point will be
  // selected, resulting in a volume 1/8th the original size.
  vtkSetVector3Macro(SampleRate, int);
  vtkGetVectorMacro(SampleRate, int, 3);

protected:
  vtkExtractRectilinearGrid();
  ~vtkExtractRectilinearGrid() {};

  void Execute();
  void ExecuteInformation();
  void ComputeInputUpdateExtents(vtkDataObject *out);
  
  int VOI[6];
  
private:
  vtkExtractRectilinearGrid(const vtkExtractRectilinearGrid&);  // Not implemented.
  void operator=(const vtkExtractRectilinearGrid&);  // Not implemented.
};

#endif



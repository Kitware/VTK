/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractGrid.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkExtractGrid - select piece (e.g., volume of interest) and/or subsample structured grid dataset

// .SECTION Description
// vtkExtractGrid is a filter that selects a portion of an input structured
// grid dataset, or subsamples an input dataset. (The selected portion of
// interested is referred to as the Volume Of Interest, or VOI.) The output of
// this filter is a structured grid dataset. The filter treats input data of
// any topological dimension (i.e., point, line, image, or volume) and can
// generate output data of any topological dimension.
//
// To use this filter set the VOI ivar which are i-j-k min/max indices that
// specify a rectangular region in the data. (Note that these are 0-offset.)
// You can also specify a sampling rate to subsample the data.
//
// Typical applications of this filter are to extract a plane from a grid for 
// contouring, subsampling large grids to reduce data size, or extracting
// regions of a grid with interesting data.

// .SECTION See Also
// vtkGeometryFilter vtkExtractGeometry vtkExtractVOI 
// vtkStructuredGridGeometryFilter

#ifndef __vtkExtractGrid_h
#define __vtkExtractGrid_h

#include "vtkStructuredGridAlgorithm.h"

class VTK_GRAPHICS_EXPORT vtkExtractGrid : public vtkStructuredGridAlgorithm
{
public:
  static vtkExtractGrid *New();
  vtkTypeMacro(vtkExtractGrid,vtkStructuredGridAlgorithm);
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
  // Initial value is (1,1,1).
  vtkSetVector3Macro(SampleRate, int);
  vtkGetVectorMacro(SampleRate, int, 3);

  // Description:
  // Control whether to enforce that the "boundary" of the grid is output in
  // the subsampling process. (This ivar only has effect when the SampleRate
  // in any direction is not equal to 1.) When this ivar IncludeBoundary is
  // on, the subsampling will always include the boundary of the grid even
  // though the sample rate is not an even multiple of the grid
  // dimensions. (By default IncludeBoundary is off.)
  vtkSetMacro(IncludeBoundary,int);
  vtkGetMacro(IncludeBoundary,int);
  vtkBooleanMacro(IncludeBoundary,int);

protected:
  vtkExtractGrid();
  ~vtkExtractGrid() {};

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int RequestUpdateExtent(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  
  int VOI[6];
  int SampleRate[3];
  int IncludeBoundary;
  
private:
  vtkExtractGrid(const vtkExtractGrid&);  // Not implemented.
  void operator=(const vtkExtractGrid&);  // Not implemented.
};

#endif



/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractRectilinearGrid.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
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

#ifndef vtkExtractRectilinearGrid_h
#define vtkExtractRectilinearGrid_h

#include "vtkFiltersExtractionModule.h" // For export macro
#include "vtkRectilinearGridAlgorithm.h"

// Forward Declarations
class vtkExtractStructuredGridHelper;

class VTKFILTERSEXTRACTION_EXPORT vtkExtractRectilinearGrid : public vtkRectilinearGridAlgorithm
{
public:
  static vtkExtractRectilinearGrid *New();
  vtkTypeMacro(vtkExtractRectilinearGrid,vtkRectilinearGridAlgorithm);
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
  vtkExtractRectilinearGrid();
  ~vtkExtractRectilinearGrid();

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int RequestUpdateExtent(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  // Description:
  // Implementation for RequestData using a specified VOI. This is because the
  // parallel filter needs to muck around with the VOI to get spacing and
  // partitioning to play nice.
  bool RequestDataImpl(int voi[6],
                       vtkInformationVector **inputVector,
                       vtkInformationVector *outputVector);

  int VOI[6];
  int SampleRate[3];
  int IncludeBoundary;

  vtkExtractStructuredGridHelper* Internal;
private:
  vtkExtractRectilinearGrid(const vtkExtractRectilinearGrid&);  // Not implemented.
  void operator=(const vtkExtractRectilinearGrid&);  // Not implemented.
};

#endif



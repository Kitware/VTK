/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractGrid.h
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
// .NAME vtkExtractGrid - select piece (e.g., volume of interest) and/or subsample structured grid dataset

// .SECTION Description
// vtkExtractGrid is a filter that selects a portion of an input structured grid
// dataset, or subsamples an input dataset. (The selected portion of interested is
// referred toas the Volume Of Interest, or VOI.) The output of this filter is a 
// structured grid dataset. The filter treats input data of any topological
// dimension (i.e., point, line, image, or volume) and can generate output data 
// of any topological dimension.
//
// To use this filter set the VOI ivar which are i-j-k min/max indices that specify
// a rectangular region in the data. (Note that these are 0-offset.) You can also
// specify a sampling rate to subsample the data.
//
// Typical applications of this filter are to extract a plane from a grid for 
// contouring, subsampling large grids to reduce data size, or extracting
// regions of a grid with interesting data.

// .SECTION See Also
// vtkGeometryFilter vtkExtractGeometry vtkExtractVOI 
// vtkStructuredGridGeometryFilter

#ifndef __vtkExtractGrid_h
#define __vtkExtractGrid_h

#include "vtkStructuredGridToStructuredGridFilter.h"

class VTK_EXPORT vtkExtractGrid : public vtkStructuredGridToStructuredGridFilter
{
public:
  vtkExtractGrid();
  static vtkExtractGrid *New() {return new vtkExtractGrid;};
  const char *GetClassName() {return "vtkExtractGrid";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify i-j-k (min,max) pairs to extract. The resulting structured grid
  // dataset can be of any topological dimension (i.e., point, line, plane,
  // or 3D grid). 
  vtkSetVectorMacro(VOI,int,6);
  vtkGetVectorMacro(VOI,int,6);
  void SetVOI(int imin, int imax, int jmin, int jmax, int kmin, int kmax);

  // Description:
  // Set the sampling rate in the i, j, and k directions. If the rate is > 1, 
  // then the resulting VOI will be subsampled representation of the input. 
  // For example, if the SampleRate=(2,2,2), every other point will be selected,
  // resulting in a volume 1/8th the original size.
  vtkSetVector3Macro(SampleRate, int);
  vtkGetVectorMacro(SampleRate, int, 3);

protected:
  void Execute();

  int VOI[6];
  int SampleRate[3];
};

#endif



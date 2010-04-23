/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractVOI.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkExtractVOI - select piece (e.g., volume of interest) and/or subsample structured points dataset

// .SECTION Description
// vtkExtractVOI is a filter that selects a portion of an input structured
// points dataset, or subsamples an input dataset. (The selected portion of
// interested is referred to as the Volume Of Interest, or VOI.) The output of
// this filter is a structured points dataset. The filter treats input data
// of any topological dimension (i.e., point, line, image, or volume) and can
// generate output data of any topological dimension.
//
// To use this filter set the VOI ivar which are i-j-k min/max indices that
// specify a rectangular region in the data. (Note that these are 0-offset.)
// You can also specify a sampling rate to subsample the data.
//
// Typical applications of this filter are to extract a slice from a volume
// for image processing, subsampling large volumes to reduce data size, or
// extracting regions of a volume with interesting data.

// .SECTION See Also
// vtkGeometryFilter vtkExtractGeometry vtkExtractGrid

#ifndef __vtkExtractVOI_h
#define __vtkExtractVOI_h

#include "vtkImageAlgorithm.h"

class VTK_IMAGING_EXPORT vtkExtractVOI : public vtkImageAlgorithm
{
public:
  vtkTypeMacro(vtkExtractVOI,vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object to extract all of the input data.
  static vtkExtractVOI *New();

  // Description:
  // Specify i-j-k (min,max) pairs to extract. The resulting structured points
  // dataset can be of any topological dimension (i.e., point, line, image, 
  // or volume). 
  vtkSetVector6Macro(VOI,int);
  vtkGetVectorMacro(VOI,int,6);

  // Description:
  // Set the sampling rate in the i, j, and k directions. If the rate is >
  // 1, then the resulting VOI will be subsampled representation of the
  // input.  For example, if the SampleRate=(2,2,2), every other point will
  // be selected, resulting in a volume 1/8th the original size.
  vtkSetVector3Macro(SampleRate, int);
  vtkGetVectorMacro(SampleRate, int, 3);

protected:
  vtkExtractVOI();
  ~vtkExtractVOI() {};

  virtual int RequestUpdateExtent(vtkInformation*,
                                  vtkInformationVector**,
                                  vtkInformationVector*);
  virtual int RequestInformation (vtkInformation*,
                                  vtkInformationVector**,
                                  vtkInformationVector*);
  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);

  int VOI[6];
  int SampleRate[3];
private:
  vtkExtractVOI(const vtkExtractVOI&);  // Not implemented.
  void operator=(const vtkExtractVOI&);  // Not implemented.
};

#endif



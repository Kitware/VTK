/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageAccumulate.h
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
// .NAME vtkImageAccumulate - Generalized histograms up to 4 dimensions.
// .SECTION Description
// vtkImageAccumulate - This filter divides component space into
// discrete bins.  It then counts the number of pixels associated
// with each bin.  The output is this "scatter plot" (histogram values for 1D).
// The dimensionality of the output depends on how many components the 
// input pixels have.  Input pixels with one component generate a 1D histogram.
// This filter can only handle images with 1 to 3 scalar components.
// The input can be any type, but the output is always int.
// Some statistics are computed on the pixel values at the same time.
// The SetStencilFunction, SetClippingExtents and ReverseStencil
// functions allow the statistics to be computed on an arbitrary
// portion of the input data.
// See the documentation for vtkImageStencil for more information.


#ifndef __vtkImageAccumulate_h
#define __vtkImageAccumulate_h


#include "vtkImageToImageFilter.h"
#include "vtkImageStencilData.h"

class VTK_IMAGING_EXPORT vtkImageAccumulate : public vtkImageToImageFilter
{
public:
  static vtkImageAccumulate *New();
  vtkTypeRevisionMacro(vtkImageAccumulate,vtkImageToImageFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get - The component spacing is the dimension of each bin.
  // This ends up being the spacing of the output "image".
  // If the number of input scalar components are less than three,
  // then some of these spacing values are ignored.
  // For a 1D histogram with 10 bins spanning the values 1000 to 2000,
  // this spacing should be set to 100, 0, 0
  vtkSetVector3Macro(ComponentSpacing, float);
  vtkGetVector3Macro(ComponentSpacing, float);

  // Description:
  // Set/Get - The component origin is the location of bin (0, 0, 0).
  // Note that if the Component extent does not include the value (0,0,0),
  // then this origin bin will not actually be in the output.
  // The origin of the output ends up being the same as the componenet origin.
  // For a 1D histogram with 10 bins spanning the values 1000 to 2000,
  // this origin should be set to 1000, 0, 0
  vtkSetVector3Macro(ComponentOrigin, float);
  vtkGetVector3Macro(ComponentOrigin, float);

  // Description:
  // Set/Get - The component extent sets the number/extent of the bins.
  // For a 1D histogram with 10 bins spanning the values 1000 to 2000,
  // this extent should be set to 0, 9, 0, 0, 0, 0.
  // The extent specifies inclusive min/max values.  
  // This implies the the top extent should be set to the number of bins - 1.
  void SetComponentExtent(int extent[6]);
  void SetComponentExtent(int minX, int maxX, int minY, int maxY,
        int minZ, int maxZ);
  void GetComponentExtent(int extent[6]);
  int *GetComponentExtent() {return this->ComponentExtent;}


  // Description:
  // Use a stencil to specify which voxels to accumulate.
  void SetStencil(vtkImageStencilData *stencil);
  vtkImageStencilData *GetStencil();

  // Description:
  // Reverse the stencil.
  vtkSetMacro(ReverseStencil, int);
  vtkBooleanMacro(ReverseStencil, int);
  vtkGetMacro(ReverseStencil, int);

  // Description:
  // Get the statistics information for the data.
  vtkGetVector3Macro(Min, double);
  vtkGetVector3Macro(Max, double);
  vtkGetVector3Macro(Mean, double);
  vtkGetVector3Macro(StandardDeviation, double);
  vtkGetMacro(VoxelCount, long int);
 
  
protected:
  vtkImageAccumulate();
  ~vtkImageAccumulate();

  float ComponentSpacing[3];
  float ComponentOrigin[3];
  int ComponentExtent[6];

  void ExecuteInformation(vtkImageData *input, vtkImageData *output);
  void ComputeInputUpdateExtent(int inExt[6], int outExt[6]);
  void ExecuteInformation(){this->vtkImageToImageFilter::ExecuteInformation();};
  void ExecuteData(vtkDataObject *out);

  double Min[3];
  double Max[3];
  double Mean[3];
  double StandardDeviation[3];
  long int VoxelCount;

  int ReverseStencil;

private:
  vtkImageAccumulate(const vtkImageAccumulate&);  // Not implemented.
  void operator=(const vtkImageAccumulate&);  // Not implemented.
};

#endif




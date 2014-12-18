/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSubPixelPositionEdgels.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSubPixelPositionEdgels - adjust edgel locations based on gradients.
// .SECTION Description
// vtkSubPixelPositionEdgels is a filter that takes a series of linked
// edgels (digital curves) and gradient maps as input. It then adjusts
// the edgel locations based on the gradient data. Specifically, the
// algorithm first determines the neighboring gradient magnitudes of
// an edgel using simple interpolation of its neighbors. It then fits
// the following three data points: negative gradient direction
// gradient magnitude, edgel gradient magnitude and positive gradient
// direction gradient magnitude to a quadratic function. It then
// solves this quadratic to find the maximum gradient location along
// the gradient orientation.  It then modifies the edgels location
// along the gradient orientation to the calculated maximum
// location. This algorithm does not adjust an edgel in the direction
// orthogonal to its gradient vector.

// .SECTION see also
// vtkImageData vtkImageGradient vtkLinkEdgels

#ifndef vtkSubPixelPositionEdgels_h
#define vtkSubPixelPositionEdgels_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkStructuredPoints;
class vtkDataArray;

class VTKFILTERSGENERAL_EXPORT vtkSubPixelPositionEdgels : public vtkPolyDataAlgorithm
{
public:
  static vtkSubPixelPositionEdgels *New();
  vtkTypeMacro(vtkSubPixelPositionEdgels,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the gradient data for doing the position adjustments.
  void SetGradMapsData(vtkStructuredPoints *gm);
  vtkStructuredPoints *GetGradMaps();

  // Description:
  // These methods can make the positioning look for a target scalar value
  // instead of looking for a maximum.
  vtkSetMacro(TargetFlag, int);
  vtkGetMacro(TargetFlag, int);
  vtkBooleanMacro(TargetFlag, int);
  vtkSetMacro(TargetValue, double);
  vtkGetMacro(TargetValue, double);

protected:
  vtkSubPixelPositionEdgels();
  ~vtkSubPixelPositionEdgels();

  // Usual data generation method
  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int FillInputPortInformation(int port, vtkInformation *info);

  void Move(int xdim, int ydim, int zdim, int x, int y,
            float *img, vtkDataArray *inVecs,
            double *result, int z, double *aspect, double *resultNormal);
  void Move(int xdim, int ydim, int zdim, int x, int y,
            double *img, vtkDataArray *inVecs,
            double *result, int z, double *aspect, double *resultNormal);
  // extension for target instead of maximum
  int TargetFlag;
  double TargetValue;
private:
  vtkSubPixelPositionEdgels(const vtkSubPixelPositionEdgels&);  // Not implemented.
  void operator=(const vtkSubPixelPositionEdgels&);  // Not implemented.
};

#endif

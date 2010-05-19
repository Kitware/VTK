/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkShepardMethod.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkShepardMethod - sample unstructured points onto structured points using the method of Shepard
// .SECTION Description
// vtkShepardMethod is a filter used to visualize unstructured point data using
// Shepard's method. The method works by resampling the unstructured points 
// onto a structured points set. The influence functions are described as 
// "inverse distance weighted". Once the structured points are computed, the 
// usual visualization techniques (e.g., iso-contouring or volume rendering)
// can be used visualize the structured points.
// .SECTION Caveats
// The input to this filter is any dataset type. This filter can be used 
// to resample any form of data, i.e., the input data need not be 
// unstructured. 
//
// The bounds of the data (i.e., the sample space) is automatically computed
// if not set by the user.
//
// If you use a maximum distance less than 1.0, some output points may
// never receive a contribution. The final value of these points can be 
// specified with the "NullValue" instance variable.

#ifndef __vtkShepardMethod_h
#define __vtkShepardMethod_h

#include "vtkImageAlgorithm.h"

class VTK_IMAGING_EXPORT vtkShepardMethod : public vtkImageAlgorithm 
{
public:
  vtkTypeMacro(vtkShepardMethod,vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct with sample dimensions=(50,50,50) and so that model bounds are
  // automatically computed from input. Null value for each unvisited output 
  // point is 0.0. Maximum distance is 0.25.
  static vtkShepardMethod *New();
  
  // Description:
  // Compute ModelBounds from input geometry.
  double ComputeModelBounds(double origin[3], double ar[3]);

  // Description:
  // Specify i-j-k dimensions on which to sample input points.
  vtkGetVectorMacro(SampleDimensions,int,3);
  
  // Description:
  // Set the i-j-k dimensions on which to sample the distance function.
  void SetSampleDimensions(int i, int j, int k);

  // Description:
  // Set the i-j-k dimensions on which to sample the distance function.
  void SetSampleDimensions(int dim[3]);

  // Description:
  // Specify influence distance of each input point. This distance is a 
  // fraction of the length of the diagonal of the sample space. Thus, values 
  // of 1.0 will cause each input point to influence all points in the 
  // structured point dataset. Values less than 1.0 can improve performance
  // significantly.
  vtkSetClampMacro(MaximumDistance,double,0.0,1.0);
  vtkGetMacro(MaximumDistance,double);

  // Description:
  // Specify the position in space to perform the sampling.
  vtkSetVector6Macro(ModelBounds,double);
  vtkGetVectorMacro(ModelBounds,double,6);

  // Description:
  // Set the Null value for output points not receiving a contribution from the
  // input points.
  vtkSetMacro(NullValue,double);
  vtkGetMacro(NullValue,double);

protected:
  vtkShepardMethod();
  ~vtkShepardMethod() {};

  virtual int RequestInformation (vtkInformation *, 
                                  vtkInformationVector **, 
                                  vtkInformationVector *);

  // see vtkAlgorithm for details
  virtual int RequestData(vtkInformation *request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);

  // see algorithm for more info
  virtual int FillInputPortInformation(int port, vtkInformation* info);

  int SampleDimensions[3];
  double MaximumDistance;
  double ModelBounds[6];
  double NullValue;
private:
  vtkShepardMethod(const vtkShepardMethod&);  // Not implemented.
  void operator=(const vtkShepardMethod&);  // Not implemented.
};

#endif



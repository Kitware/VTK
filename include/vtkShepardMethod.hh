/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkShepardMethod.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkShepardMethod - sample unstructured points onto structured points using the method of Shepard
// .SECTION Description
// vtkShepardMethod is a filter used to visualize unstructured point data using
// Shepard's method. The method works by resampling the unstructured points 
// onto a structured points set. The influence functions are described as 
// "inverse distance weighted". Once the structured points are computed, the 
// usual visualization techniques can be used visualize the structured points.
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

#include "vtkDataSetToStructuredPointsFilter.hh"

class vtkShepardMethod : public vtkDataSetToStructuredPointsFilter 
{
public:
  vtkShepardMethod();
  char *GetClassName() {return "vtkShepardMethod";};
  void PrintSelf(ostream& os, vtkIndent indent);

  float ComputeModelBounds(float origin[3], float ar[3]);

  // Description:
  // Specify i-j-k dimensions on which to sample input points.
  vtkGetVectorMacro(SampleDimensions,int,3);

  void SetSampleDimensions(int i, int j, int k);
  void SetSampleDimensions(int dim[3]);

  // Description:
  // Specify influence distance of each input point. This distance is a 
  // fraction of the length of the diagonal of the sample space. Thus, values 
  // of 1.0 will cause each input point to influence all points in the 
  // structured point dataset. Values less than 1.0 can improve performance
  // significantly.
  vtkSetClampMacro(MaximumDistance,float,0.0,1.0);
  vtkGetMacro(MaximumDistance,float);

  // Description:
  // Specify the position in space to perform the sampling.
  vtkSetVectorMacro(ModelBounds,float,6);
  vtkGetVectorMacro(ModelBounds,float,6);
  void SetModelBounds(float xmin, float xmax, float ymin, float ymax, 
                      float zmin, float zmax);

  // Description:
  // Set the Null value for output points not receiving a contribution from the
  // input points.
  vtkSetMacro(NullValue,float);
  vtkGetMacro(NullValue,float);

protected:
  void Execute();

  int SampleDimensions[3];
  float MaximumDistance;
  float ModelBounds[6];
  float NullValue;
};

#endif



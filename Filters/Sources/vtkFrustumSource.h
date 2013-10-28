/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFrustumSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkFrustumSource - create a polygonal representation of a frustum
// .SECTION Description
// vtkFrustumSource creates a frustum defines by a set of planes. The frustum
// is represented with four-sided polygons. It is possible to specify extra
// lines to better visualize the field of view.
//
// .SECTION Usage
// Typical use consists of 3 steps:
// 1. get the planes coefficients from a vtkCamera with
// vtkCamera::GetFrustumPlanes()
// 2. initialize the planes with vtkPlanes::SetFrustumPlanes() with the planes
// coefficients
// 3. pass the vtkPlanes to a vtkFrustumSource.

#ifndef __vtkFrustumSource_h
#define __vtkFrustumSource_h

#include "vtkFiltersSourcesModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"
class vtkPlanes;

class VTKFILTERSSOURCES_EXPORT vtkFrustumSource : public vtkPolyDataAlgorithm
{
public:
  static vtkFrustumSource *New();
  vtkTypeMacro(vtkFrustumSource,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Return the 6 planes defining the frustum. Initial value is NULL.
  // The 6 planes are defined in this order: left,right,bottom,top,far,near.
  // If Planes==NULL or if Planes->GetNumberOfPlanes()!=6 when RequestData()
  // is called, an error message will be emitted and RequestData() will
  // return right away.
  vtkGetObjectMacro(Planes,vtkPlanes);

  // Description:
  // Set the 6 planes defining the frustum.
  virtual void SetPlanes(vtkPlanes *planes);

  // Description:
  // Tells if some extra lines will be generated. Initial value is true.
  vtkGetMacro(ShowLines,bool);
  vtkSetMacro(ShowLines,bool);
  vtkBooleanMacro(ShowLines,bool);

  // Description:
  // Length of the extra lines. This a stricly positive value.
  // Initial value is 1.0.
  vtkGetMacro(LinesLength,double);
  vtkSetMacro(LinesLength,double);

  // Description:
  // Modified GetMTime because of Planes.
  unsigned long GetMTime();

  // Description:
  // Set/get the desired precision for the output points.
  // vtkAlgorithm::SINGLE_PRECISION - Output single-precision floating point.
  // vtkAlgorithm::DOUBLE_PRECISION - Output double-precision floating point.
  vtkSetMacro(OutputPointsPrecision,int);
  vtkGetMacro(OutputPointsPrecision,int);

protected:
  // Description:
  // Default construtor. Planes=NULL. ShowLines=true. LinesLength=1.0.
  vtkFrustumSource();

  virtual ~vtkFrustumSource();

  virtual int RequestData(vtkInformation *request,
                          vtkInformationVector **inputVector,
                          vtkInformationVector *outputVector);

  // Description:
  // Compute the intersection of 3 planes.
  void ComputePoint(int planes[3],
                    double *pt);

  vtkPlanes *Planes;
  bool ShowLines;
  double LinesLength;
  int OutputPointsPrecision;

private:
  vtkFrustumSource(const vtkFrustumSource&);  // Not implemented.
  void operator=(const vtkFrustumSource&);  // Not implemented.
};

#endif

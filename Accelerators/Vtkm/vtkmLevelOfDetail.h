//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2012 Sandia Corporation.
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//=============================================================================

#ifndef vtkmLevelOfDetail_h
#define vtkmLevelOfDetail_h

#include "vtkPolyDataAlgorithm.h"
#include "vtkAcceleratorsVTKmModule.h" //required for correct implementation
#include "vtkmConfig.h" //required for general vtkm setup

class VTKACCELERATORSVTKM_EXPORT vtkmLevelOfDetail : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkmLevelOfDetail,vtkPolyDataAlgorithm)
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  static vtkmLevelOfDetail* New();

  // Description:
  // Set/Get the number of divisions along an individual axis for the spatial
  // bins.
  // The number of spatial bins is NumberOfXDivisions*NumberOfYDivisions*
  // NumberOfZDivisions.
  void SetNumberOfXDivisions(int num);
  void SetNumberOfYDivisions(int num);
  void SetNumberOfZDivisions(int num);
  int GetNumberOfXDivisions();
  int GetNumberOfYDivisions();
  int GetNumberOfZDivisions();

  // Description:
  // Set/Get the number of divisions for each axis for the spatial bins.
  // The number of spatial bins is NumberOfXDivisions*NumberOfYDivisions*
  // NumberOfZDivisions.
  void SetNumberOfDivisions(int div[3])
  {
    this->SetNumberOfDivisions(div[0], div[1], div[2]);
  }
  void SetNumberOfDivisions(int div0, int div1, int div2);

  const int* GetNumberOfDivisions();
  void GetNumberOfDivisions(int div[3]);

protected:
  vtkmLevelOfDetail();
  ~vtkmLevelOfDetail();

  virtual int RequestData(vtkInformation*, vtkInformationVector**,
                          vtkInformationVector*) VTK_OVERRIDE;

private:
  int NumberOfDivisions[3];

  vtkmLevelOfDetail(const vtkmLevelOfDetail&) VTK_DELETE_FUNCTION;
  void operator=(const vtkmLevelOfDetail&) VTK_DELETE_FUNCTION;
};

#endif // vtkmLevelOfDetail_h
// VTK-HeaderTest-Exclude: vtkmLevelOfDetail.h

// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkGeodesicPath.h"

#include "vtkExecutive.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkGeodesicPath::vtkGeodesicPath()
{
  this->SetNumberOfInputPorts(1);
}

//------------------------------------------------------------------------------
vtkGeodesicPath::~vtkGeodesicPath() = default;

//------------------------------------------------------------------------------
int vtkGeodesicPath::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
    return 1;
  }
  return 0;
}

//------------------------------------------------------------------------------
void vtkGeodesicPath::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END

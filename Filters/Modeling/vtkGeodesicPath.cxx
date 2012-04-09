/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeodesicPath.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGeodesicPath.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkExecutive.h"


//-----------------------------------------------------------------------------
vtkGeodesicPath::vtkGeodesicPath()
{
  this->SetNumberOfInputPorts(1);
}

//-----------------------------------------------------------------------------
vtkGeodesicPath::~vtkGeodesicPath()
{
}

//-----------------------------------------------------------------------------
int vtkGeodesicPath::FillInputPortInformation(int port,
                                              vtkInformation *info)
{
  if (port == 0)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
    return 1;
    }
  return 0;
}

//-----------------------------------------------------------------------------
void vtkGeodesicPath::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}


/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMapperNode.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMapperNode.h"

#include "vtkAbstractArray.h"
#include "vtkAbstractVolumeMapper.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkMapper.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolygon.h"
#include "vtkProperty.h"

//============================================================================
vtkStandardNewMacro(vtkMapperNode);

//----------------------------------------------------------------------------
vtkMapperNode::vtkMapperNode() {}

//----------------------------------------------------------------------------
vtkMapperNode::~vtkMapperNode() {}

//----------------------------------------------------------------------------
void vtkMapperNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkAbstractArray* vtkMapperNode::GetArrayToProcess(vtkDataSet* input, int& cellFlag)
{
  cellFlag = -1;
  vtkAbstractVolumeMapper* mapper = vtkAbstractVolumeMapper::SafeDownCast(this->GetRenderable());
  if (!mapper)
  {
    return nullptr;
  }

  vtkAbstractArray* scalars;
  int scalarMode = mapper->GetScalarMode();
  if (scalarMode == VTK_SCALAR_MODE_DEFAULT)
  {
    scalars = input->GetPointData()->GetScalars();
    cellFlag = 0;
    if (!scalars)
    {
      scalars = input->GetCellData()->GetScalars();
      cellFlag = 1;
    }
    return scalars;
  }

  if (scalarMode == VTK_SCALAR_MODE_USE_POINT_DATA)
  {
    scalars = input->GetPointData()->GetScalars();
    cellFlag = 0;
    return scalars;
  }
  if (scalarMode == VTK_SCALAR_MODE_USE_CELL_DATA)
  {
    scalars = input->GetCellData()->GetScalars();
    cellFlag = 1;
    return scalars;
  }

  int arrayAccessMode = mapper->GetArrayAccessMode();
  const char* arrayName = mapper->GetArrayName();
  int arrayId = mapper->GetArrayId();
  vtkPointData* pd;
  vtkCellData* cd;
  vtkFieldData* fd;
  if (scalarMode == VTK_SCALAR_MODE_USE_POINT_FIELD_DATA)
  {
    pd = input->GetPointData();
    if (arrayAccessMode == VTK_GET_ARRAY_BY_ID)
    {
      scalars = pd->GetAbstractArray(arrayId);
    }
    else
    {
      scalars = pd->GetAbstractArray(arrayName);
    }
    cellFlag = 0;
    return scalars;
  }

  if (scalarMode == VTK_SCALAR_MODE_USE_CELL_FIELD_DATA)
  {
    cd = input->GetCellData();
    if (arrayAccessMode == VTK_GET_ARRAY_BY_ID)
    {
      scalars = cd->GetAbstractArray(arrayId);
    }
    else
    {
      scalars = cd->GetAbstractArray(arrayName);
    }
    cellFlag = 1;
    return scalars;
  }

  if (scalarMode == VTK_SCALAR_MODE_USE_FIELD_DATA)
  {
    fd = input->GetFieldData();
    if (arrayAccessMode == VTK_GET_ARRAY_BY_ID)
    {
      scalars = fd->GetAbstractArray(arrayId);
    }
    else
    {
      scalars = fd->GetAbstractArray(arrayName);
    }
    cellFlag = 2;
    return scalars;
  }

  return nullptr;
}

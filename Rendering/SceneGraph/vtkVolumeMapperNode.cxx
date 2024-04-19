// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkVolumeMapperNode.h"

#include "vtkActor.h"
#include "vtkCellArray.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolygon.h"
#include "vtkProperty.h"

//============================================================================
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkVolumeMapperNode);

//------------------------------------------------------------------------------
vtkVolumeMapperNode::vtkVolumeMapperNode() = default;

//------------------------------------------------------------------------------
vtkVolumeMapperNode::~vtkVolumeMapperNode() = default;

//------------------------------------------------------------------------------
void vtkVolumeMapperNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END

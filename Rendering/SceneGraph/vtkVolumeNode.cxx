// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkVolumeNode.h"

#include "vtkAbstractVolumeMapper.h"
#include "vtkCellArray.h"
#include "vtkMapper.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolygon.h"
#include "vtkProperty.h"
#include "vtkVolume.h"

//============================================================================
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkVolumeNode);

//------------------------------------------------------------------------------
vtkVolumeNode::vtkVolumeNode() = default;

//------------------------------------------------------------------------------
vtkVolumeNode::~vtkVolumeNode() = default;

//------------------------------------------------------------------------------
void vtkVolumeNode::Build(bool prepass)
{
  if (prepass)
  {
    vtkVolume* mine = vtkVolume::SafeDownCast(this->GetRenderable());
    if (!mine)
    {
      return;
    }
    if (!mine->GetMapper())
    {
      return;
    }

    this->PrepareNodes();
    this->AddMissingNode(mine->GetMapper());
    this->RemoveUnusedNodes();
  }
}

//------------------------------------------------------------------------------
void vtkVolumeNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END

/*=========================================================================

   Program:   Visualization Toolkit
   Module:    vtkOSPRayMoleculeMapperNode.cxx

   Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
   All rights reserved.
   See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

      This software is distributed WITHOUT ANY WARRANTY; without even
      the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
      PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkOSPRayMoleculeMapperNode.h"

#include "vtkInformation.h"
#include "vtkMolecule.h"
#include "vtkMoleculeMapper.h"
#include "vtkOSPRayActorNode.h"
#include "vtkOSPRayCache.h"
#include "vtkOSPRayRendererNode.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkRenderer.h"

#include <algorithm>
#include <vector>

//============================================================================
vtkStandardNewMacro(vtkOSPRayMoleculeMapperNode);

//----------------------------------------------------------------------------
vtkOSPRayMoleculeMapperNode::vtkOSPRayMoleculeMapperNode()
{
  this->OSPMesh = nullptr;
}

//----------------------------------------------------------------------------
vtkOSPRayMoleculeMapperNode::~vtkOSPRayMoleculeMapperNode()
{
  vtkOSPRayRendererNode* orn = vtkOSPRayRendererNode::GetRendererNode(this);
  if (orn)
  {
    RTW::Backend* backend = orn->GetBackend();
    if (backend != nullptr)
    {
      ospRelease(this->OSPMesh);
    }
  }
  delete this->Cache;
}

//----------------------------------------------------------------------------
void vtkOSPRayMoleculeMapperNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkOSPRayMoleculeMapperNode::Render(bool prepass)
{
  if (!prepass)
  {
    return;
  }
  vtkOSPRayActorNode* aNode = vtkOSPRayActorNode::SafeDownCast(this->Parent);
  vtkActor* act = vtkActor::SafeDownCast(aNode->GetRenderable());
  if (act->GetVisibility() == false)
  {
    return;
  }

  vtkOSPRayRendererNode* orn =
    static_cast<vtkOSPRayRendererNode*>(this->GetFirstAncestorOfType("vtkOSPRayRendererNode"));
  RTW::Backend* backend = orn->GetBackend();
  if (backend == nullptr)
  {
    return;
  }

  auto oModel = orn->GetOModel();
  vtkMoleculeMapper* mapper = vtkMoleculeMapper::SafeDownCast(this->GetRenderable());
  vtkMolecule* molecule = mapper->GetInput();
  if (mapper->GetMTime() > this->BuildTime || molecule->GetMTime() > this->BuildTime)
  {
    ospRelease(this->OSPMesh);
    if (mapper->GetRenderAtoms())
    {
      vtkPoints* allPoints = molecule->GetAtomicPositionArray();
      osp::vec3f* vertices = new osp::vec3f[molecule->GetNumberOfAtoms()];
      for (size_t i = 0; i < molecule->GetNumberOfAtoms(); i++)
      {
        vertices[i] = osp::vec3f{ static_cast<float>(allPoints->GetPoint(i)[0]),
          static_cast<float>(allPoints->GetPoint(i)[1]),
          static_cast<float>(allPoints->GetPoint(i)[2]) };
      }
      OSPData position = ospNewData(molecule->GetNumberOfAtoms(), OSP_FLOAT3, &vertices[0]);
      ospCommit(position);
      this->OSPMesh = ospNewGeometry("spheres");
      ospSetObject(this->OSPMesh, "spheres", position);
      ospSet1i(this->OSPMesh, "bytes_per_sphere", 3 * sizeof(float));
      ospSet1i(this->OSPMesh, "offset_center", 0 * sizeof(float));
      ospSet1f(this->OSPMesh, "radius", 1.0);
      ospCommit(this->OSPMesh);
      ospRelease(position);
    }
    this->BuildTime.Modified();
  }

  ospAddGeometry(oModel, this->OSPMesh);
}

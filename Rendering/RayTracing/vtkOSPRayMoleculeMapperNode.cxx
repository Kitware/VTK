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
  cerr << "CREATE OSPMOLMAPPERNODE " << this << endl;
}

//----------------------------------------------------------------------------
vtkOSPRayMoleculeMapperNode::~vtkOSPRayMoleculeMapperNode()
{
  cerr << "DESTROY OSPMOLMAPPERNODE " << this << endl;

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
  if (prepass)
  {
    vtkOSPRayRendererNode* orn =
      static_cast<vtkOSPRayRendererNode*>(this->GetFirstAncestorOfType("vtkOSPRayRendererNode"));
    RTW::Backend* backend = orn->GetBackend();
    if (backend == nullptr)
    {
      return;
    }
  }
  else
  {
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
    if (mapper->GetRenderAtoms())
    {
      std::vector<double> _vertices;
      vtkPoints* allPoints = molecule->GetAtomicPositionArray();
      for (vtkIdType i = 0; i < molecule->GetNumberOfAtoms(); i++)
      {
        _vertices.push_back(allPoints->GetPoint(i)[0]);
        _vertices.push_back(allPoints->GetPoint(i)[1]);
        _vertices.push_back(allPoints->GetPoint(i)[2]);
      }
      osp::vec3f* vertices = new osp::vec3f[molecule->GetNumberOfAtoms()];
      for (size_t i = 0; i < molecule->GetNumberOfAtoms(); i++)
      {
        vertices[i] = osp::vec3f{ static_cast<float>(_vertices[i * 3 + 0]),
          static_cast<float>(_vertices[i * 3 + 1]), static_cast<float>(_vertices[i * 3 + 2]) };
      }
      OSPData position = ospNewData(molecule->GetNumberOfAtoms(), OSP_FLOAT3, &vertices[0]);
      ospCommit(position);
      _vertices.clear();
      OSPGeometry ospMesh = ospNewGeometry("spheres");
      ospSetObject(ospMesh, "spheres", position);
      ospSet1i(ospMesh, "bytes_per_sphere", 3 * sizeof(float));
      ospSet1i(ospMesh, "offset_center", 0 * sizeof(float));
      ospSet1f(ospMesh, "radius", 1.0);
      ospCommit(ospMesh);
      ospRelease(position);
      ospAddGeometry(oModel, ospMesh);
    }
    OSPModel OSPRayModel = orn->GetOModel();
  }
}

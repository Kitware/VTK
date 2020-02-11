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
#include "vtkLookupTable.h"
#include "vtkMolecule.h"
#include "vtkMoleculeMapper.h"
#include "vtkOSPRayActorNode.h"
#include "vtkOSPRayCache.h"
#include "vtkOSPRayRendererNode.h"
#include "vtkObjectFactory.h"
#include "vtkPeriodicTable.h"
#include "vtkPoints.h"
#include "vtkRenderer.h"
#include "vtkUnsignedShortArray.h"

#include <algorithm>
#include <vector>

//============================================================================
vtkStandardNewMacro(vtkOSPRayMoleculeMapperNode);

//----------------------------------------------------------------------------
vtkOSPRayMoleculeMapperNode::vtkOSPRayMoleculeMapperNode() {}

//----------------------------------------------------------------------------
vtkOSPRayMoleculeMapperNode::~vtkOSPRayMoleculeMapperNode()
{
  vtkOSPRayRendererNode* orn = vtkOSPRayRendererNode::GetRendererNode(this);
  if (orn)
  {
    RTW::Backend* backend = orn->GetBackend();
    if (backend != nullptr)
    {
      for (auto g : this->Geometries)
      {
        ospRelease(g);
      }
      this->Geometries.clear();
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
    for (auto g : this->Geometries)
    {
      ospRelease(g);
    }
    this->Geometries.clear();

    if (mapper->GetRenderAtoms())
    {
      OSPGeometry atoms;

      vtkPoints* allPoints = molecule->GetAtomicPositionArray();
      float* mdata = new float[molecule->GetNumberOfAtoms() * 5];
      int* idata = (int*)mdata;

      vtkUnsignedShortArray* atomicNumbers = molecule->GetAtomicNumberArray();
      vtkScalarsToColors* lut = mapper->GetLookupTable();
      float* _colors = new float[molecule->GetNumberOfAtoms() * 5];

      for (size_t i = 0; i < molecule->GetNumberOfAtoms(); i++)
      {
        mdata[i * 5 + 0] = static_cast<float>(allPoints->GetPoint(i)[0]);
        mdata[i * 5 + 1] = static_cast<float>(allPoints->GetPoint(i)[1]);
        mdata[i * 5 + 2] = static_cast<float>(allPoints->GetPoint(i)[2]);

        mdata[i * 5 + 3] = mapper->GetAtomicRadiusScaleFactor() *
          mapper->GetPeriodicTable()->GetVDWRadius(atomicNumbers->GetValue(i));

        idata[i * 5 + 4] = i; // index into colors array
        const unsigned char* ucolor = lut->MapValue(atomicNumbers->GetTuple1(i));
        _colors[i * 4 + 0] = static_cast<float>(ucolor[0] / 255.0);
        _colors[i * 4 + 1] = static_cast<float>(ucolor[1] / 255.0);
        _colors[i * 4 + 2] = static_cast<float>(ucolor[2] / 255.0);
        _colors[i * 4 + 3] = static_cast<float>(ucolor[3] / 255.0);
      }
      OSPData mesh = ospNewData(molecule->GetNumberOfAtoms() * 5, OSP_FLOAT, &mdata[0]);
      ospCommit(mesh);

      OSPData colors = ospNewData(molecule->GetNumberOfAtoms(), OSP_FLOAT4, &_colors[0]);
      ospCommit(colors);

      atoms = ospNewGeometry("spheres");
      ospSetObject(atoms, "spheres", mesh);
      ospSet1f(atoms, "radius", 1.0);
      ospSet1i(atoms, "bytes_per_sphere", 5 * sizeof(float));
      ospSet1i(atoms, "offset_center", 0 * sizeof(float));
      ospSet1i(atoms, "offset_radius", 3 * sizeof(float));
      ospSet1i(atoms, "offset_colorID", 4 * sizeof(float));
      ospSetData(atoms, "color", colors);

      this->Geometries.emplace_back(atoms);

      ospCommit(atoms);
      ospRelease(mesh);
      ospRelease(colors);

      delete[] mdata;
      delete[] _colors;
    }
    this->BuildTime.Modified();
  }

  for (auto g : this->Geometries)
  {
    ospAddGeometry(oModel, g);
  }
}

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
#include "vtkVector.h"
#include "vtkVectorOperators.h"

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
      const vtkIdType numAtoms = molecule->GetNumberOfAtoms();
      vtkPoints* allPoints = molecule->GetAtomicPositionArray();
      float* mdata = new float[numAtoms * 5];
      int* idata = (int*)mdata;

      vtkUnsignedShortArray* atomicNumbers = molecule->GetAtomicNumberArray();
      vtkScalarsToColors* lut = mapper->GetLookupTable();
      float* _colors = new float[numAtoms * 4];

      for (size_t i = 0; i < numAtoms; i++)
      {
        mdata[i * 5 + 0] = static_cast<float>(allPoints->GetPoint(i)[0]);
        mdata[i * 5 + 1] = static_cast<float>(allPoints->GetPoint(i)[1]);
        mdata[i * 5 + 2] = static_cast<float>(allPoints->GetPoint(i)[2]);

        mdata[i * 5 + 3] = mapper->GetAtomicRadiusScaleFactor() *
          mapper->GetPeriodicTable()->GetVDWRadius(atomicNumbers->GetValue(i));

        // todo: make a single color array for the vtkPeriodicTable and index into that instead of
        // one color per atom
        idata[i * 5 + 4] = i; // index into colors array
        const unsigned char* ucolor = lut->MapValue(atomicNumbers->GetTuple1(i));
        _colors[i * 4 + 0] = static_cast<float>(ucolor[0] / 255.0);
        _colors[i * 4 + 1] = static_cast<float>(ucolor[1] / 255.0);
        _colors[i * 4 + 2] = static_cast<float>(ucolor[2] / 255.0);
        _colors[i * 4 + 3] = static_cast<float>(ucolor[3] / 255.0);
      }
      OSPData mesh = ospNewData(numAtoms * 5, OSP_FLOAT, &mdata[0]);
      ospCommit(mesh);

      OSPData colors = ospNewData(numAtoms, OSP_FLOAT4, &_colors[0]);
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

    if (mapper->GetRenderBonds())
    {
      OSPGeometry bonds = ospNewGeometry("cylinders");

      vtkVector3f pos1, pos2;
      vtkIdType atomIds[2];
      vtkVector3f bondVec;
      float bondLength;
      float bondRadius = mapper->GetBondRadius();
      vtkVector3f bondCenter;

      const vtkIdType numBonds = molecule->GetNumberOfBonds();
      int width = 3 + 3 + 1 + 1; // Sxyx Exyz rad coloridx
      float* mdata = new float[2 * numBonds * width];
      int* idata = (int*)mdata;

      vtkUnsignedShortArray* atomicNumbers = molecule->GetAtomicNumberArray();
      vtkScalarsToColors* lut = mapper->GetLookupTable();
      float* _colors = new float[2 * numBonds * 4];

      for (vtkIdType bondInd = 0; bondInd < numBonds; ++bondInd)
      {
        vtkBond bond = molecule->GetBond(bondInd);
        pos1 = bond.GetBeginAtom().GetPosition();
        pos2 = bond.GetEndAtom().GetPosition();
        atomIds[0] = bond.GetBeginAtomId();
        atomIds[1] = bond.GetEndAtomId();

        // Compute additional bond info
        // - Normalized vector in direction of bond
        bondVec = pos2 - pos1;
        // - Center of bond for translation
        bondCenter[0] = (pos1[0] + pos2[0]) * 0.5;
        bondCenter[1] = (pos1[1] + pos2[1]) * 0.5;
        bondCenter[2] = (pos1[2] + pos2[2]) * 0.5;

        mdata[(bondInd * 2 + 0) * width + 0] = static_cast<float>(pos1.GetX());
        mdata[(bondInd * 2 + 0) * width + 1] = static_cast<float>(pos1.GetY());
        mdata[(bondInd * 2 + 0) * width + 2] = static_cast<float>(pos1.GetZ());
        mdata[(bondInd * 2 + 0) * width + 3] = static_cast<float>(bondCenter.GetX());
        mdata[(bondInd * 2 + 0) * width + 4] = static_cast<float>(bondCenter.GetY());
        mdata[(bondInd * 2 + 0) * width + 5] = static_cast<float>(bondCenter.GetZ());
        mdata[(bondInd * 2 + 0) * width + 6] = bondRadius;
        // todo: make a single color array for the vtkPeriodicTable and index into that instead of
        // one color per atom
        idata[(bondInd * 2 + 0) * width + 7] = bondInd * 2 + 0; // index into colors array
        const unsigned char* ucolor = lut->MapValue(atomicNumbers->GetTuple1(atomIds[0]));
        _colors[(bondInd * 2 + 0) * 4 + 0] = static_cast<float>(ucolor[0] / 255.0);
        _colors[(bondInd * 2 + 0) * 4 + 1] = static_cast<float>(ucolor[1] / 255.0);
        _colors[(bondInd * 2 + 0) * 4 + 2] = static_cast<float>(ucolor[2] / 255.0);
        _colors[(bondInd * 2 + 0) * 4 + 3] = static_cast<float>(ucolor[3] / 255.0);

        mdata[(bondInd * 2 + 1) * width + 0] = static_cast<float>(bondCenter.GetX());
        mdata[(bondInd * 2 + 1) * width + 1] = static_cast<float>(bondCenter.GetY());
        mdata[(bondInd * 2 + 1) * width + 2] = static_cast<float>(bondCenter.GetZ());
        mdata[(bondInd * 2 + 1) * width + 3] = static_cast<float>(pos2.GetX());
        mdata[(bondInd * 2 + 1) * width + 4] = static_cast<float>(pos2.GetY());
        mdata[(bondInd * 2 + 1) * width + 5] = static_cast<float>(pos2.GetZ());
        mdata[(bondInd * 2 + 1) * width + 6] = bondRadius;
        idata[(bondInd * 2 + 1) * width + 7] = bondInd * 2 + 1; // index into colors array
        ucolor = lut->MapValue(atomicNumbers->GetTuple1(atomIds[1]));
        _colors[(bondInd * 2 + 1) * 4 + 0] = static_cast<float>(ucolor[0] / 255.0);
        _colors[(bondInd * 2 + 1) * 4 + 1] = static_cast<float>(ucolor[1] / 255.0);
        _colors[(bondInd * 2 + 1) * 4 + 2] = static_cast<float>(ucolor[2] / 255.0);
        _colors[(bondInd * 2 + 1) * 4 + 3] = static_cast<float>(ucolor[3] / 255.0);
      }

      OSPData _mdata = ospNewData(2 * numBonds * width, OSP_FLOAT, mdata);
      ospSet1i(bonds, "bytes_per_cylinder", width * sizeof(float));
      ospSet1i(bonds, "offset_v0", 0);
      ospSet1i(bonds, "offset_v1", 3 * sizeof(float));
      ospSet1i(bonds, "offset_radius", 6 * sizeof(float));
      ospSet1i(bonds, "offset_colorID", 7 * sizeof(float));
      ospCommit(_mdata);

      OSPData colors = ospNewData(2 * numBonds, OSP_FLOAT4, &_colors[0]);
      ospCommit(colors);

      ospSetData(bonds, "cylinders", _mdata);
      ospSetData(bonds, "color", colors);

      ospCommit(bonds);

      this->Geometries.emplace_back(bonds);

      ospRelease(_mdata);
      ospRelease(colors);
    }

    if (mapper->GetRenderLattice())
    {
      // OSPGeometry lattice;
      // todo: take a half hour and translate vtkMoleculeMapper::UpdateLatticePolyData() to ospray
      // API this->Geometries.emplace_back(lattice);
    }

    this->BuildTime.Modified();
  }

  for (auto g : this->Geometries)
  {
    ospAddGeometry(oModel, g);
  }
}

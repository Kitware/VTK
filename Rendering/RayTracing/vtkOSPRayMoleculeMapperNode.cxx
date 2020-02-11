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
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkUnsignedShortArray.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"

#include <algorithm>
#include <string.h>
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
  vtkProperty* property = act->GetProperty(); // for opacity, linewidth
  double opacity = property->GetOpacity();

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
  if (mapper->GetMTime() > this->BuildTime || property->GetMTime() > this->BuildTime ||
    act->GetMTime() > this->BuildTime || molecule->GetMTime() > this->BuildTime)
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
        _colors[i * 4 + 3] = static_cast<float>(ucolor[3] / 255.0) * opacity;
      }
      OSPData mesh = ospNewData(numAtoms * 5, OSP_FLOAT, &mdata[0]);
      ospCommit(mesh);

      OSPData colors = ospNewData(numAtoms, OSP_FLOAT4, &_colors[0]);
      ospCommit(colors);

      atoms = ospNewGeometry("spheres");
      ospSetObject(atoms, "spheres", mesh);
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
      float ocolor[4];
      unsigned char* vcolor = mapper->GetBondColor();
      ocolor[0] = vcolor[0] / 255.0;
      ocolor[1] = vcolor[1] / 255.0;
      ocolor[2] = vcolor[2] / 255.0;
      ocolor[3] = opacity;

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
        if (mapper->GetBondColorMode() == vtkMoleculeMapper::DiscreteByAtom)
        {
          const unsigned char* ucolor = lut->MapValue(atomicNumbers->GetTuple1(atomIds[0]));
          _colors[(bondInd * 2 + 0) * 4 + 0] = static_cast<float>(ucolor[0] / 255.0);
          _colors[(bondInd * 2 + 0) * 4 + 1] = static_cast<float>(ucolor[1] / 255.0);
          _colors[(bondInd * 2 + 0) * 4 + 2] = static_cast<float>(ucolor[2] / 255.0);
          _colors[(bondInd * 2 + 0) * 4 + 3] = static_cast<float>(ucolor[3] / 255.0) * opacity;
        }
        else
        {
          _colors[(bondInd * 2 + 0) * 4 + 0] = ocolor[0];
          _colors[(bondInd * 2 + 0) * 4 + 1] = ocolor[1];
          _colors[(bondInd * 2 + 0) * 4 + 2] = ocolor[2];
          _colors[(bondInd * 2 + 0) * 4 + 3] = ocolor[3];
        }

        mdata[(bondInd * 2 + 1) * width + 0] = static_cast<float>(bondCenter.GetX());
        mdata[(bondInd * 2 + 1) * width + 1] = static_cast<float>(bondCenter.GetY());
        mdata[(bondInd * 2 + 1) * width + 2] = static_cast<float>(bondCenter.GetZ());
        mdata[(bondInd * 2 + 1) * width + 3] = static_cast<float>(pos2.GetX());
        mdata[(bondInd * 2 + 1) * width + 4] = static_cast<float>(pos2.GetY());
        mdata[(bondInd * 2 + 1) * width + 5] = static_cast<float>(pos2.GetZ());
        mdata[(bondInd * 2 + 1) * width + 6] = bondRadius;
        idata[(bondInd * 2 + 1) * width + 7] = bondInd * 2 + 1; // index into colors array
        if (mapper->GetBondColorMode() == vtkMoleculeMapper::DiscreteByAtom)
        {
          const unsigned char* ucolor = lut->MapValue(atomicNumbers->GetTuple1(atomIds[1]));
          _colors[(bondInd * 2 + 1) * 4 + 0] = static_cast<float>(ucolor[0] / 255.0);
          _colors[(bondInd * 2 + 1) * 4 + 1] = static_cast<float>(ucolor[1] / 255.0);
          _colors[(bondInd * 2 + 1) * 4 + 2] = static_cast<float>(ucolor[2] / 255.0);
          _colors[(bondInd * 2 + 1) * 4 + 3] = static_cast<float>(ucolor[3] / 255.0) * opacity;
        }
        else
        {
          _colors[(bondInd * 2 + 1) * 4 + 0] = ocolor[0];
          _colors[(bondInd * 2 + 1) * 4 + 1] = ocolor[1];
          _colors[(bondInd * 2 + 1) * 4 + 2] = ocolor[2];
          _colors[(bondInd * 2 + 1) * 4 + 3] = ocolor[3];
        }
      }

      OSPData _mdata = ospNewData(2 * numBonds * width, OSP_FLOAT, mdata);
      ospSet1i(bonds, "bytes_per_cylinder", width * sizeof(float));
      ospSet1i(bonds, "offset_v0", 0);
      ospSet1i(bonds, "offset_v1", 3 * sizeof(float));
      ospSet1i(bonds, "offset_radius", 6 * sizeof(float));
      ospSetData(bonds, "cylinders", _mdata);

      OSPData colors = ospNewData(2 * numBonds, OSP_FLOAT4, &_colors[0]);
      ospSet1i(bonds, "offset_colorID", 7 * sizeof(float));
      ospCommit(colors);
      ospSetData(bonds, "color", colors);
      ospCommit(_mdata);

      ospCommit(bonds);

      this->Geometries.emplace_back(bonds);

      ospRelease(_mdata);
      ospRelease(colors);
    }

    if (mapper->GetRenderLattice() && molecule->HasLattice())
    {
      OSPGeometry lattice = ospNewGeometry("cylinders");

      vtkVector3d a;
      vtkVector3d b;
      vtkVector3d c;
      vtkVector3d origin;

      molecule->GetLattice(a, b, c, origin);
      float tdata[8 * 3];
      for (int i = 0; i < 8; ++i)
      {
        vtkVector3d corner;
        switch (i)
        {
          case 0:
            corner = origin;
            break;
          case 1:
            corner = origin + a;
            break;
          case 2:
            corner = origin + b;
            break;
          case 3:
            corner = origin + c;
            break;
          case 4:
            corner = origin + a + b;
            break;
          case 5:
            corner = origin + a + c;
            break;
          case 6:
            corner = origin + b + c;
            break;
          case 7:
            corner = origin + a + b + c;
            break;
        }
        tdata[i * 3 + 0] = corner.GetData()[0];
        tdata[i * 3 + 1] = corner.GetData()[1];
        tdata[i * 3 + 2] = corner.GetData()[2];
      }

      float* mdata = new float[12 * 2 * 3];
      memcpy(&mdata[0 * 3], &tdata[0 * 3], 3 * sizeof(float));
      memcpy(&mdata[1 * 3], &tdata[1 * 3], 3 * sizeof(float));
      memcpy(&mdata[2 * 3], &tdata[1 * 3], 3 * sizeof(float));
      memcpy(&mdata[3 * 3], &tdata[4 * 3], 3 * sizeof(float));
      memcpy(&mdata[4 * 3], &tdata[4 * 3], 3 * sizeof(float));
      memcpy(&mdata[5 * 3], &tdata[2 * 3], 3 * sizeof(float));
      memcpy(&mdata[6 * 3], &tdata[2 * 3], 3 * sizeof(float));
      memcpy(&mdata[7 * 3], &tdata[0 * 3], 3 * sizeof(float));
      memcpy(&mdata[8 * 3], &tdata[0 * 3], 3 * sizeof(float));
      memcpy(&mdata[9 * 3], &tdata[3 * 3], 3 * sizeof(float));
      memcpy(&mdata[10 * 3], &tdata[2 * 3], 3 * sizeof(float));
      memcpy(&mdata[11 * 3], &tdata[6 * 3], 3 * sizeof(float));
      memcpy(&mdata[12 * 3], &tdata[4 * 3], 3 * sizeof(float));
      memcpy(&mdata[13 * 3], &tdata[7 * 3], 3 * sizeof(float));
      memcpy(&mdata[14 * 3], &tdata[1 * 3], 3 * sizeof(float));
      memcpy(&mdata[15 * 3], &tdata[5 * 3], 3 * sizeof(float));
      memcpy(&mdata[16 * 3], &tdata[6 * 3], 3 * sizeof(float));
      memcpy(&mdata[17 * 3], &tdata[3 * 3], 3 * sizeof(float));
      memcpy(&mdata[18 * 3], &tdata[5 * 3], 3 * sizeof(float));
      memcpy(&mdata[19 * 3], &tdata[3 * 3], 3 * sizeof(float));
      memcpy(&mdata[20 * 3], &tdata[5 * 3], 3 * sizeof(float));
      memcpy(&mdata[21 * 3], &tdata[7 * 3], 3 * sizeof(float));
      memcpy(&mdata[22 * 3], &tdata[6 * 3], 3 * sizeof(float));
      memcpy(&mdata[23 * 3], &tdata[7 * 3], 3 * sizeof(float));

      OSPData _mdata = ospNewData(12 * 2 * 3, OSP_FLOAT, mdata);
      ospSet1i(lattice, "bytes_per_cylinder", 6 * sizeof(float));
      ospSet1i(lattice, "offset_v0", 0);
      ospSet1i(lattice, "offset_v1", 3 * sizeof(float));

      double length = length = mapper->GetLength();
      double lineWidth = length / 1000.0 * property->GetLineWidth();
      ospSet1f(lattice, "radius", lineWidth);

      float ocolor[4];
      unsigned char* vcolor = mapper->GetLatticeColor();
      ocolor[0] = vcolor[0] / 255.0;
      ocolor[1] = vcolor[1] / 255.0;
      ocolor[2] = vcolor[2] / 255.0;
      ocolor[3] = opacity;
      ospSet3fv(lattice, "color", ocolor);
      ospCommit(_mdata);

      ospSetData(lattice, "cylinders", _mdata);
      ospCommit(lattice);

      this->Geometries.emplace_back(lattice);

      ospRelease(_mdata);
    }

    this->BuildTime.Modified();
  }

  for (auto g : this->Geometries)
  {
    ospAddGeometry(oModel, g);
  }
}

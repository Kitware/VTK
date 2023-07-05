// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOSPRayMoleculeMapperNode.h"

#include "vtkDataSetAttributes.h"
#include "vtkInformation.h"
#include "vtkLookupTable.h"
#include "vtkMolecule.h"
#include "vtkMoleculeMapper.h"
#include "vtkOSPRayActorNode.h"
#include "vtkOSPRayMaterialHelpers.h"
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
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkOSPRayMoleculeMapperNode);

//------------------------------------------------------------------------------
vtkOSPRayMoleculeMapperNode::vtkOSPRayMoleculeMapperNode() = default;

//------------------------------------------------------------------------------
vtkOSPRayMoleculeMapperNode::~vtkOSPRayMoleculeMapperNode() {}

//------------------------------------------------------------------------------
void vtkOSPRayMoleculeMapperNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
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

  vtkProperty* property = act->GetProperty();
  vtkMoleculeMapper* mapper = vtkMoleculeMapper::SafeDownCast(this->GetRenderable());
  vtkMolecule* molecule = mapper->GetInput();
  vtkScalarsToColors* lut = mapper->GetLookupTable();

  if (act->GetMTime() > this->BuildTime || property->GetMTime() > this->BuildTime ||
    mapper->GetMTime() > this->BuildTime || lut->GetMTime() > this->BuildTime ||
    molecule->GetMTime() > this->BuildTime)
  {
    // free up whatever we did last time
    for (auto instance : this->Instances)
    {
      ospRelease(&(*instance));
    }
    this->Instances.clear();

    // some state that affect everything we draw
    double opacity = property->GetOpacity();
    float specPower = static_cast<float>(property->GetSpecularPower());
    float specAdjust = 2.0f / (2.0f + specPower);
    float specularf[3];
    specularf[0] =
      static_cast<float>(property->GetSpecularColor()[0] * property->GetSpecular() * specAdjust);
    specularf[1] =
      static_cast<float>(property->GetSpecularColor()[1] * property->GetSpecular() * specAdjust);
    specularf[2] =
      static_cast<float>(property->GetSpecularColor()[2] * property->GetSpecular() * specAdjust);
    // setup color/appearance of each element that we may draw
    std::vector<OSPMaterial> _elementMaterials;
    for (int i = 0; i < mapper->GetPeriodicTable()->GetNumberOfElements(); i++)
    {
      // todo: I'm no physicist, but I don't think the periodic table changes often. We should
      // rethink when we construct this. todo: unify with
      // vtkOSPRayPolyDataMapperNode::CellMaterials() and lookup custom materials from discrete LUT.
      // That way we can have glowing Thorium with matte Calcium and reflective Aluminum.
      const unsigned char* ucolor = lut->MapValue(static_cast<unsigned short>(i));
      auto oMaterial = vtkOSPRayMaterialHelpers::NewMaterial(orn, orn->GetORenderer(), "obj");
      float diffusef[] = { static_cast<float>(ucolor[0]) / (255.0f),
        static_cast<float>(ucolor[1]) / (255.0f), static_cast<float>(ucolor[2]) / (255.0f) };
      ospSetVec3f(oMaterial, "kd", diffusef[0], diffusef[1], diffusef[2]);
      ospSetVec3f(oMaterial, "ks", specularf[0], specularf[1], specularf[2]);
      ospSetFloat(oMaterial, "ns", specPower);
      ospSetFloat(oMaterial, "d", opacity);
      ospCommit(oMaterial);
      _elementMaterials.push_back(oMaterial);
    }
    auto elementMaterials =
      ospNewCopyData1D(&_elementMaterials[0], OSP_OBJECT, _elementMaterials.size());
    ospCommit(elementMaterials);

    // now translate the three things we may actually draw into OSPRay calls
    const vtkIdType numAtoms = molecule->GetNumberOfAtoms();
    if (mapper->GetRenderAtoms() && numAtoms)
    {
      OSPGeometry atoms = ospNewGeometry("sphere");
      OSPGeometricModel atomsModel = ospNewGeometricModel(atoms);
      ospRelease(atoms);

      vtkPoints* allPoints = molecule->GetAtomicPositionArray();
      std::vector<osp::vec3f> vertices;
      std::vector<float> radii;
      std::vector<OSPMaterial> materials;
      vtkUnsignedShortArray* atomicNumbers = molecule->GetAtomicNumberArray();

      for (vtkIdType i = 0; i < numAtoms; i++)
      {
        vertices.emplace_back(osp::vec3f{ static_cast<float>(allPoints->GetPoint(i)[0]),
          static_cast<float>(allPoints->GetPoint(i)[1]),
          static_cast<float>(allPoints->GetPoint(i)[2]) });

        materials.emplace_back(_elementMaterials[atomicNumbers->GetValue(i)]);
      }

      switch (mapper->GetAtomicRadiusType())
      {
        default:
          vtkWarningMacro(<< "Unknown radius type: " << mapper->GetAtomicRadiusType()
                          << ". Falling back to 'VDWRadius'.");
          VTK_FALLTHROUGH;
        case vtkMoleculeMapper::VDWRadius:
          for (vtkIdType i = 0; i < numAtoms; i++)
          {
            radii.emplace_back(mapper->GetAtomicRadiusScaleFactor() *
              mapper->GetPeriodicTable()->GetVDWRadius(atomicNumbers->GetValue(i)));
          }
          break;
        case vtkMoleculeMapper::CovalentRadius:
          for (vtkIdType i = 0; i < numAtoms; i++)
          {
            radii.emplace_back(mapper->GetAtomicRadiusScaleFactor() *
              mapper->GetPeriodicTable()->GetCovalentRadius(atomicNumbers->GetValue(i)));
          }
          break;
        case vtkMoleculeMapper::UnitRadius:
          for (vtkIdType i = 0; i < numAtoms; i++)
          {
            radii.emplace_back(mapper->GetAtomicRadiusScaleFactor());
          }
          break;
        case vtkMoleculeMapper::CustomArrayRadius:
          vtkDataArray* allRadii =
            molecule->GetVertexData()->GetArray(mapper->GetAtomicRadiusArrayName());
          if (!allRadii)
          {
            vtkWarningMacro("AtomicRadiusType set to CustomArrayRadius, but no array named "
              << mapper->GetAtomicRadiusArrayName() << " found in input VertexData.");
            for (vtkIdType i = 0; i < numAtoms; i++)
            {
              radii.emplace_back(mapper->GetAtomicRadiusScaleFactor());
            }
            break;
          }
          for (vtkIdType i = 0; i < numAtoms; i++)
          {
            radii.emplace_back(allRadii->GetTuple1(i));
          }
          break;
      }
      OSPData vertData = ospNewCopyData1D(vertices.data(), OSP_VEC3F, numAtoms);
      ospCommit(vertData);
      OSPData radiiData = ospNewCopyData1D(radii.data(), OSP_FLOAT, numAtoms);
      ospCommit(radiiData);
      OSPData materialsData = ospNewCopyData1D(materials.data(), OSP_MATERIAL, numAtoms);
      ospCommit(materialsData);

      ospSetObject(atoms, "sphere.position", vertData);
      ospSetObject(atoms, "sphere.radius", radiiData);
      ospSetObject(atomsModel, "material", materialsData);

      this->GeometricModels.emplace_back(atomsModel);
      ospCommit(atoms);
      ospCommit(atomsModel);
      ospRelease(vertData);
      ospRelease(radiiData);
      ospRelease(materialsData);
    }

    const vtkIdType numBonds = molecule->GetNumberOfBonds();
    if (mapper->GetRenderBonds() && numBonds)
    {
      OSPGeometry bonds = ospNewGeometry("curve");
      OSPGeometricModel bondsModel = ospNewGeometricModel(bonds);
      ospRelease(bonds);

      vtkVector3f pos1, pos2;
      vtkIdType atomIds[2];
      vtkVector3f bondVec;
      float bondRadius = mapper->GetBondRadius();
      vtkVector3f bondCenter;

      std::vector<osp::vec4f> vertsAndRadii;
      std::vector<OSPMaterial> materials;
      std::vector<unsigned int> indices;

      vtkUnsignedShortArray* atomicNumbers = molecule->GetAtomicNumberArray();
      for (vtkIdType bondInd = 0; bondInd < numBonds; ++bondInd)
      {
        // each endpoint is doubled because we need to use OSP_BEZIER to vary width
        indices.push_back(bondInd * 8 + 0);
        indices.push_back(bondInd * 8 + 4);

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

        auto start = osp::vec4f{ static_cast<float>(pos1.GetX()), static_cast<float>(pos1.GetY()),
          static_cast<float>(pos1.GetZ()), bondRadius };
        auto mid =
          osp::vec4f{ static_cast<float>(bondCenter.GetX()), static_cast<float>(bondCenter.GetY()),
            static_cast<float>(bondCenter.GetZ()), bondRadius };
        auto end = osp::vec4f{ static_cast<float>(pos2.GetX()), static_cast<float>(pos2.GetY()),
          static_cast<float>(pos2.GetZ()), bondRadius };

        // tube from atom1 to midpoint
        materials.emplace_back(_elementMaterials[atomicNumbers->GetValue(atomIds[0])]);
        vertsAndRadii.emplace_back(start);
        vertsAndRadii.emplace_back(start);
        vertsAndRadii.emplace_back(mid);
        vertsAndRadii.emplace_back(mid);

        // tube from midpoint to atom2
        materials.emplace_back(_elementMaterials[atomicNumbers->GetValue(atomIds[1])]);
        vertsAndRadii.emplace_back(mid);
        vertsAndRadii.emplace_back(mid);
        vertsAndRadii.emplace_back(end);
        vertsAndRadii.emplace_back(end);
      }

      OSPData vertsAndRadiiData =
        ospNewCopyData1D(vertsAndRadii.data(), OSP_VEC4F, vertsAndRadii.size());
      ospCommit(vertsAndRadiiData);
      ospSetObject(bonds, "vertex.position_radius", vertsAndRadiiData);
      ospRelease(vertsAndRadiiData);

      OSPData indicesData = ospNewCopyData1D(indices.data(), OSP_UINT, indices.size());
      ospCommit(indicesData);
      ospSetObject(bonds, "index", indicesData);
      ospRelease(indicesData);

      if (mapper->GetBondColorMode() == vtkMoleculeMapper::DiscreteByAtom)
      {
        OSPData materialData = ospNewCopyData1D(materials.data(), OSP_MATERIAL, materials.size());
        ospCommit(materialData);
        ospSetObject(bondsModel, "material", materialData);
        ospRelease(materialData);
      }
      else
      {
        auto oMaterial = vtkOSPRayMaterialHelpers::NewMaterial(orn, orn->GetORenderer(), "obj");
        unsigned char* vcolor = mapper->GetBondColor();
        float diffusef[] = { static_cast<float>(vcolor[0]) / (255.0f),
          static_cast<float>(vcolor[1]) / (255.0f), static_cast<float>(vcolor[2]) / (255.0f) };
        ospSetVec3f(oMaterial, "kd", diffusef[0], diffusef[1], diffusef[2]);
        ospSetVec3f(oMaterial, "ks", specularf[0], specularf[1], specularf[2]);
        ospSetInt(oMaterial, "ns", specPower);
        ospSetInt(oMaterial, "d", opacity);
        ospCommit(oMaterial);
        ospSetObjectAsData(bondsModel, "material", OSP_MATERIAL, oMaterial);
        ospRelease(oMaterial);
      }

      ospSetInt(bonds, "type", OSP_ROUND);
      ospSetInt(bonds, "basis", OSP_BEZIER);

      this->GeometricModels.emplace_back(bondsModel);
      ospCommit(bonds);
      ospCommit(bondsModel);
    }

    if (mapper->GetRenderLattice() && molecule->HasLattice())
    {
      OSPGeometry lattice = ospNewGeometry("curve");
      OSPGeometricModel latticeModel = ospNewGeometricModel(lattice);
      ospRelease(lattice);

      vtkVector3d a;
      vtkVector3d b;
      vtkVector3d c;
      vtkVector3d origin;

      molecule->GetLattice(a, b, c, origin);
      std::vector<osp::vec3f> vertices;
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
        vertices.emplace_back(osp::vec3f{ static_cast<float>(corner.GetData()[0]),
          static_cast<float>(corner.GetData()[1]), static_cast<float>(corner.GetData()[2]) });
      }
      OSPData verticesData = ospNewCopyData1D(vertices.data(), OSP_VEC3F, vertices.size());
      ospCommit(verticesData);
      ospSetObject(lattice, "vertex.position", verticesData);
      ospRelease(verticesData);

      std::vector<unsigned int> indices;
      indices.emplace_back(0);
      indices.emplace_back(1);
      indices.emplace_back(1);
      indices.emplace_back(4);
      indices.emplace_back(4);
      indices.emplace_back(2);
      indices.emplace_back(2);
      indices.emplace_back(0);
      indices.emplace_back(0);
      indices.emplace_back(3);
      indices.emplace_back(2);
      indices.emplace_back(6);
      indices.emplace_back(4);
      indices.emplace_back(7);
      indices.emplace_back(1);
      indices.emplace_back(5);
      indices.emplace_back(6);
      indices.emplace_back(5);
      indices.emplace_back(3);
      indices.emplace_back(5);
      indices.emplace_back(3);
      indices.emplace_back(5);
      indices.emplace_back(7);
      indices.emplace_back(6);
      indices.emplace_back(7);
      OSPData indicesData = ospNewCopyData1D(indices.data(), OSP_UINT, indices.size());
      ospCommit(indicesData);
      ospSetObject(lattice, "index", indicesData);
      ospRelease(indicesData);

      double length = mapper->GetLength();
      double lineWidth = length / 1000.0 * property->GetLineWidth();
      ospSetFloat(lattice, "radius", lineWidth);

      float ocolor[4];
      unsigned char* vcolor = mapper->GetLatticeColor();
      ocolor[0] = vcolor[0] / 255.0;
      ocolor[1] = vcolor[1] / 255.0;
      ocolor[2] = vcolor[2] / 255.0;
      ocolor[3] = opacity;
      ospSetVec3f(latticeModel, "color", ocolor[0], ocolor[1], ocolor[2]);

      ospSetInt(lattice, "type", OSP_ROUND);
      ospSetInt(lattice, "basis", OSP_LINEAR);

      this->GeometricModels.emplace_back(latticeModel);
      ospCommit(lattice);
      ospCommit(latticeModel);
    }

    this->BuildTime.Modified();
  }

  for (auto g : this->GeometricModels)
  {
    OSPGroup group = ospNewGroup();
    OSPInstance instance = ospNewInstance(group);
    ospCommit(instance);
    ospRelease(group);
    OSPData data = ospNewCopyData1D(&g, OSP_GEOMETRIC_MODEL, 1);
    ospRelease(&(*g));
    ospCommit(data);
    ospSetObject(group, "geometry", data);
    ospCommit(group);
    ospRelease(data);
    this->Instances.emplace_back(instance);
  }
  this->GeometricModels.clear();

  for (auto instance : this->Instances)
  {
    orn->Instances.emplace_back(instance);
  }
}
VTK_ABI_NAMESPACE_END

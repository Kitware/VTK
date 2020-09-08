/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOSPRayTetrahedraMapperNode.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOSPRayTetrahedraMapperNode.h"

#include "vtkCell.h"
#include "vtkColorTransferFunction.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkOSPRayCache.h"
#include "vtkOSPRayRendererNode.h"
#include "vtkObjectFactory.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPointData.h"
#include "vtkRenderer.h"
#include "vtkUnstructuredGridVolumeMapper.h"
#include "vtkVolume.h"
#include "vtkVolumeNode.h"
#include "vtkVolumeProperty.h"

#include <algorithm>

#include "RTWrapper/RTWrapper.h"

#include <cassert>

vtkStandardNewMacro(vtkOSPRayTetrahedraMapperNode);

//------------------------------------------------------------------------------
vtkOSPRayTetrahedraMapperNode::vtkOSPRayTetrahedraMapperNode()
{
  this->SamplingRate = 0.0f;
  this->NumColors = 128;
  this->OSPRayVolume = nullptr;
  this->OSPRayVolumeModel = nullptr;
  this->TransferFunction = nullptr;
}

vtkOSPRayTetrahedraMapperNode::~vtkOSPRayTetrahedraMapperNode()
{
  vtkOSPRayRendererNode* orn = vtkOSPRayRendererNode::GetRendererNode(this);
  if (orn)
  {
    RTW::Backend* backend = orn->GetBackend();
    if (backend != nullptr)
    {
      ospRelease(this->TransferFunction);
    }
  }
}

void vtkOSPRayTetrahedraMapperNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkOSPRayTetrahedraMapperNode::Render(bool prepass)
{
  if (prepass)
  {
    vtkUnstructuredGridVolumeMapper* mapper =
      vtkUnstructuredGridVolumeMapper::SafeDownCast(this->GetRenderable());
    if (!mapper)
    {
      vtkErrorMacro("invalid mapper");
      return;
    }

    vtkVolumeNode* volNode = vtkVolumeNode::SafeDownCast(this->Parent);
    if (!volNode)
    {
      vtkErrorMacro("invalid volumeNode");
      return;
    }

    vtkVolume* vol = vtkVolume::SafeDownCast(volNode->GetRenderable());
    if (vol->GetVisibility() == false)
    {
      return;
    }

    if (!vol->GetProperty())
    {
      // this is OK, happens in paraview client side for instance
      return;
    }

    mapper->GetInputAlgorithm()->UpdateInformation();
    mapper->GetInputAlgorithm()->Update();

    vtkOSPRayRendererNode* orn =
      static_cast<vtkOSPRayRendererNode*>(this->GetFirstAncestorOfType("vtkOSPRayRendererNode"));
    RTW::Backend* backend = orn->GetBackend();
    if (backend == nullptr)
      return;
    vtkRenderer* ren = vtkRenderer::SafeDownCast(orn->GetRenderable());

    vtkDataSet* dataSet = mapper->GetDataSetInput();
    if (!dataSet)
    {
      return;
    }
    int fieldAssociation;
    vtkDataArray* array =
      vtkDataArray::SafeDownCast(this->GetArrayToProcess(dataSet, fieldAssociation));
    if (!array)
    {
      // ok can happen in paraview client server mode for example
      return;
    }

    int numberOfCells = dataSet->GetNumberOfCells();
    int numberOfPoints = dataSet->GetNumberOfPoints();

    if (!this->TransferFunction)
    {
      this->TransferFunction = ospNewTransferFunction("piecewiseLinear");
    }

    // when input data is modified
    if (mapper->GetDataSetInput()->GetMTime() > this->BuildTime)
    {
      this->OSPRayVolume = ospNewVolume("unstructured");

      this->Vertices.clear();
      double point[3];
      for (int i = 0; i < numberOfPoints; i++)
      {
        dataSet->GetPoint(i, point);
        osp::vec3f v;
        v.x = point[0];
        v.y = point[1];
        v.z = point[2];
        this->Vertices.push_back(v);
      }

      this->Cells.clear();
      this->CellIndices.clear();
      this->CellTypes.clear();
      for (int i = 0; i < numberOfCells; i++)
      {
        this->CellIndices.push_back(static_cast<unsigned int>(this->Cells.size()));
        vtkCell* cell = dataSet->GetCell(i);
        if (cell->GetCellType() == VTK_TETRA)
        {
          this->CellTypes.push_back(OSP_TETRAHEDRON);
          for (int j = 0; j < 4; j++)
          {
            this->Cells.push_back(cell->GetPointId(j));
          }
        }
        else if (cell->GetCellType() == VTK_HEXAHEDRON)
        {
          this->CellTypes.push_back(OSP_HEXAHEDRON);
          for (int j = 0; j < 8; j++)
          {
            this->Cells.push_back(cell->GetPointId(j));
          }
        }
        else if (cell->GetCellType() == VTK_WEDGE)
        {
          this->CellTypes.push_back(OSP_WEDGE);
          for (int j = 0; j < 6; ++j)
          {
            this->Cells.push_back(cell->GetPointId(j));
          }
        }
        else
        {
          // TODO: WEDGE
          vtkWarningMacro("Unsupported cell type encountered: "
            << cell->GetClassName() << " id=" << cell->GetCellType() << ". Ignored.");
        }
      }

      // Now the point data to volume render
      this->Field.clear();
      for (int j = 0; j < (fieldAssociation ? numberOfCells : numberOfPoints); j++)
      {
        // TODO: when OSP TET volume gets other types, use them
        // TODO: try pass ref to entire array instead of this val by val copy
        float val = static_cast<float>(array->GetTuple1(j));
        this->Field.push_back(val);
      }

      OSPData verticesData =
        ospNewCopyData1D(this->Vertices.data(), OSP_VEC3F, this->Vertices.size());
      assert(verticesData);
      ospSetObject(this->OSPRayVolume, "vertex.position", verticesData);

      OSPData fieldData = ospNewCopyData1D(this->Field.data(), OSP_FLOAT, this->Field.size());
      assert(fieldData);
      if (fieldAssociation)
      {
        ospSetObject(this->OSPRayVolume, "cell.data", fieldData);
      }
      else
      {
        ospSetObject(this->OSPRayVolume, "vertex.data", fieldData);
      }

      OSPData indexData = ospNewCopyData1D(this->Cells.data(), OSP_UINT, this->Cells.size());
      ospSetObject(this->OSPRayVolume, "index", indexData);
      OSPData cellTypeData =
        ospNewCopyData1D(this->CellTypes.data(), OSP_UCHAR, this->CellTypes.size());
      ospSetObject(this->OSPRayVolume, "cell.type", cellTypeData);
      OSPData cellIndexData =
        ospNewCopyData1D(this->CellIndices.data(), OSP_UINT, this->CellIndices.size());
      ospSetObject(this->OSPRayVolume, "cell.index", cellIndexData);
    }
    ospCommit(this->OSPRayVolume);

    double* dim = mapper->GetBounds();
    int minBound = std::min(std::min(dim[1] - dim[0], dim[3] - dim[2]), dim[5] - dim[4]);
    float samplingStep = minBound * 0.01f;

    // test for modifications to volume properties
    vtkVolumeProperty* volProperty = vol->GetProperty();
    if (vol->GetProperty()->GetMTime() > this->PropertyTime ||
      mapper->GetDataSetInput()->GetMTime() > this->BuildTime)
    {
      // Get transfer function.
      vtkColorTransferFunction* colorTF = volProperty->GetRGBTransferFunction(0);
      vtkPiecewiseFunction* scalarTF = volProperty->GetScalarOpacity(0);

      this->TFVals.resize(this->NumColors * 3);
      this->TFOVals.resize(this->NumColors);
      scalarTF->GetTable(array->GetRange()[0], array->GetRange()[1], this->NumColors, &TFOVals[0]);
      colorTF->GetTable(
        array->GetRange()[0], array->GetRange()[1], this->NumColors, &this->TFVals[0]);

      float scalarOpacityUnitDistance = volProperty->GetScalarOpacityUnitDistance();
      if (scalarOpacityUnitDistance < 1e-29) // avoid div by 0
      {
        scalarOpacityUnitDistance = 1e-29;
      }
      for (int i = 0; i < this->NumColors; i++)
      {
        this->TFOVals[i] = this->TFOVals[i] / scalarOpacityUnitDistance * samplingStep;
      }

      OSPData colorData = ospNewCopyData1D(&this->TFVals[0], OSP_VEC3F, this->NumColors);

      ospSetObject(this->TransferFunction, "color", colorData);

      OSPData tfAlphaData = ospNewCopyData1D(&TFOVals[0], OSP_FLOAT, NumColors);
      ospSetObject(this->TransferFunction, "opacity", tfAlphaData);

      ospSetVec2f(this->TransferFunction, "valueRange", array->GetRange()[0], array->GetRange()[1]);

      ospCommit(this->TransferFunction);
      ospRelease(colorData);
      ospRelease(tfAlphaData);
      ospRelease(this->OSPRayVolumeModel);

      this->OSPRayVolumeModel = ospNewVolumetricModel(this->OSPRayVolume);
      ospSetObject(this->OSPRayVolumeModel, "transferFunction", this->TransferFunction);
      const float densityScale = 1.0f / volProperty->GetScalarOpacityUnitDistance();
      ospSetFloat(this->OSPRayVolumeModel, "densityScale", densityScale);
      const float anisotropy = orn->GetVolumeAnisotropy(ren); // Carson-TODO: unhardcode
      ospSetFloat(this->OSPRayVolumeModel, "anisotropy", anisotropy);
      ospSetFloat(
        this->OSPRayVolumeModel, "gradientShadingScale", volProperty->GetShade() ? 0.5 : 0.0);
      ospCommit(this->OSPRayVolumeModel);

      this->PropertyTime.Modified();
    }

    OSPGroup group = ospNewGroup();
    OSPInstance instance = ospNewInstance(group);
    OSPData instanceData = ospNewCopyData1D(&this->OSPRayVolumeModel, OSP_VOLUMETRIC_MODEL, 1);
    ospCommit(instanceData);
    ospSetObject(group, "volume", instanceData);
    ospCommit(group);
    ospCommit(instance);
    ospRelease(group);
    orn->Instances.emplace_back(instance);
    this->OSPRayInstance = instance;

    this->RenderTime = volNode->GetMTime();
    this->BuildTime.Modified();
  }
}

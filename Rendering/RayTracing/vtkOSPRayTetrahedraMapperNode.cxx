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

//-----------------------------------------------------------------------------
vtkOSPRayTetrahedraMapperNode::vtkOSPRayTetrahedraMapperNode()
{
  this->SamplingRate = 0.0f;
  this->NumColors = 128;
  this->OSPRayVolume = nullptr;
  this->TransferFunction = nullptr;
  this->Cache = new vtkOSPRayCache<vtkOSPRayCacheItemObject>();
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
      if (this->Cache->GetSize() == 0)
      {
        ospRelease(this->OSPRayVolume);
      }
    }
  }
  delete this->Cache;
}

void vtkOSPRayTetrahedraMapperNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
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
    this->Cache->SetSize(vtkOSPRayRendererNode::GetTimeCacheSize(ren));

    OSPModel OSPRayModel = orn->GetOModel();
    if (!OSPRayModel)
    {
      return;
    }

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
      this->TransferFunction = ospNewTransferFunction("piecewise_linear");
    }

    // when input data is modified
    if (mapper->GetDataSetInput()->GetMTime() > this->BuildTime)
    {
      double tstep = vtkOSPRayRendererNode::GetViewTime(ren);
      auto cached_entry = this->Cache->Get(tstep);
      auto cached_Volume = cached_entry ? this->Cache->Get(tstep)->object : nullptr;
      if (cached_Volume)
      {
        this->OSPRayVolume = static_cast<OSPVolume>(cached_Volume);
      }
      else
      {
        if (this->OSPRayVolume && this->Cache->GetSize() == 0)
        {
          ospRelease(this->OSPRayVolume);
        }
        this->OSPRayVolume = ospNewVolume("unstructured_volume");
        if (this->Cache->HasRoom())
        {
          auto cacheEntry = std::make_shared<vtkOSPRayCacheItemObject>(backend, this->OSPRayVolume);
          this->Cache->Set(tstep, cacheEntry);
        }

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
        for (int i = 0; i < numberOfCells; i++)
        {
          vtkCell* cell = dataSet->GetCell(i);
          if (cell->GetCellType() == VTK_TETRA)
          {
            for (int j = 0; j < 4; j++)
            {
              this->Cells.push_back(-1);
            }
            for (int j = 0; j < 4; j++)
            {
              this->Cells.push_back(cell->GetPointId(j));
            }
          }
          else if (cell->GetCellType() == VTK_HEXAHEDRON)
          {
            for (int j = 0; j < 8; j++)
            {
              this->Cells.push_back(cell->GetPointId(j));
            }
          }
          else if (cell->GetCellType() == VTK_WEDGE)
          {
            for (int j = 0; j < 2; ++j)
            {
              this->Cells.push_back(-1);
            }
            for (int j = 0; j < 6; ++j)
            {
              this->Cells.push_back(cell->GetPointId(j));
            }
          }
          else
          {
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
          ospNewData(this->Vertices.size(), OSP_FLOAT3, this->Vertices.data(), 0);
        assert(verticesData);
        ospSetData(this->OSPRayVolume, "vertices", verticesData);

        OSPData fieldData = ospNewData(this->Field.size(), OSP_FLOAT, this->Field.data(), 0);
        assert(fieldData);
        if (fieldAssociation)
        {
          ospSetData(this->OSPRayVolume, "cellField", fieldData);
        }
        else
        {
          ospSetData(this->OSPRayVolume, "field", fieldData);
        }

        OSPData indicesData = ospNewData(this->Cells.size() / 4, OSP_INT4, this->Cells.data(), 0);
        ospSetData(this->OSPRayVolume, "indices", indicesData);

        ospSet1i(this->OSPRayVolume, "nVertices", static_cast<int>(this->Vertices.size()));
        ospSet1i(this->OSPRayVolume, "nCells", static_cast<int>(this->Cells.size()) / 8);
      }
    }

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

      OSPData colorData = ospNewData(this->NumColors, OSP_FLOAT3, &this->TFVals[0]);

      ospSetData(this->TransferFunction, "colors", colorData);

      OSPData tfAlphaData = ospNewData(NumColors, OSP_FLOAT, &TFOVals[0]);
      ospSetData(this->TransferFunction, "opacities", tfAlphaData);

      ospSet2f(this->TransferFunction, "valueRange", array->GetRange()[0], array->GetRange()[1]);

      ospSet1i(this->OSPRayVolume, "gradientShadingEnabled", volProperty->GetShade());

      ospCommit(this->TransferFunction);
      ospRelease(colorData);
      ospRelease(tfAlphaData);

      ospSetObject(this->OSPRayVolume, "transferFunction", this->TransferFunction);

      this->PropertyTime.Modified();
    }

    ospSet1f(this->OSPRayVolume, "samplingStep", samplingStep);
    ospSet1f(this->OSPRayVolume, "adaptiveMaxSamplingRate", 2.0f);
    ospSet1f(this->OSPRayVolume, "adaptiveBacktrack", 0.01f);
    ospSet1i(this->OSPRayVolume, "adaptiveSampling", 1);
    if (this->SamplingRate == 0.0f) // 0 means automatic sampling rate
    {
      // automatically determine sampling rate
      if (minBound < 100)
      {
        float s = (100.0f - minBound) / 100.0f;
        ospSet1f(this->OSPRayVolume, "samplingRate", s * 2.0f + 0.2f);
      }
      else
      {
        ospSet1f(this->OSPRayVolume, "samplingRate", 0.2f);
      }
    }
    else
    {
      ospSet1f(this->OSPRayVolume, "samplingRate", this->SamplingRate);
    }
    ospSet1f(this->OSPRayVolume, "adaptiveScalar", 15.f);
    float rs = static_cast<float>(volProperty->GetSpecular(0));
    float gs = static_cast<float>(volProperty->GetSpecular(1));
    float bs = static_cast<float>(volProperty->GetSpecular(2));
    ospSet3f(this->OSPRayVolume, "specular", rs, gs, bs);
    ospSet1i(this->OSPRayVolume, "preIntegration", 0); // turn off preIntegration

    ospCommit(this->OSPRayVolume);
    ospAddVolume(OSPRayModel, this->OSPRayVolume);
    this->RenderTime = volNode->GetMTime();
    this->BuildTime.Modified();
  }
}

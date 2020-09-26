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
#include "vtkFloatArray.h"
#include "vtkOSPRayCache.h"
#include "vtkOSPRayRendererNode.h"
#include "vtkObjectFactory.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkRenderer.h"
#include "vtkUnstructuredGrid.h"
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

    vtkUnstructuredGrid* dataSet = vtkUnstructuredGrid::SafeDownCast(mapper->GetDataSetInput());
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
      ospRelease(this->OSPRayVolume);
      this->OSPRayVolume = ospNewVolume("unstructured");

      OSPData verticesData;
      vtkFloatArray* vptsArray = vtkFloatArray::FastDownCast(dataSet->GetPoints()->GetData());
      if (!vptsArray)
      {
        std::vector<osp::vec3f> vertices;
        vertices.reserve(numberOfPoints);
        double point[3];
        for (int i = 0; i < numberOfPoints; i++)
        {
          dataSet->GetPoint(i, point);
          vertices[i] = { static_cast<float>(point[0]), static_cast<float>(point[1]),
            static_cast<float>(point[2]) };
        }
        verticesData = ospNewCopyData1D(vertices.data(), OSP_VEC3F, numberOfPoints);
      }
      else
      {
        verticesData = ospNewSharedData1D(vptsArray->GetVoidPointer(0), OSP_VEC3F, numberOfPoints);
      }
      ospSetObject(this->OSPRayVolume, "vertex.position", verticesData);
      std::vector<unsigned int> cells;
      std::vector<unsigned char> cellTypes;
      cellTypes.resize(numberOfCells);
      std::vector<unsigned int> cellIndices;
      for (int i = 0; i < numberOfCells; i++)
      {
        cellIndices.push_back(static_cast<unsigned int>(cells.size()));
        vtkCell* cell = dataSet->GetCell(i);
        if (cell->GetCellType() == VTK_TETRA)
        {
          cellTypes[i] = OSP_TETRAHEDRON;
          for (int j = 0; j < 4; j++)
          {
            cells.push_back(cell->GetPointId(j));
          }
        }
        else if (cell->GetCellType() == VTK_VOXEL)
        {
          cellTypes[i] = OSP_HEXAHEDRON;
          cells.push_back(cell->GetPointId(0));
          cells.push_back(cell->GetPointId(1));
          cells.push_back(cell->GetPointId(2));
          cells.push_back(cell->GetPointId(3));
          cells.push_back(cell->GetPointId(4));
          cells.push_back(cell->GetPointId(5));
          cells.push_back(cell->GetPointId(6));
          cells.push_back(cell->GetPointId(7));
        }
        else if (cell->GetCellType() == VTK_HEXAHEDRON)
        {
          cellTypes[i] = OSP_HEXAHEDRON;
          for (int j = 0; j < 8; j++)
          {
            cells.push_back(cell->GetPointId(j));
          }
        }
        else if (cell->GetCellType() == VTK_WEDGE)
        {
          cellTypes[i] = OSP_WEDGE;
          for (int j = 0; j < 6; ++j)
          {
            cells.push_back(cell->GetPointId(j));
          }
        }
        else if (cell->GetCellType() == VTK_PYRAMID)
        {
          cellTypes[i] = OSP_PYRAMID;
          for (int j = 0; j < 5; ++j)
          {
            cells.push_back(cell->GetPointId(j));
          }
        }
        else
        {
          vtkWarningMacro("Unsupported cell type encountered: "
            << cell->GetClassName() << " id=" << cell->GetCellType() << ". Ignored.");
        }
      }
      OSPData indexData = ospNewCopyData1D(cells.data(), OSP_UINT, cells.size());
      ospSetObject(this->OSPRayVolume, "index", indexData);
      OSPData cellTypeData = ospNewCopyData1D(cellTypes.data(), OSP_UCHAR, cellTypes.size());
      ospSetObject(this->OSPRayVolume, "cell.type", cellTypeData);
      OSPData cellIndexData = ospNewCopyData1D(cellIndices.data(), OSP_UINT, cellIndices.size());
      ospSetObject(this->OSPRayVolume, "cell.index", cellIndexData);
    }

    // Now the data to volume render
    vtkVolumeProperty* volProperty = vol->GetProperty();
    vtkColorTransferFunction* ctf = volProperty->GetRGBTransferFunction(0);
    int const indep = volProperty->GetIndependentComponents();
    int const mode = indep ? ctf->GetVectorMode() : vtkScalarsToColors::COMPONENT;
    int const comp = indep ? ctf->GetVectorComponent() : 0;
    int const val = (mode << 6) | comp; // combine to compare as one
    if (mapper->GetDataSetInput()->GetMTime() > this->BuildTime ||
      this->LastArrayName != mapper->GetArrayName() || this->LastArrayComponent != val)
    {
      this->LastArrayName = mapper->GetArrayName();
      this->LastArrayComponent = val;
      vtkIdType numberOfElements = (fieldAssociation ? numberOfCells : numberOfPoints);
      OSPData fieldData;
      bool VKLsupported = array->GetDataType() == VTK_FLOAT;
      if (array->GetNumberOfComponents() == 1 && VKLsupported)
      {
        fieldData = ospNewSharedData1D(array->GetVoidPointer(0), OSP_FLOAT, numberOfElements);
      }
      else
      {
        std::vector<float> field;
        field.reserve(numberOfElements);
        for (int j = 0; j < numberOfElements; j++)
        {
          double* vals = array->GetTuple(j);
          double mag = 0;
          if (mode == 0) // display vector magnitude
          {
            for (int c = 0; c < array->GetNumberOfComponents(); c++)
            {
              {
                mag += vals[c] * vals[c];
              }
            }
            mag = std::sqrt(mag);
          }
          else
          {
            mag = vals[comp];
          }
          field[j] = static_cast<float>(mag);
        }
        fieldData = ospNewCopyData1D(field.data(), OSP_FLOAT, numberOfElements);
      }
      if (fieldAssociation)
      {
        ospSetObject(this->OSPRayVolume, "cell.data", fieldData);
      }
      else
      {
        ospSetObject(this->OSPRayVolume, "vertex.data", fieldData);
      }
      ospCommit(this->OSPRayVolume);
      ospRelease(fieldData);
    }

    double* dim = mapper->GetBounds();
    double minBound = std::min(std::min(dim[1] - dim[0], dim[3] - dim[2]), dim[5] - dim[4]);
    float samplingStep = minBound * 0.01f;

    // test for modifications to volume properties
    if (volProperty->GetMTime() > this->PropertyTime ||
      mapper->GetDataSetInput()->GetMTime() > this->BuildTime)
    {
      // Get transfer function.
      vtkColorTransferFunction* colorTF = volProperty->GetRGBTransferFunction(0);
      vtkPiecewiseFunction* scalarTF = volProperty->GetScalarOpacity(0);

      std::vector<float> tfCVals;
      std::vector<float> tfOVals;

      tfCVals.resize(this->NumColors * 3);
      tfOVals.resize(this->NumColors);
      scalarTF->GetTable(array->GetRange()[0], array->GetRange()[1], this->NumColors, &tfOVals[0]);
      colorTF->GetTable(array->GetRange()[0], array->GetRange()[1], this->NumColors, &tfCVals[0]);

      float scalarOpacityUnitDistance = volProperty->GetScalarOpacityUnitDistance();
      if (scalarOpacityUnitDistance < 1e-29) // avoid div by 0
      {
        scalarOpacityUnitDistance = 1e-29;
      }
      for (int i = 0; i < this->NumColors; i++)
      {
        tfOVals[i] = tfOVals[i] / scalarOpacityUnitDistance * samplingStep;
      }

      OSPData colorData = ospNewCopyData1D(&tfCVals[0], OSP_VEC3F, this->NumColors);

      ospSetObject(this->TransferFunction, "color", colorData);

      OSPData tfAlphaData = ospNewCopyData1D(&tfOVals[0], OSP_FLOAT, NumColors);
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
      const float anisotropy = orn->GetVolumeAnisotropy(ren);
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

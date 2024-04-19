// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOSPRayUnstructuredVolumeMapperNode.h"

#include "vtkCell.h"
#include "vtkCellTypes.h"
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

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkOSPRayUnstructuredVolumeMapperNode);

//------------------------------------------------------------------------------
vtkOSPRayUnstructuredVolumeMapperNode::vtkOSPRayUnstructuredVolumeMapperNode()
{
  this->SamplingRate = 0.0f;
  this->NumColors = 128;
  this->OSPRayVolume = nullptr;
  this->OSPRayVolumeModel = nullptr;
}

void vtkOSPRayUnstructuredVolumeMapperNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkOSPRayUnstructuredVolumeMapperNode::Render(bool prepass)
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

    // when input data is modified
    if (mapper->GetDataSetInput()->GetMTime() > this->BuildTime)
    {
      ospRelease(this->OSPRayVolume);

      vtkUnsignedCharArray* iCellTypes = dataSet->GetDistinctCellTypesArray();
      for (vtkIdType cti = 0; cti < iCellTypes->GetNumberOfValues(); cti++)
      {
        auto ct = iCellTypes->GetValue(cti);
        if (ct != VTK_TETRA && ct != VTK_HEXAHEDRON && ct != VTK_WEDGE && ct != VTK_PYRAMID)
        {
          vtkWarningMacro("Unsupported voxel type " << ct);
          return;
        }
      }

      this->OSPRayVolume = ospNewVolume("unstructured");

      // First the spatial locations
      OSPData verticesData;
      vtkFloatArray* vptsArray = vtkFloatArray::FastDownCast(dataSet->GetPoints()->GetData());
      if (vptsArray)
      {
        verticesData = ospNewSharedData1D(vptsArray->GetVoidPointer(0), OSP_VEC3F, numberOfPoints);
      }
      else
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
      ospCommit(verticesData);
      ospSetObject(this->OSPRayVolume, "vertex.position", verticesData);
      ospRelease(verticesData);

      // Now the connectivity
      auto cellArray = dataSet->GetCells();
      bool isShareable = cellArray->IsStorageShareable();
      if (isShareable)
      {
        bool is32bit = !cellArray->IsStorage64Bit();
        auto ctypes = dataSet->GetCellTypesArray();
        OSPData cellTypeData =
          ospNewSharedData1D(ctypes->GetVoidPointer(0), OSP_UCHAR, numberOfCells);
        ospCommit(cellTypeData);
        ospSetObject(this->OSPRayVolume, "cell.type", cellTypeData);
        auto off = cellArray->GetOffsetsArray();
        OSPData cellIndexData =
          ospNewSharedData1D(off->GetVoidPointer(0), is32bit ? OSP_UINT : OSP_ULONG, numberOfCells);
        ospCommit(cellIndexData);
        ospSetObject(this->OSPRayVolume, "cell.index", cellIndexData);
        auto con = cellArray->GetConnectivityArray();
        OSPData indexData = ospNewSharedData1D(
          con->GetVoidPointer(0), is32bit ? OSP_UINT : OSP_ULONG, con->GetNumberOfTuples() - 1);
        ospCommit(indexData);
        ospSetObject(this->OSPRayVolume, "index", indexData);
        ospRelease(cellTypeData);
        ospRelease(cellIndexData);
        ospRelease(indexData);
      }
      else
      {
        std::vector<unsigned char> ctypes;
        ctypes.resize(numberOfCells);
        std::vector<unsigned int> off;
        off.resize(numberOfCells);
        std::vector<unsigned int> con;
        for (int i = 0; i < numberOfCells; i++)
        {
          off[i] = static_cast<unsigned int>(con.size());
          vtkCell* cell = dataSet->GetCell(i);
          if (cell->GetCellType() == VTK_TETRA)
          {
            ctypes[i] = OSP_TETRAHEDRON;
            for (int j = 0; j < 4; j++)
            {
              con.push_back(cell->GetPointId(j));
            }
          }
          else if (cell->GetCellType() == VTK_HEXAHEDRON)
          {
            ctypes[i] = OSP_HEXAHEDRON;
            for (int j = 0; j < 8; j++)
            {
              con.push_back(cell->GetPointId(j));
            }
          }
          else if (cell->GetCellType() == VTK_WEDGE)
          {
            ctypes[i] = OSP_WEDGE;
            for (int j = 0; j < 6; ++j)
            {
              con.push_back(cell->GetPointId(j));
            }
          }
          else if (cell->GetCellType() == VTK_PYRAMID)
          {
            ctypes[i] = OSP_PYRAMID;
            for (int j = 0; j < 5; ++j)
            {
              con.push_back(cell->GetPointId(j));
            }
          }
        }
        OSPData cellTypeData = ospNewCopyData1D(ctypes.data(), OSP_UCHAR, ctypes.size());
        ospCommit(cellTypeData);
        ospSetObject(this->OSPRayVolume, "cell.type", cellTypeData);
        OSPData cellIndexData = ospNewCopyData1D(off.data(), OSP_UINT, off.size());
        ospCommit(cellIndexData);
        ospSetObject(this->OSPRayVolume, "cell.index", cellIndexData);
        OSPData indexData = ospNewCopyData1D(con.data(), OSP_UINT, con.size());
        ospCommit(indexData);
        ospSetObject(this->OSPRayVolume, "index", indexData);
        ospRelease(cellTypeData);
        ospRelease(cellIndexData);
        ospRelease(indexData);
      }
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
          if (mode == 0 && array->GetNumberOfComponents() > 1) // vector magnitude
          {
            for (int c = 0; c < array->GetNumberOfComponents(); c++)
            {
              mag += vals[c] * vals[c];
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
      ospCommit(fieldData);
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

    // finally appearance
    if (volProperty->GetMTime() > this->PropertyTime ||
      mapper->GetDataSetInput()->GetMTime() > this->BuildTime)
    {
      double* dim = mapper->GetBounds();
      double minBound = std::min(std::min(dim[1] - dim[0], dim[3] - dim[2]), dim[5] - dim[4]);
      float samplingStep = minBound * 0.01f;

      // Get transfer function.
      vtkColorTransferFunction* colorTF = volProperty->GetRGBTransferFunction(0);
      vtkPiecewiseFunction* scalarTF = volProperty->GetScalarOpacity(0);

      std::vector<float> tfCVals;
      std::vector<float> tfOVals;

      tfCVals.resize(this->NumColors * 3);
      tfOVals.resize(this->NumColors);
      double range[2];
      // prefer transfer function's range
      scalarTF->GetRange(range);
      // but use data's range if we can and we have to
      if (range[1] <= range[0])
      {
        array->GetRange(range, comp);
        if (mode == 0 && array->GetNumberOfComponents() > 1) // vector magnitude
        {
          double min = 0;
          double max = 0;
          for (int c = 0; c < array->GetNumberOfComponents(); c++)
          {
            double lmin = 0;
            double lmax = 0;
            double cRange[2];
            array->GetRange(cRange, c);
            double ldist = cRange[0] * cRange[0];
            double rdist = cRange[1] * cRange[1];
            lmin = std::min(ldist, rdist);
            if (cRange[0] < 0 && cRange[1] > 0)
            {
              lmin = 0;
            }
            lmax = std::max(ldist, rdist);
            min += lmin;
            max += lmax;
          }
          range[0] = std::sqrt(min);
          range[1] = std::sqrt(max);
        }
      }

      scalarTF->GetTable(range[0], range[1], this->NumColors, &tfOVals[0]);
      colorTF->GetTable(range[0], range[1], this->NumColors, &tfCVals[0]);

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
      ospCommit(colorData);

      auto oTF = ospNewTransferFunction("piecewiseLinear");
      ospSetObject(oTF, "color", colorData);
      OSPData tfAlphaData = ospNewCopyData1D(&tfOVals[0], OSP_FLOAT, NumColors);
      ospCommit(tfAlphaData);
      ospSetObject(oTF, "opacity", tfAlphaData);
#if OSPRAY_VERSION_MAJOR < 3
      ospSetVec2f(oTF, "valueRange", (float)range[0], (float)range[1]);
#else
      ospSetBox1f(oTF, "value", (float)range[0], (float)range[1]);
#endif
      ospCommit(oTF);

      ospRelease(colorData);
      ospRelease(tfAlphaData);

      ospRelease(this->OSPRayVolumeModel);
      this->OSPRayVolumeModel = ospNewVolumetricModel(this->OSPRayVolume);
      ospSetObject(this->OSPRayVolumeModel, "transferFunction", oTF);
      const float densityScale = 1.0f / volProperty->GetScalarOpacityUnitDistance();
      ospSetFloat(this->OSPRayVolumeModel, "densityScale", densityScale);
      const float anisotropy = volProperty->GetScatteringAnisotropy();
      ospSetFloat(this->OSPRayVolumeModel, "anisotropy", anisotropy);
      ospSetFloat(
        this->OSPRayVolumeModel, "gradientShadingScale", volProperty->GetShade() ? 0.5 : 0.0);
      ospCommit(this->OSPRayVolumeModel);
      ospRelease(oTF);

      this->PropertyTime.Modified();
    }

    OSPGroup group = ospNewGroup();
    // instance object doesn't need a matching ospRelease() here because the responsibility
    // for its destruction gets handed off to the vtkRendererNode
    OSPInstance instance = ospNewInstance(group);
    OSPData instanceData = ospNewSharedData1D(&this->OSPRayVolumeModel, OSP_VOLUMETRIC_MODEL, 1);
    ospCommit(instanceData);
    ospSetObject(group, "volume", instanceData);
    ospCommit(group);
    ospRelease(instanceData);
    ospCommit(instance);
    ospRelease(group);
    orn->Instances.emplace_back(instance);

    this->RenderTime = volNode->GetMTime();
    this->BuildTime.Modified();
  }
}
VTK_ABI_NAMESPACE_END

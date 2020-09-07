/*=========================================================================

   Program:   Visualization Toolkit
   Module:    vtkOSPRayAMRVolumeMapperNode.cxx

   Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
   All rights reserved.
   See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

      This software is distributed WITHOUT ANY WARRANTY; without even
      the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
      PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkOSPRayAMRVolumeMapperNode.h"

#include "vtkAMRBox.h"
#include "vtkAMRInformation.h"
#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkOSPRayCache.h"
#include "vtkOSPRayRendererNode.h"
#include "vtkObjectFactory.h"
#include "vtkOverlappingAMR.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkUniformGridAMRDataIterator.h"
#include "vtkVolume.h"
#include "vtkVolumeMapper.h"
#include "vtkVolumeNode.h"
#include "vtkVolumeProperty.h"

#include "RTWrapper/RTWrapper.h"

#include <cassert>

vtkStandardNewMacro(vtkOSPRayAMRVolumeMapperNode);

//------------------------------------------------------------------------------
vtkOSPRayAMRVolumeMapperNode::vtkOSPRayAMRVolumeMapperNode()
{
  this->NumColors = 128;
  this->TransferFunction = nullptr;
  this->SamplingRate = 0.5f;
  this->OldSamplingRate = -1.f;
}

//------------------------------------------------------------------------------
void vtkOSPRayAMRVolumeMapperNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkOSPRayAMRVolumeMapperNode::Render(bool prepass)
{
  if (prepass)
  {
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
    vtkVolumeMapper* mapper = vtkVolumeMapper::SafeDownCast(this->GetRenderable());
    if (!mapper)
    {
      vtkErrorMacro("invalid mapper");
      return;
    }
    if (!vol->GetProperty())
    {
      vtkErrorMacro("VolumeMapper had no vtkProperty");
      return;
    }

    vtkOSPRayRendererNode* orn =
      static_cast<vtkOSPRayRendererNode*>(this->GetFirstAncestorOfType("vtkOSPRayRendererNode"));
    vtkRenderer* ren = vtkRenderer::SafeDownCast(orn->GetRenderable());

    RTW::Backend* backend = orn->GetBackend();
    if (backend == nullptr)
    {
      return;
    }
    if (!this->TransferFunction)
    {
      this->TransferFunction = ospNewTransferFunction("piecewiseLinear");
    }

    vtkOverlappingAMR* amr = vtkOverlappingAMR::SafeDownCast(mapper->GetInputDataObject(0, 0));
    if (!amr)
    {
      vtkErrorMacro("couldn't get amr data\n");
      return;
    }

    vtkVolumeProperty* volProperty = vol->GetProperty();

    bool volDirty = false;
    if (!this->OSPRayVolume || amr->GetMTime() > this->BuildTime)
    {
      this->OSPRayVolume = ospNewVolume("amr");
      volDirty = true;

      unsigned int lastLevel = 0;
      std::vector<OSPData> brickDataArray;
      std::vector<float> cellWidthArray;
      std::vector<ospcommon::box3i> blockBoundsArray;
      std::vector<int> blockLevelArray;

      vtkAMRInformation* amrInfo = amr->GetAMRInfo();
      vtkSmartPointer<vtkUniformGridAMRDataIterator> iter;
      iter.TakeReference(vtkUniformGridAMRDataIterator::SafeDownCast(amr->NewIterator()));
      for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
        unsigned int level = iter->GetCurrentLevel();
        if (!(level >= lastLevel))
        {
          vtkErrorMacro("ospray requires level info be ordered lowest to highest");
        }
        lastLevel = level;
        unsigned int index = iter->GetCurrentIndex();

        // note: this iteration "naturally" goes from datasets at lower levels to
        // those at higher levels.
        vtkImageData* data = vtkImageData::SafeDownCast(iter->GetCurrentDataObject());
        if (!data)
        {
          return;
        }
        int dim[3];

        const vtkAMRBox& box = amrInfo->GetAMRBox(level, index);
        const int* lo = box.GetLoCorner();
        const int* hi = box.GetHiCorner();
        ospcommon::vec3i lo_v = { lo[0], lo[1], lo[2] };
        ospcommon::vec3i hi_v = { hi[0], hi[1], hi[2] };
        dim[0] = hi[0] - lo[0] + 1;
        dim[1] = hi[1] - lo[1] + 1;
        dim[2] = hi[2] - lo[2] + 1;
        int fieldAssociation;
        mapper->SetScalarMode(VTK_SCALAR_MODE_USE_CELL_FIELD_DATA);
        vtkDataArray* cellArray =
          vtkDataArray::SafeDownCast(this->GetArrayToProcess(data, fieldAssociation));
        if (!cellArray)
        {
          std::cerr << "could not get data!\n";
          return;
        }

        OSPData odata;
        if (cellArray->GetDataType() != VTK_FLOAT)
        {
          std::vector<float> fdata(dim[0] * dim[1] * dim[2]);
          for (int i = 0; i < dim[0] * dim[1] * dim[2]; i++)
          {
            fdata[i] = static_cast<float>(cellArray->GetTuple(i)[0]);
          }
          odata = ospNewCopyData1D(fdata.data(), OSP_FLOAT, dim[0] * dim[1] * dim[2]);
          ospCommit(odata);
        }
        else
        {
          odata = ospNewSharedData1D(reinterpret_cast<float*>(cellArray->GetVoidPointer(0)),
            OSP_FLOAT, dim[0] * dim[1] * dim[2]);
          ospCommit(odata);
        }

        brickDataArray.push_back(odata);
        blockLevelArray.push_back(level);
        ospcommon::box3i obox = { lo_v, hi_v };
        blockBoundsArray.push_back(obox);
      }

      double* bds = mapper->GetBounds();
      ospSetVec3f(this->OSPRayVolume, "gridOrigin", static_cast<float>(bds[0]),
        static_cast<float>(bds[2]), static_cast<float>(bds[4]));
      double spacing[3] = { 1.0, 1.0, 1.0 };
      amr->GetAMRInfo()->GetSpacing(0, spacing);
      ospSetVec3f(this->OSPRayVolume, "gridSpacing", static_cast<float>(spacing[0]),
        static_cast<float>(spacing[1]), static_cast<float>(spacing[2]));

      for (unsigned int i = 0; i < amrInfo->GetNumberOfLevels(); ++i)
      {
        amrInfo->GetSpacing(i, spacing);
        cellWidthArray.push_back(spacing[0]); // TODO - must OSP cells be cubes?
      }
      OSPData cellWidthData =
        ospNewCopyData1D(cellWidthArray.data(), OSP_FLOAT, cellWidthArray.size());
      ospCommit(cellWidthData);
      ospSetObject(this->OSPRayVolume, "cellWidth", cellWidthData);

      OSPData brickDataData =
        ospNewCopyData1D(brickDataArray.data(), OSP_DATA, brickDataArray.size());
      ospCommit(brickDataData);
      ospSetObject(this->OSPRayVolume, "block.data", brickDataData);
      OSPData blockBoundsData =
        ospNewCopyData1D(blockBoundsArray.data(), OSP_BOX3I, blockBoundsArray.size());
      ospCommit(blockBoundsData);
      ospSetObject(this->OSPRayVolume, "block.bounds", blockBoundsData);
      OSPData blockLevelData =
        ospNewCopyData1D(blockLevelArray.data(), OSP_INT, blockLevelArray.size());
      ospCommit(blockLevelData);
      ospSetObject(this->OSPRayVolume, "block.level", blockLevelData);
      this->BuildTime.Modified();
    }
    if ((vol->GetProperty()->GetMTime() > this->PropertyTime) || volDirty)
    {
      //
      // transfer function
      //
      this->UpdateTransferFunction(backend, vol);
      ospSetInt(this->OSPRayVolume, "gradientShadingEnabled", volProperty->GetShade());
      this->PropertyTime.Modified();
    }

    if (this->OldSamplingRate != this->SamplingRate)
    {
      this->OldSamplingRate = this->SamplingRate;
      volDirty = true;
    }

    if (volDirty)
    {
      ospCommit(this->OSPRayVolume);
    }

    ospRelease(this->OSPRayVolumeModel);
    ospRelease(this->OSPRayInstance);
    this->OSPRayVolumeModel = ospNewVolumetricModel(this->OSPRayVolume);
    ospSetObject(this->OSPRayVolumeModel, "transferFunction", this->TransferFunction);
    const float densityScale = 1.0f / volProperty->GetScalarOpacityUnitDistance();
    ospSetFloat(this->OSPRayVolumeModel, "densityScale", densityScale);
    const float anisotropy = orn->GetVolumeAnisotropy(ren);
    ospSetFloat(this->OSPRayVolumeModel, "anisotropy", anisotropy);
    ospSetFloat(
      this->OSPRayVolumeModel, "gradientShadingScale", volProperty->GetShade() ? 0.5 : 0.0);
    ospCommit(this->OSPRayVolumeModel);

    OSPGroup group = ospNewGroup();
    OSPInstance instance = ospNewInstance(group);
    ospCommit(instance);
    ospRelease(group);
    OSPData instanceData = ospNewCopyData1D(&this->OSPRayVolumeModel, OSP_VOLUMETRIC_MODEL, 1);
    ospCommit(instanceData);
    ospSetObject(group, "volume", instanceData);
    ospCommit(group);
    orn->Instances.emplace_back(instance);
    this->OSPRayInstance = instance;
  } // prepass
}

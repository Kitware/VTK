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

namespace ospray
{
namespace amr
{
struct BrickInfo
{
  /*! bounding box of integer coordinates of cells. note that
      this EXCLUDES the width of the rightmost cell: ie, a 4^3
      box at root level pos (0,0,0) would have a _box_ of
      [(0,0,0)-(3,3,3)] (because 3,3,3 is the highest valid
      coordinate in this box!), while its bounds would be
      [(0,0,0)-(4,4,4)]. Make sure to NOT use box.size() for the
      grid dimensions, since this will always be one lower than
      the dims of the grid.... */
  ospcommon::box3i box;
  //! level this brick is at
  int level;
  // width of each cell in this level
  float cellWidth;

  // inline ospcommon::box3f worldBounds() const {
  //  return ospcommon::box3f(ospcommon::vec3f(box.lower)*cellWidth,
  //        ospcommon::vec3f(box.upper+ospcommon::vec3i(1))*cellWidth);
  //}
};
}
}

using namespace ospray::amr;

vtkStandardNewMacro(vtkOSPRayAMRVolumeMapperNode);

//----------------------------------------------------------------------------
vtkOSPRayAMRVolumeMapperNode::vtkOSPRayAMRVolumeMapperNode()
{
  this->NumColors = 128;
  this->TransferFunction = nullptr;
  this->SamplingRate = 0.5f;
  this->OldSamplingRate = -1.f;
}

//----------------------------------------------------------------------------
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
      return;

    if (!this->TransferFunction)
    {
      this->TransferFunction = ospNewTransferFunction("piecewise_linear");
    }

    this->Cache->SetSize(vtkOSPRayRendererNode::GetTimeCacheSize(ren));

    OSPModel OSPRayModel = orn->GetOModel();
    if (!OSPRayModel)
    {
      return;
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
      double tstep = vtkOSPRayRendererNode::GetViewTime(ren);
      auto cached_Volume = this->Cache->Get(tstep);
      if (cached_Volume)
      {
        this->OSPRayVolume = static_cast<OSPVolume>(cached_Volume->object);
      }
      else
      {
        if (this->Cache->GetSize() == 0)
        {
          ospRelease(this->OSPRayVolume);
        }
        this->OSPRayVolume = ospNewVolume("amr_volume");
        if (this->Cache->HasRoom())
        {
          auto cacheEntry = std::make_shared<vtkOSPRayCacheItemObject>(backend, this->OSPRayVolume);
          this->Cache->Set(tstep, cacheEntry);
        }
        volDirty = true;

        unsigned int lastLevel = 0;
        std::vector<OSPData> brickDataArray;
        std::vector<BrickInfo> brickInfoArray;
        size_t totalDataSize = 0;

        vtkAMRInformation* amrInfo = amr->GetAMRInfo();
        vtkSmartPointer<vtkUniformGridAMRDataIterator> iter;
        iter.TakeReference(vtkUniformGridAMRDataIterator::SafeDownCast(amr->NewIterator()));
        for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
        {
          unsigned int level = iter->GetCurrentLevel();
          if (!(level >= lastLevel))
          {
            vtkErrorMacro("ospray requires level info be ordered lowest to highest");
          };
          lastLevel = level;
          unsigned int index = iter->GetCurrentIndex();

          // note: this iteration "naturally" goes from datasets at lower levels to
          // those at higher levels.
          vtkImageData* data = vtkImageData::SafeDownCast(iter->GetCurrentDataObject());
          if (!data)
          {
            return;
          }
          float* dataPtr;
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

          if (cellArray->GetDataType() != VTK_FLOAT)
          {
            if (cellArray->GetDataType() == VTK_DOUBLE)
            {
              float* fdata = new float[dim[0] * dim[1] * size_t(dim[2])];
              double* dptr;
              dptr = (double*)cellArray->WriteVoidPointer(0, cellArray->GetSize());
              for (size_t i = 0; i < dim[0] * dim[1] * size_t(dim[2]); i++)
              {
                fdata[i] = dptr[i];
              }
              dataPtr = fdata;
            }
            else
            {
              std::cerr
                << "Only doubles and floats are supported in OSPRay AMR volume mapper currently";
              return;
            }
          }
          else
          {
            dataPtr = (float*)cellArray->WriteVoidPointer(0, cellArray->GetSize());
          }

          totalDataSize += sizeof(float) * size_t(dim[0] * dim[1]) * dim[2];
          OSPData odata =
            ospNewData(dim[0] * dim[1] * dim[2], OSP_FLOAT, dataPtr, OSP_DATA_SHARED_BUFFER);
          brickDataArray.push_back(odata);

          BrickInfo bi;
          ospcommon::box3i obox = { lo_v, hi_v };
          bi.box = obox;
          double spacing[3];
          amrInfo->GetSpacing(level, spacing);
          bi.cellWidth = spacing[0];
          // cell bounds:  origin + box.LoCorner*spacing,
          bi.level = level;
          totalDataSize += sizeof(BrickInfo);

          brickInfoArray.push_back(bi);
        }

        assert(brickDataArray.size() == brickInfoArray.size());
        ospSet1f(this->OSPRayVolume, "samplingRate", this->SamplingRate); // TODO: gui option

        double origin[3];
        vol->GetOrigin(origin);
        double* bds = mapper->GetBounds();
        origin[0] = bds[0];
        origin[1] = bds[2];
        origin[2] = bds[4];

        double spacing[3];
        amr->GetAMRInfo()->GetSpacing(0, spacing);
        ospSet3f(this->OSPRayVolume, "gridOrigin", origin[0], origin[1], origin[2]);
        ospSetString(this->OSPRayVolume, "voxelType", "float");

        OSPData brickDataData =
          ospNewData(brickDataArray.size(), OSP_OBJECT, &brickDataArray[0], 0);
        ospSetData(this->OSPRayVolume, "brickData", brickDataData);
        OSPData brickInfoData = ospNewData(
          brickInfoArray.size() * sizeof(brickInfoArray[0]), OSP_RAW, &brickInfoArray[0], 0);
        ospSetData(this->OSPRayVolume, "brickInfo", brickInfoData);
        ospSetObject(this->OSPRayVolume, "transferFunction", this->TransferFunction);
        this->BuildTime.Modified();
      }
    }
    if ((vol->GetProperty()->GetMTime() > this->PropertyTime) || volDirty)
    {
      //
      // transfer function
      //
      this->UpdateTransferFunction(backend, vol);
      ospSet1i(this->OSPRayVolume, "gradientShadingEnabled", volProperty->GetShade());
      this->PropertyTime.Modified();
    }

    if (this->OldSamplingRate != this->SamplingRate)
    {
      this->OldSamplingRate = this->SamplingRate;
      volDirty = true;
    }

    if (volDirty)
    {
      ospSet1f(this->OSPRayVolume, "samplingRate", this->SamplingRate);
      ospCommit(this->OSPRayVolume);
    }
    ospAddVolume(OSPRayModel, this->OSPRayVolume);
    ospCommit(OSPRayModel);
  } // prepass
}

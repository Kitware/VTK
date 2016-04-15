// /*=========================================================================

//   Program:   Visualization Toolkit
//   Module:    vtkOSPRayVolumeMapperNode.cxx

//   Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
//   All rights reserved.
//   See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

//      This software is distributed WITHOUT ANY WARRANTY; without even
//      the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//      PURPOSE.  See the above copyright notice for more information.

// =========================================================================*/
#include "vtkOSPRayVolumeMapperNode.h"

#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPointData.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"
#include "vtkColorTransferFunction.h"
#include "vtkAbstractVolumeMapper.h"

#include "vtkVolumeNode.h"
#include "vtkOSPRayRendererNode.h"

#include "ospray/ospray.h"
#include "ospray/version.h"

#include <map>
#include <math.h>
#include <algorithm>

//============================================================================
vtkStandardNewMacro(vtkOSPRayVolumeMapperNode);

//----------------------------------------------------------------------------
vtkOSPRayVolumeMapperNode::vtkOSPRayVolumeMapperNode()
{

  this->VolumeAdded=false;
  this->NumColors = 128;
  this->SampleDistance             =  1.0;
  this->ImageSampleDistance        =  1.0;
  this->MinimumImageSampleDistance =  1.0;
  this->MaximumImageSampleDistance = 10.0;
  this->AutoAdjustSampleDistances  =  1;

  this->ZBuffer                = NULL;

  this->IntermixIntersectingGeometry = 1;

  this->SharedData = false;
  if (SharedData)
    OSPRayVolume = ospNewVolume("shared_structured_volume");
  else
    OSPRayVolume = ospNewVolume("block_bricked_volume");
  transferFunction = ospNewTransferFunction("piecewise_linear");
  ospCommit(transferFunction);
  SamplingRate=0.0;
}

//----------------------------------------------------------------------------
vtkOSPRayVolumeMapperNode::~vtkOSPRayVolumeMapperNode()
{
}

//----------------------------------------------------------------------------
void vtkOSPRayVolumeMapperNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkOSPRayVolumeMapperNode::Render(bool prepass)
{
  //Carson:  TODO: this appears to be rendering twice on every update.
  //does not update with changes to underlying size
  if (prepass)
    {
    vtkVolumeNode* volNode = vtkVolumeNode::SafeDownCast(this->Parent);
    vtkVolume* vol = vtkVolume::SafeDownCast(volNode->GetRenderable());
    vtkAbstractVolumeMapper* mapper = vtkAbstractVolumeMapper::SafeDownCast(this->GetRenderable());
    if (!vol->GetProperty())
      {
      std::cerr << "vol had no property\n";
      return;
      }
    if (vol->GetVisibility() == false)
      {
      return;
      }

    vtkOSPRayRendererNode *orn =
      static_cast<vtkOSPRayRendererNode *>(
        this->GetFirstAncestorOfType("vtkOSPRayRendererNode"));

    //if there are no changes, just reuse last result
    unsigned long inTime = volNode->GetMTime();
    if (this->RenderTime >= inTime)
      {
      //   this->AddVolumesToModel(orn->GetOModel());
      //   return;
      }

    this->RenderTime = inTime;

    //something changed so make new meshes
    // this->CreateNewVolumes();

    osp::Model* OSPRayModel = orn->GetOModel();


    // make sure that we have scalar input and update the scalar input
    if ( mapper->GetDataSetInput() == NULL )
      {
      vtkErrorMacro(<< "No Input!");
      return;
      }
    else
      {
      mapper->GetInputAlgorithm()->UpdateInformation();
      // vtkStreamingDemandDrivenPipeline::SetUpdateExtentToWholeExtent(
      //   this->GetInputInformation());
      mapper->GetInputAlgorithm()->Update();
      }

    vtkImageData *data = vtkImageData::SafeDownCast(mapper->GetDataSetInput());
    if (!data)
      {
      cerr << "could not get data";
      return;
      }
    // vtkDataArray * scalars = mapper->GetScalars(data, mapper->GetScalarMode(),
    // mapper->GetArrayAccessMode(), mapper->GetArrayId(), mapper->GetArrayName(), mapper->GetCellFlag());

    void* ScalarDataPointer =
      mapper->GetDataSetInput()->GetPointData()->GetScalars()->GetVoidPointer(0);
    int ScalarDataType =
    mapper->GetDataSetInput()->GetPointData()->GetScalars()->GetDataType();

    int dim[3];
    data->GetDimensions(dim);

    size_t typeSize = 0;
    std::string voxelType;
    if (ScalarDataType == VTK_FLOAT)
      {
      typeSize = sizeof(float);
      voxelType = "float";
      }
    else if (ScalarDataType == VTK_UNSIGNED_CHAR)
      {
      typeSize = sizeof(unsigned char);
      voxelType = "uchar";
      }
    else if (ScalarDataType == VTK_DOUBLE)
      {
      typeSize = sizeof(double);
      voxelType = "double";
      }
    else
      {
      std::cerr << "ERROR: Unsupported data type for ospray volumes, current supported data types are: "
                << " float, uchar, double\n";
      return;
      }

    //
    // Cache timesteps
    //
    double timestep=-1;
    vtkInformation *inputInfo = mapper->GetDataSetInput()->GetInformation();
    if (inputInfo && inputInfo->Has(vtkDataObject::DATA_TIME_STEP()))
      {
      timestep = inputInfo->Get(vtkDataObject::DATA_TIME_STEP());
      }
    vtkOSPRayVolumeCacheEntry* cacheEntry = 0;
    //Carson:  caching disabled for now, need to check for updated input //Cache[vol][timestep];
    if (!cacheEntry)
      {
      cacheEntry = new vtkOSPRayVolumeCacheEntry();
      if (SharedData)
        {
        OSPRayVolume = ospNewVolume("shared_structured_volume");
        }
      else
        {
        OSPRayVolume = ospNewVolume("block_bricked_volume");
        }
      cacheEntry->Volume = OSPRayVolume;
      Cache[vol][timestep] = cacheEntry;

      //
      // Send Volumetric data to OSPRay
      //

      char* buffer = NULL;
      size_t sizeBytes = dim[0]*dim[1]*dim[2] *typeSize;

      buffer = (char*)ScalarDataPointer;
      ospSet3i(OSPRayVolume, "dimensions", dim[0], dim[1], dim[2]);
      double origin[3];
      double scale[3];
      vol->GetOrigin(origin);
      vol->GetScale(scale);
      double *bds = vol->GetBounds();
      std::cerr << "bounds: " << bds[0] << " " << bds[1] << " " << bds[2] << " "
       << bds[3] << " " << bds[4] << " " << bds[5] << std::endl;
      origin[0] = bds[0];
      origin[1] = bds[2];
      origin[2] = bds[4];

      double spacing[3];
      data->GetSpacing(spacing);
      // int extent[3];
      // data->GetExtent(extent);  Carson: For some reason this call makes the volume disappear...
      scale[0] = (bds[1]-bds[0])/double(dim[0]-1);
      scale[1] = (bds[3]-bds[2])/double(dim[1]-1);
      scale[2] = (bds[5]-bds[4])/double(dim[2]-1);

      ospSet3f(OSPRayVolume, "gridOrigin", origin[0], origin[1], origin[2]);
      ospSet3f(OSPRayVolume, "gridSpacing", spacing[0]*scale[0],spacing[1]*scale[1],spacing[2]*scale[2]);
      ospSetString(OSPRayVolume, "voxelType", voxelType.c_str());
      if (SharedData)
        {
        OSPData voxelData = ospNewData(sizeBytes, OSP_UCHAR, ScalarDataPointer, OSP_DATA_SHARED_BUFFER);
        ospSetData(OSPRayVolume, "voxelData", voxelData);
        }
      else
        {
        osp::vec3i ll, uu;
        ll.x = 0, ll.y = 0, ll.z = 0;
        uu.x = dim[0], uu.y = dim[1], uu.z = dim[2];
        ospSetRegion(OSPRayVolume, ScalarDataPointer, ll, uu);
        }
      }
    OSPRayVolume = cacheEntry->Volume;

    // test for modifications to volume properties
    if (vol->GetProperty()->GetMTime() > PropertyTime)
      {
      vtkVolumeProperty* volProperty = vol->GetProperty();
      vtkColorTransferFunction* colorTF = volProperty->GetRGBTransferFunction(0);
      vtkPiecewiseFunction *scalarTF = volProperty->GetScalarOpacity(0);
      int numNodes = colorTF->GetSize();
      double* tfData = colorTF->GetDataPointer();

      TFVals.resize(NumColors*3);
      TFOVals.resize(NumColors);
      scalarTF->GetTable(data->GetScalarRange()[0],data->GetScalarRange()[1], NumColors, &TFOVals[0]);
      colorTF->GetTable(data->GetScalarRange()[0],data->GetScalarRange()[1], NumColors, &TFVals[0]);

      OSPData colorData = ospNewData(NumColors, OSP_FLOAT3, &TFVals[0]);// TODO: memory leak?  does ospray manage this>
      ospSetData(transferFunction, "colors", colorData);
      OSPData tfAlphaData = ospNewData(NumColors, OSP_FLOAT, &TFOVals[0]);
      ospSetData(transferFunction, "opacities", tfAlphaData);

      ospCommit(transferFunction);
      ospSet1i(OSPRayVolume, "gradientShadingEnabled", volProperty->GetShade());
      PropertyTime.Modified();
      }

    // test for modifications to input
    if (mapper->GetDataSetInput()->GetMTime() > this->BuildTime)
      {
      ospSet2f(transferFunction, "valueRange", data->GetScalarRange()[0], data->GetScalarRange()[1]);

      //! Commit the transfer function only after the initial colors and alphas have been set (workaround for Qt signalling issue).
      ospCommit(transferFunction);

      //TODO: manage memory
      }
    ospSetObject((OSPObject)OSPRayVolume, "transferFunction", transferFunction);
    this->BuildTime.Modified();

    if (SamplingRate == 0.0f)
      {
      //automatically determine sampling rate, for now just a simple switch
      int maxBound = std::max(dim[0],dim[1]);
      maxBound = std::max(maxBound,dim[2]);
      if (maxBound < 1000)
        {
        float s = 1000.0f - maxBound;
        s = (s/1000.0f*4.0f + 0.25f);
        ospSet1f(OSPRayVolume, "samplingRate", s);
        }
      else
        {
        ospSet1f(OSPRayVolume, "samplingRate", 0.25f);
        }
      }
    else
      {
      ospSet1f(OSPRayVolume, "samplingRate", SamplingRate);
      }

    ospCommit(OSPRayVolume);
    ospAddVolume(OSPRayModel,(OSPVolume)OSPRayVolume);
    ospCommit(OSPRayModel);
  }
}

/*=========================================================================

   Program:   Visualization Toolkit
   Module:    vtkOSPRayVolumeMapperNode.cxx

   Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
   All rights reserved.
   See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

      This software is distributed WITHOUT ANY WARRANTY; without even
      the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
      PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkOSPRayVolumeMapperNode.h"

#include "vtkAbstractVolumeMapper.h"
#include "vtkCellData.h"
#include "vtkColorTransferFunction.h"
#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkOSPRayRendererNode.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPointData.h"
#include "vtkVolume.h"
#include "vtkVolumeNode.h"
#include "vtkVolumeProperty.h"

#include <algorithm>

#include "ospray/ospray.h"

//============================================================================
vtkStandardNewMacro(vtkOSPRayVolumeMapperNode);

//----------------------------------------------------------------------------
vtkOSPRayVolumeMapperNode::vtkOSPRayVolumeMapperNode()
{
  this->SamplingRate=0.0;
  this->NumColors = 128;
  this->OSPRayVolume = NULL;
  this->TransferFunction = NULL;
}

//----------------------------------------------------------------------------
vtkOSPRayVolumeMapperNode::~vtkOSPRayVolumeMapperNode()
{
  delete this->OSPRayVolume;
  ospRelease(this->TransferFunction);
}

//----------------------------------------------------------------------------
void vtkOSPRayVolumeMapperNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkOSPRayVolumeMapperNode::Render(bool prepass)
{
  if (prepass)
  {
    vtkVolumeNode* volNode = vtkVolumeNode::SafeDownCast(this->Parent);
    vtkVolume* vol = vtkVolume::SafeDownCast(volNode->GetRenderable());
    if (vol->GetVisibility() == false)
    {
      return;
    }
    vtkAbstractVolumeMapper* mapper = vtkAbstractVolumeMapper::SafeDownCast(this->GetRenderable());
    if (!vol->GetProperty())
    {
      // this is OK, happens in paraview client side for instance
      return;
    }

    vtkOSPRayRendererNode *orn =
      static_cast<vtkOSPRayRendererNode *>(
        this->GetFirstAncestorOfType("vtkOSPRayRendererNode"));

    osp::Model* OSPRayModel = orn->GetOModel();

    // make sure that we have scalar input and update the scalar input
    if ( mapper->GetDataSetInput() == NULL )
    {
      //OK - PV cli/srv for instance vtkErrorMacro("VolumeMapper had no input!");
      return;
    }
    mapper->GetInputAlgorithm()->UpdateInformation();
    mapper->GetInputAlgorithm()->Update();

    vtkImageData *data = vtkImageData::SafeDownCast(mapper->GetDataSetInput());
    if (!data)
    {
      vtkErrorMacro("VolumeMapper's Input has no data!");
      return;
    }
    vtkDataArray *sa = mapper->GetDataSetInput()->GetPointData()->GetScalars();
    bool onPoints = true;
    if (!sa)
    {
      onPoints = false;
      sa = mapper->GetDataSetInput()->GetCellData()->GetScalars();
    }
    if (!sa)
    {
      vtkErrorMacro("VolumeMapper's Input has no scalar array!");
      return;
    }
    int ScalarDataType = sa->GetDataType();
    void* ScalarDataPointer = sa->GetVoidPointer(0);
    int dim[3];
    data->GetDimensions(dim);
    if (!onPoints)
    {
      dim[0] = dim[0]-1;
      dim[1] = dim[1]-1;
      dim[2] = dim[2]-1;
    }

    std::string voxelType;
    if (ScalarDataType == VTK_FLOAT)
    {
      voxelType = "float";
    }
    else if (ScalarDataType == VTK_UNSIGNED_CHAR)
    {
      voxelType = "uchar";
    }
    else if (ScalarDataType == VTK_DOUBLE)
    {
      voxelType = "double";
    }
    else
    {
      vtkErrorMacro("ERROR: Unsupported data type for ospray volumes, current supported data types are: float, uchar, and double.");
      return;
    }

    if (!this->TransferFunction)
    {
      this->TransferFunction = ospNewTransferFunction("piecewise_linear");
    }

    // when input data is modified
    if (mapper->GetDataSetInput()->GetMTime() > this->BuildTime)
    {
      delete this->OSPRayVolume;
      this->OSPRayVolume = ospNewVolume("block_bricked_volume");

      //
      // Send Volumetric data to OSPRay
      //
      ospSet3i(this->OSPRayVolume, "dimensions", dim[0], dim[1], dim[2]);
      double origin[3];
      double scale[3];
      data->GetOrigin(origin);
      vol->GetScale(scale);
      double *bds = vol->GetBounds();
      origin[0] = bds[0];
      origin[1] = bds[2];
      origin[2] = bds[4];

      double spacing[3];
      data->GetSpacing(spacing);
      scale[0] = (bds[1]-bds[0])/double(dim[0]-1);
      scale[1] = (bds[3]-bds[2])/double(dim[1]-1);
      scale[2] = (bds[5]-bds[4])/double(dim[2]-1);

      ospSet3f(this->OSPRayVolume, "gridOrigin", origin[0], origin[1], origin[2]);
      ospSet3f(this->OSPRayVolume, "gridSpacing", scale[0], scale[1], scale[2]);
      ospSetString(this->OSPRayVolume, "voxelType", voxelType.c_str());

      osp::vec3i ll, uu;
      ll.x = 0, ll.y = 0, ll.z = 0;
      uu.x = dim[0], uu.y = dim[1], uu.z = dim[2];
      ospSetRegion(this->OSPRayVolume, ScalarDataPointer, ll, uu);

      ospSet2f(this->TransferFunction, "valueRange",
               data->GetScalarRange()[0], data->GetScalarRange()[1]);
    }

    // test for modifications to volume properties
    vtkVolumeProperty* volProperty = vol->GetProperty();
    if (vol->GetProperty()->GetMTime() > this->PropertyTime
        || mapper->GetDataSetInput()->GetMTime() > this->BuildTime)
    {
      vtkColorTransferFunction* colorTF =
        volProperty->GetRGBTransferFunction(0);
      vtkPiecewiseFunction *scalarTF = volProperty->GetScalarOpacity(0);

      this->TFVals.resize(this->NumColors*3);
      this->TFOVals.resize(this->NumColors);
      scalarTF->GetTable(data->GetScalarRange()[0],
                         data->GetScalarRange()[1],
                         this->NumColors,
                         &TFOVals[0]);
      colorTF->GetTable(data->GetScalarRange()[0],
                        data->GetScalarRange()[1],
                        this->NumColors,
                        &this->TFVals[0]);

      OSPData colorData = ospNewData(this->NumColors,
                                     OSP_FLOAT3,
                                     &this->TFVals[0]);
      ospSetData(this->TransferFunction, "colors", colorData);

      OSPData tfAlphaData = ospNewData(NumColors, OSP_FLOAT, &TFOVals[0]);
      ospSetData(this->TransferFunction, "opacities", tfAlphaData);

      ospSetObject(this->OSPRayVolume, "transferFunction",
                   this->TransferFunction);

      ospSet1i(this->OSPRayVolume, "gradientShadingEnabled",
               volProperty->GetShade());
      PropertyTime.Modified();
      ospRelease(colorData);
      ospRelease(tfAlphaData);
    }

    if (this->SamplingRate == 0.0f)  // 0 means automatic sampling rate
    {
      //automatically determine sampling rate
      int maxBound = std::max(dim[0],dim[1]);
      maxBound = std::max(maxBound,dim[2]);
      if (maxBound < 1000)
      {
        float s = 1000.0f - maxBound;
        s = (s/1000.0f*4.0f + 0.25f);
        ospSet1f(this->OSPRayVolume, "samplingRate", s);
      }
      else
      {
        ospSet1f(this->OSPRayVolume, "samplingRate", 0.25f);
      }
    }
    else
    {
      ospSet1f(this->OSPRayVolume, "samplingRate", this->SamplingRate);
    }

    this->RenderTime = volNode->GetMTime();
    this->BuildTime.Modified();

    ospCommit(this->TransferFunction);
    ospCommit(this->OSPRayVolume);
    ospAddVolume(OSPRayModel, this->OSPRayVolume);
    //ospCommit(OSPRayModel);
  }
}

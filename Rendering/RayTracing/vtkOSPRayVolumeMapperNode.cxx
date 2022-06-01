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

#include "vtkCellData.h"
#include "vtkColorTransferFunction.h"
#include "vtkContourValues.h"
#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkMath.h"
#include "vtkOSPRayCache.h"
#include "vtkOSPRayMaterialHelpers.h"
#include "vtkOSPRayRendererNode.h"
#include "vtkObjectFactory.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPointData.h"
#include "vtkRenderer.h"
#include "vtkVolume.h"
#include "vtkVolumeMapper.h"
#include "vtkVolumeNode.h"
#include "vtkVolumeProperty.h"

#include <algorithm>
#include <vector>

//============================================================================
vtkStandardNewMacro(vtkOSPRayVolumeMapperNode);

//------------------------------------------------------------------------------
vtkOSPRayVolumeMapperNode::vtkOSPRayVolumeMapperNode()
{
  this->SamplingRate = 0.0;
  this->SamplingStep = 1.0;
  this->NumColors = 128;

  this->OSPRayVolume = nullptr;
  this->OSPRayVolumeModel = nullptr;
  this->Cropper = nullptr;
  this->TransferFunction = nullptr;
  this->OSPRayInstance = nullptr;

  this->LastArray = nullptr;
  this->LastComponent = -1;

  this->Cache = new vtkOSPRayCache<vtkOSPRayCacheItemObject>;
}

//------------------------------------------------------------------------------
vtkOSPRayVolumeMapperNode::~vtkOSPRayVolumeMapperNode()
{
  vtkOSPRayRendererNode* orn =
    static_cast<vtkOSPRayRendererNode*>(this->GetFirstAncestorOfType("vtkOSPRayRendererNode"));
  if (orn)
  {
    RTW::Backend* backend = orn->GetBackend();
    if (backend != nullptr)
    {
      ospRelease(this->OSPRayVolume);
      ospRelease(this->OSPRayVolumeModel);
      ospRelease(this->Cropper);
      ospRelease(this->TransferFunction);
      ospRelease(this->OSPRayInstance);
    }
  }
}

//------------------------------------------------------------------------------
void vtkOSPRayVolumeMapperNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkOSPRayVolumeMapperNode::Render(bool prepass)
{
  vtkOSPRayRendererNode* orn =
    static_cast<vtkOSPRayRendererNode*>(this->GetFirstAncestorOfType("vtkOSPRayRendererNode"));
  if (prepass)
  {
    vtkVolumeNode* volNode = vtkVolumeNode::SafeDownCast(this->Parent);
    vtkVolume* vol = vtkVolume::SafeDownCast(volNode->GetRenderable());
    if (vol->GetVisibility() == false)
    {
      return;
    }
    vtkVolumeMapper* mapper = vtkVolumeMapper::SafeDownCast(this->GetRenderable());
    if (!vol->GetProperty())
    {
      // this is OK, happens in paraview client side for instance
      return;
    }

    vtkRenderer* ren = vtkRenderer::SafeDownCast(orn->GetRenderable());
    RTW::Backend* backend = orn->GetBackend();
    if (backend == nullptr)
    {
      return;
    }

    // make sure that we have scalar input and update the scalar input
    if (mapper->GetDataSetInput() == nullptr)
    {
      // OK - PV cli/srv for instance vtkErrorMacro("VolumeMapper had no input!");
      return;
    }
    mapper->GetInputAlgorithm()->UpdateInformation();
    mapper->GetInputAlgorithm()->Update();

    vtkImageData* data = vtkImageData::SafeDownCast(mapper->GetDataSetInput());
    if (!data)
    {
      return;
    }

    int fieldAssociation;
    vtkDataArray* sa = vtkDataArray::SafeDownCast(this->GetArrayToProcess(data, fieldAssociation));
    if (!sa)
    {
      // OK - PV cli/srv for instance vtkErrorMacro("VolumeMapper's Input has no scalar array!");
      return;
    }

    if (!this->TransferFunction)
    {
      this->TransferFunction = ospNewTransferFunction("piecewiseLinear");
    }

    vtkVolumeProperty* volProperty = vol->GetProperty();
    // when input data is modified
    vtkDataArray* componentArray = nullptr;
    int comp = volProperty->GetRGBTransferFunction()->GetVectorComponent();
    if (mapper->GetDataSetInput()->GetMTime() > this->BuildTime || sa != LastArray ||
      comp != LastComponent)
    {
      LastArray = sa;
      LastComponent = comp;
      int ncomp = sa->GetNumberOfComponents();
      if (ncomp > 1)
      {
        componentArray = sa->NewInstance();
        componentArray->SetNumberOfComponents(1);
        componentArray->SetNumberOfTuples(sa->GetNumberOfTuples());
        if (volProperty->GetRGBTransferFunction()->GetVectorMode() !=
          vtkColorTransferFunction::MAGNITUDE)
        {
          componentArray->CopyComponent(0, sa, comp);
        }
        else
        {
          for (vtkIdType t = 0; t < sa->GetNumberOfTuples(); ++t)
          {
            componentArray->SetTuple1(t, vtkMath::Norm(sa->GetTuple3(t)));
          }
        }
        sa = componentArray;
      }
      int ScalarDataType = sa->GetDataType();
      void* ScalarDataPointer = sa->GetVoidPointer(0);
      int dim[3];
      data->GetDimensions(dim);
      if (fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_CELLS)
      {
        dim[0] = dim[0] - 1;
        dim[1] = dim[1] - 1;
        dim[2] = dim[2] - 1;
      }

      OSPDataType ospVoxelType = OSP_UNKNOWN;
      if (ScalarDataType == VTK_FLOAT)
      {
        ospVoxelType = OSP_FLOAT;
      }
      else if (ScalarDataType == VTK_UNSIGNED_CHAR)
      {
        ospVoxelType = OSP_UCHAR;
      }
      else if (ScalarDataType == VTK_UNSIGNED_SHORT)
      {
        ospVoxelType = OSP_USHORT;
      }
      else if (ScalarDataType == VTK_SHORT)
      {
        ospVoxelType = OSP_SHORT;
      }
      else if (ScalarDataType == VTK_DOUBLE)
      {
        ospVoxelType = OSP_DOUBLE;
      }
      else
      {
        vtkErrorMacro("ERROR: Unsupported data type for ospray volumes, current supported data "
                      "types are: float, uchar, short, ushort, and double.");
        return;
      }

      ospRelease(this->OSPRayVolume);
      this->OSPRayVolume = ospNewVolume("structuredRegular");
      //
      // Send Volumetric data to OSPRay
      //
      double origin[3];
      double scale[3];
      data->GetOrigin(origin);
      vol->GetScale(scale);
      const double* bds = vol->GetBounds();
      origin[0] = bds[0];
      origin[1] = bds[2];
      origin[2] = bds[4];

      double spacing[3];
      data->GetSpacing(spacing);
      scale[0] = (bds[1] - bds[0]) / double(dim[0] - 1);
      scale[1] = (bds[3] - bds[2]) / double(dim[1] - 1);
      scale[2] = (bds[5] - bds[4]) / double(dim[2] - 1);

      ospSetVec3f(this->OSPRayVolume, "gridOrigin", origin[0], origin[1], origin[2]);
      ospSetVec3f(this->OSPRayVolume, "gridSpacing", scale[0], scale[1], scale[2]);
      this->SamplingStep = std::min(scale[0], std::min(scale[1], scale[2]));

      OSPData volData;
      if (!componentArray)
      {
        volData = ospNewSharedData3D(ScalarDataPointer, ospVoxelType, dim[0], dim[1], dim[2]);
      }
      else
      {
        volData = ospNewCopyData3D(ScalarDataPointer, ospVoxelType, dim[0], dim[1], dim[2]);
      }
      ospCommit(volData);
      ospSetObject(this->OSPRayVolume, "data", volData);

      ospCommit(this->OSPRayVolume);
      ospRelease(volData);
    }

    if (mapper->GetMTime() > this->BuildTime)
    {
      if (mapper->GetCropping())
      {
        double planes[6];
        mapper->GetCroppingRegionPlanes(planes);
        OSPGeometry clipBox = ospNewGeometry("box");
        this->Cropper = ospNewGeometricModel(clipBox);
        std::vector<osp::box3f> boxes = { osp::box3f{
          osp::vec3f{ static_cast<float>(planes[0]), static_cast<float>(planes[2]),
            static_cast<float>(planes[4]) },
          osp::vec3f{ static_cast<float>(planes[1]), static_cast<float>(planes[3]),
            static_cast<float>(planes[5]) } } };
        OSPData boundsData = ospNewCopyData1D(&boxes[0], OSP_BOX3F, 1);
        ospCommit(boundsData);
        ospSetObject(clipBox, "box", boundsData);
        ospCommit(clipBox);
        ospSetBool(this->Cropper, "invertNormals", true);
        ospCommit(this->Cropper);
        ospRelease(clipBox);
        ospRelease(boundsData);
      }
      else
      {
        ospRelease(this->Cropper);
        this->Cropper = nullptr;
      }
    }

    // test for modifications to volume properties
    if (volProperty->GetMTime() > this->PropertyTime ||
      mapper->GetDataSetInput()->GetMTime() > this->BuildTime)
    {
      this->UpdateTransferFunction(backend, vol, sa->GetRange());
    }

    ospRelease(this->OSPRayVolumeModel);
    this->OSPRayVolumeModel = ospNewVolumetricModel(this->OSPRayVolume);
    ospSetObject(this->OSPRayVolumeModel, "transferFunction", this->TransferFunction);
    const float densityScale = 1.0f / volProperty->GetScalarOpacityUnitDistance();
    ospSetFloat(this->OSPRayVolumeModel, "densityScale", densityScale);
    const float anisotropy = volProperty->GetScatteringAnisotropy();
    ospSetFloat(this->OSPRayVolumeModel, "anisotropy", anisotropy);
    // todo: unhardcode gradientshadingScale value when enabled
    ospSetFloat(
      this->OSPRayVolumeModel, "gradientShadingScale", volProperty->GetShade() ? 0.5 : 0.0);
    ospCommit(this->OSPRayVolumeModel);

    this->RenderTime = volNode->GetMTime();
    this->BuildTime.Modified();

    vtkContourValues* contours = volProperty->GetIsoSurfaceValues();
    if (mapper->GetBlendMode() == vtkVolumeMapper::ISOSURFACE_BLEND)
    {
      int nbContours = contours->GetNumberOfContours();
      if (nbContours > 0)
      {
        double* p = contours->GetValues();
        std::vector<float> values(p, p + nbContours);

        OSPGeometry OSPRayIsosurface = ospNewGeometry("isosurface");
        OSPData isosurfaces = ospNewCopyData1D(values.data(), OSP_FLOAT, values.size());
        ospCommit(isosurfaces);

        ospSetObject(OSPRayIsosurface, "isovalue", isosurfaces);
        ospSetObject(OSPRayIsosurface, "volume", this->OSPRayVolumeModel);
        ospCommit(OSPRayIsosurface);
        ospRelease(isosurfaces);

        OSPGroup group = ospNewGroup();
        OSPInstance instance = ospNewInstance(group);

        OSPGeometricModel OSPRayGeometricModel = ospNewGeometricModel(OSPRayIsosurface);

        OSPMaterial material =
          vtkOSPRayMaterialHelpers::NewMaterial(orn, orn->GetORenderer(), "obj");
        ospCommit(material);
        ospSetObjectAsData(OSPRayGeometricModel, "material", OSP_MATERIAL, material);
        ospCommit(OSPRayGeometricModel);
        ospRelease(material);
        ospRelease(OSPRayIsosurface);

        OSPData instanceData = ospNewCopyData1D(&OSPRayGeometricModel, OSP_GEOMETRIC_MODEL, 1);
        ospCommit(instanceData);
        ospRelease(OSPRayGeometricModel);

        ospSetObject(group, "geometry", instanceData);
        ospCommit(group);
        ospCommit(instance);
        ospRelease(group);

        orn->Instances.emplace_back(instance);

        ospRelease(this->OSPRayInstance);
        this->OSPRayInstance = instance;
      }
      else
      {
        vtkWarningMacro("Isosurface mode is selected but no contour is defined");
      }
    }
    else
    {
      OSPGroup group = ospNewGroup();
      OSPInstance instance = ospNewInstance(group);
      OSPData instanceData = ospNewCopyData1D(&this->OSPRayVolumeModel, OSP_VOLUMETRIC_MODEL, 1);
      ospCommit(instanceData);
      ospSetObject(group, "volume", instanceData);
      if (this->Cropper)
      {
        ospSetObjectAsData(group, "clippingGeometry", OSP_GEOMETRIC_MODEL, this->Cropper);
      }
      ospCommit(group);
      ospCommit(instance);
      ospRelease(group);
      ospRelease(instanceData);
      orn->Instances.emplace_back(instance);
      ospRelease(this->OSPRayInstance);
      this->OSPRayInstance = instance;
    }

    if (componentArray)
    {
      componentArray->Delete();
    }
  }
}

//------------------------------------------------------------------------------
void vtkOSPRayVolumeMapperNode::UpdateTransferFunction(
  RTW::Backend* backend, vtkVolume* vol, double* dataRange)
{
  if (backend == nullptr)
    return;
  vtkVolumeProperty* volProperty = vol->GetProperty();
  vtkColorTransferFunction* colorTF = volProperty->GetRGBTransferFunction(0);
  vtkPiecewiseFunction* scalarTF = volProperty->GetScalarOpacity(0);

  this->TFVals.resize(this->NumColors * 3);
  this->TFOVals.resize(this->NumColors);
  double tfRangeD[2];
  // prefer transfer function's range
  colorTF->GetRange(tfRangeD);
  // but use data's range if we can and we have to
  if (dataRange && (dataRange[1] > dataRange[0]) && (tfRangeD[1] <= tfRangeD[0]))
  {
    tfRangeD[0] = dataRange[0];
    tfRangeD[1] = dataRange[1];
  }
  osp::vec2f tfRange = { float(tfRangeD[0]), float(tfRangeD[1]) };
  scalarTF->GetTable(tfRangeD[0], tfRangeD[1], this->NumColors, &this->TFOVals[0]);
  colorTF->GetTable(tfRangeD[0], tfRangeD[1], this->NumColors, &this->TFVals[0]);

  ospRelease(this->TransferFunction);
  this->TransferFunction = ospNewTransferFunction("piecewiseLinear");

  OSPData colorData = ospNewCopyData1D(&this->TFVals[0], OSP_VEC3F, this->NumColors);
  ospCommit(colorData);
  ospSetObject(this->TransferFunction, "color", colorData);

  ospSetVec2f(this->TransferFunction, "valueRange", tfRange.x, tfRange.y);

  OSPData tfAlphaData = ospNewCopyData1D(&this->TFOVals[0], OSP_FLOAT, this->NumColors);
  ospCommit(tfAlphaData);
  ospSetObject(this->TransferFunction, "opacity", tfAlphaData);

  ospCommit(this->TransferFunction);
  ospRelease(colorData);
  ospRelease(tfAlphaData);

  this->PropertyTime.Modified();
}

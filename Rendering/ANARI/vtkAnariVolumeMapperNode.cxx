// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAnariVolumeMapperNode.h"
#include "vtkAnariProfiling.h"
#include "vtkAnariSceneGraph.h"
#include "vtkAnariVolumeNode.h"

#include "vtkAbstractVolumeMapper.h"
#include "vtkArrayDispatch.h"
#include "vtkCellDataToPointData.h"
#include "vtkColorTransferFunction.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkPiecewiseFunction.h"
#include "vtkVolume.h"
#include "vtkVolumeNode.h"
#include "vtkVolumeProperty.h"

#include <anari/anari_cpp.hpp>
#include <anari/anari_cpp/ext/std.h>
#include <vector>

using vec3 = anari::std_types::vec3;

//============================================================================
namespace anari_structured
{
VTK_ABI_NAMESPACE_BEGIN
struct TransferFunction
{
  TransferFunction()
    : valueRange{ 0, 1 }
  {
  }

  std::vector<vec3> color;
  std::vector<float> opacity;
  float valueRange[2];
};

struct StructuredRegularSpatialFieldDataWorker
{
  StructuredRegularSpatialFieldDataWorker()
    : AnariDevice(nullptr)
    , AnariSpatialField(nullptr)
    , Dim(nullptr)
  {
  }

  //------------------------------------------------------------------------------
  template <typename ScalarArray>
  void operator()(ScalarArray* scalars)
  {
    if (this->AnariDevice == nullptr || this->AnariSpatialField == nullptr || this->Dim == nullptr)
    {
      vtkLogF(ERROR, "[ANARI::ERROR] %s\n",
        "StructuredRegularSpatialFieldDataWorker not properly initialized");
      return;
    }

    VTK_ASSUME(scalars->GetNumberOfComponents() == 1);
    const auto scalarRange = vtk::DataArrayValueRange<1>(scalars);

    auto dataArray =
      anari::newArray3D(this->AnariDevice, ANARI_FLOAT32, this->Dim[0], this->Dim[1], this->Dim[2]);
    {
      auto dataArrayPtr = anari::map<float>(this->AnariDevice, dataArray);
      int i = 0;

      for (auto val : scalarRange)
      {
        dataArrayPtr[i++] = static_cast<float>(val);
      }

      anari::unmap(this->AnariDevice, dataArray);
    }

    anari::setAndReleaseParameter(this->AnariDevice, this->AnariSpatialField, "data", dataArray);
  }

  anari::Device AnariDevice;
  anari::SpatialField AnariSpatialField;
  int* Dim;
};
VTK_ABI_NAMESPACE_END
} // namespace: anari_structured

VTK_ABI_NAMESPACE_BEGIN

class vtkAnariVolumeMapperNodeInternals
{
public:
  vtkAnariVolumeMapperNodeInternals(vtkAnariVolumeMapperNode*);
  ~vtkAnariVolumeMapperNodeInternals();

  void UpdateTransferFunction(vtkVolume*, double, double);
  vtkDataArray* ConvertScalarData(vtkDataArray*, int, int);

  void StageVolume();

  vtkTimeStamp BuildTime;
  vtkTimeStamp PropertyTime;

  std::string LastArrayName;
  int LastArrayComponent{ -2 };

  double DataTimeStep = std::numeric_limits<float>::quiet_NaN();
  std::string VolumeName;

  vtkAnariVolumeMapperNode* Owner{ nullptr };
  vtkAnariSceneGraph* AnariRendererNode{ nullptr };
  anari::Device AnariDevice{ nullptr };
  anari::Volume AnariVolume{ nullptr };
  std::unique_ptr<anari_structured::TransferFunction> TransferFunction;
};

//----------------------------------------------------------------------------
vtkAnariVolumeMapperNodeInternals::vtkAnariVolumeMapperNodeInternals(
  vtkAnariVolumeMapperNode* owner)
  : Owner(owner)
{
}

//----------------------------------------------------------------------------
vtkAnariVolumeMapperNodeInternals::~vtkAnariVolumeMapperNodeInternals()
{
  anari::retain(this->AnariDevice, this->AnariVolume);
  anari::retain(this->AnariDevice, this->AnariDevice);
}

//----------------------------------------------------------------------------
void vtkAnariVolumeMapperNodeInternals::StageVolume()
{
  vtkAnariProfiling startProfiling(
    "vtkAnariVolumeMapperNode::RenderVolumes", vtkAnariProfiling::GREEN);

  if (this->AnariRendererNode != nullptr)
  {
    this->AnariRendererNode->AddVolume(this->AnariVolume);
  }
}

//------------------------------------------------------------------------------
void vtkAnariVolumeMapperNodeInternals::UpdateTransferFunction(
  vtkVolume* vtkVol, double low, double high)
{
  this->TransferFunction.reset(new anari_structured::TransferFunction());
  vtkVolumeProperty* volProperty = vtkVol->GetProperty();
  const int transferFunctionMode = volProperty->GetTransferFunctionMode();

  if (transferFunctionMode == vtkVolumeProperty::TF_2D)
  {
    vtkWarningWithObjectMacro(
      this->Owner, << "ANARI currently doesn't support 2D transfer functions. "
                   << "Using default RGB and Scalar transfer functions.");
  }

  if (volProperty->HasGradientOpacity())
  {
    vtkWarningWithObjectMacro(this->Owner, << "ANARI currently doesn't support gradient opacity");
  }

  vtkColorTransferFunction* colorTF = volProperty->GetRGBTransferFunction(0);
  vtkPiecewiseFunction* opacityTF = volProperty->GetScalarOpacity(0);

  // Value Range
  double tfRange[2] = { 0, -1 };

  if (transferFunctionMode == vtkVolumeProperty::TF_1D)
  {
    double* tfRangePtr = colorTF->GetRange();
    tfRange[0] = tfRangePtr[0];
    tfRange[1] = tfRangePtr[1];
  }

  if (tfRange[1] <= tfRange[0])
  {
    tfRange[0] = low;
    tfRange[1] = high;
  }

  this->TransferFunction->valueRange[0] = static_cast<float>(tfRange[0]);
  this->TransferFunction->valueRange[1] = static_cast<float>(tfRange[1]);

  // Opacity
  int opacitySize = this->Owner->GetOpacitySize();
  this->TransferFunction->opacity.resize(opacitySize);
  opacityTF->GetTable(tfRange[0], tfRange[1], opacitySize, this->TransferFunction->opacity.data());

  // Color
  int colorSize = this->Owner->GetColorSize();
  std::vector<float> colorArray(colorSize * 3);
  colorTF->GetTable(tfRange[0], tfRange[1], colorSize, colorArray.data());

  for (int i = 0, j = 0; i < colorSize; i++, j += 3)
  {
    this->TransferFunction->color.emplace_back(
      vec3{ colorArray[j], colorArray[j + 1], colorArray[j + 2] });
  }
}

//------------------------------------------------------------------------------
vtkDataArray* vtkAnariVolumeMapperNodeInternals::ConvertScalarData(
  vtkDataArray* scalarData, int vectorComponent, int vectorMode)
{
  int numComponents = scalarData->GetNumberOfComponents();
  const vtkIdType numTuples = scalarData->GetNumberOfTuples();
  vtkDataArray* scalarDataOut = nullptr;

  if (numComponents > 1)
  {
    scalarDataOut = scalarData->NewInstance();
    scalarDataOut->SetNumberOfComponents(1);
    scalarDataOut->SetNumberOfTuples(numTuples);

    if (vectorMode != vtkColorTransferFunction::MAGNITUDE)
    {
      scalarDataOut->CopyComponent(0, scalarData, vectorComponent);
    }
    else
    {
      for (vtkIdType t = 0; t < numTuples; t++)
      {
        scalarDataOut->SetTuple1(t, vtkMath::Norm(scalarData->GetTuple3(t)));
      }
    }
  }

  return scalarDataOut;
}

//============================================================================
vtkStandardNewMacro(vtkAnariVolumeMapperNode);

//----------------------------------------------------------------------------
vtkAnariVolumeMapperNode::vtkAnariVolumeMapperNode()
{
  this->Internal = new vtkAnariVolumeMapperNodeInternals(this);
}

//----------------------------------------------------------------------------
vtkAnariVolumeMapperNode::~vtkAnariVolumeMapperNode()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkAnariVolumeMapperNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkAnariVolumeMapperNode::Synchronize(bool prepass)
{
  vtkAnariProfiling startProfiling(
    "vtkAnariVolumeMapperNode::Synchronize", vtkAnariProfiling::GREEN);

  if (!prepass)
  {
    return;
  }

  vtkVolumeNode* volNode = vtkVolumeNode::SafeDownCast(this->Parent);
  vtkVolume* vol = vtkVolume::SafeDownCast(volNode->GetRenderable());

  if (vol->GetVisibility() != false)
  {
    vtkVolumeProperty* const volumeProperty = vol->GetProperty();

    if (!volumeProperty)
    {
      // this is OK, happens in paraview client side for instance
      vtkDebugMacro(<< "Volume doesn't have property set");
      return;
    }

    vtkAbstractVolumeMapper* mapper = vtkAbstractVolumeMapper::SafeDownCast(this->GetRenderable());

    // make sure that we have scalar input and update the scalar input
    if (mapper->GetDataSetInput() == nullptr)
    {
      // OK - PV cli/srv for instance vtkErrorMacro("VolumeMapper had no input!");
      vtkDebugMacro(<< "No scalar input for the Volume");
      return;
    }

    mapper->GetInputAlgorithm()->UpdateInformation();
    mapper->GetInputAlgorithm()->Update();
    vtkDataSet* dataSet = mapper->GetDataSetInput();
    vtkImageData* data = vtkImageData::SafeDownCast(dataSet);

    if (!data)
    {
      vtkDebugMacro("VolumeMapper's Input has no data!");
      return;
    }

    int fieldAssociation;
    vtkDataArray* sa = vtkDataArray::SafeDownCast(this->GetArrayToProcess(data, fieldAssociation));

    if (!sa)
    {
      vtkErrorMacro("VolumeMapper's Input has no scalar array!");
      return;
    }

    const int vectorComponent = volumeProperty->GetRGBTransferFunction()->GetVectorComponent();
    const int vectorMode = volumeProperty->GetRGBTransferFunction()->GetVectorMode();

    vtkDataArray* sca = this->Internal->ConvertScalarData(sa, vectorComponent, vectorMode);

    if (sca != nullptr)
    {
      sa = sca;
    }

    this->Internal->AnariRendererNode =
      static_cast<vtkAnariSceneGraph*>(this->GetFirstAncestorOfType("vtkAnariSceneGraph"));
    auto anariDevice = this->Internal->AnariRendererNode->GetDeviceHandle();

    if (!this->Internal->AnariDevice)
    {
      this->Internal->AnariDevice = anariDevice;
      anari::retain(anariDevice, anariDevice);
    }

    //
    // Create ANARI Volume
    //

    vtkInformation* info = vol->GetPropertyKeys();
    if (info && info->Has(vtkAnariVolumeNode::VOLUME_NODE_NAME()))
    {
      this->Internal->VolumeName = info->Get(vtkAnariVolumeNode::VOLUME_NODE_NAME());
    }
    else
    {
      this->Internal->VolumeName =
        "vtk_volume_" + this->Internal->AnariRendererNode->ReservePropId();
    }

    if (this->Internal->AnariVolume == nullptr)
    {
      this->Internal->AnariVolume =
        anari::newObject<anari::Volume>(anariDevice, "transferFunction1D");

      std::string volumeName = this->Internal->VolumeName + "_volume";
      anari::setParameter(
        anariDevice, this->Internal->AnariVolume, "name", ANARI_STRING, volumeName.c_str());
    }

    auto anariVolume = this->Internal->AnariVolume;

    if (mapper->GetDataSetInput()->GetMTime() > this->Internal->BuildTime ||
      this->Internal->LastArrayName != mapper->GetArrayName() ||
      this->Internal->LastArrayComponent != vectorComponent)
    {
      this->Internal->LastArrayName = mapper->GetArrayName();
      this->Internal->LastArrayComponent = vectorComponent;

      // Spatial Field
      auto anariSpatialField =
        anari::newObject<anari::SpatialField>(anariDevice, "structuredRegular");

      std::string spatialFieldName = this->Internal->VolumeName + "_spatialfield";
      anari::setParameter(
        anariDevice, anariSpatialField, "name", ANARI_STRING, spatialFieldName.c_str());

      this->Internal->DataTimeStep = std::numeric_limits<float>::quiet_NaN();
      if (info && info->Has(vtkDataObject::DATA_TIME_STEP()))
      {
        this->Internal->DataTimeStep = info->Get(vtkDataObject::DATA_TIME_STEP());

        anari::setParameter(anariDevice, anariSpatialField, "usd::time", ANARI_FLOAT64,
          &this->Internal->DataTimeStep);
      }

      double origin[3];
      const double* bds = vol->GetBounds();
      origin[0] = bds[0];
      origin[1] = bds[2];
      origin[2] = bds[4];
      vec3 gridOrigin = { static_cast<float>(origin[0]), static_cast<float>(origin[1]),
        static_cast<float>(origin[2]) };
      anari::setParameter(anariDevice, anariSpatialField, "origin", gridOrigin);

      double spacing[3];
      data->GetSpacing(spacing);
      vec3 gridSpacing = { static_cast<float>(spacing[0]), static_cast<float>(spacing[1]),
        static_cast<float>(spacing[2]) };

      anari::setParameter(anariDevice, anariSpatialField, "spacing", gridSpacing);

      // Filter
      const int filterType = vol->GetProperty()->GetInterpolationType();

      if (filterType == VTK_LINEAR_INTERPOLATION)
      {
        anari::setParameter(anariDevice, anariSpatialField, "filter", "linear");
      }
      else if (filterType == VTK_NEAREST_INTERPOLATION)
      {
        anari::setParameter(anariDevice, anariSpatialField, "filter", "nearest");
      }
      else if (filterType == VTK_CUBIC_INTERPOLATION)
      {
        vtkWarningMacro(
          << "ANARI currently doesn't support cubic interpolation, using default value.");
      }
      else
      {
        vtkWarningMacro(<< "ANARI currently only supports linear and nearest interpolation, using "
                           "default value.");
      }

      int dim[3];
      data->GetDimensions(dim);

      if (fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_CELLS)
      {
        dim[0] -= 1;
        dim[1] -= 1;
        dim[2] -= 1;
      }

      vtkDebugMacro(<< "Volume Dimensions: " << dim[0] << "x" << dim[1] << "x" << dim[2]);

      // Create the actual field values for the 3D grid; the scalars are assumed to be
      // vertex centered.
      anari_structured::StructuredRegularSpatialFieldDataWorker worker;
      worker.AnariDevice = anariDevice;
      worker.AnariSpatialField = anariSpatialField;
      worker.Dim = dim;

      using Dispatcher = vtkArrayDispatch::DispatchByValueType<vtkTypeList::Create<double, float,
        int, unsigned int, char, unsigned char, unsigned short, short>>;

      if (!Dispatcher::Execute(sa, worker))
      {
        worker(sa);
      }

      anari::commitParameters(anariDevice, anariSpatialField);
      anari::setAndReleaseParameter(anariDevice, anariVolume, "value", anariSpatialField);
      anari::commitParameters(anariDevice, anariVolume);
    }

    if (volumeProperty->GetMTime() > this->Internal->PropertyTime ||
      mapper->GetDataSetInput()->GetMTime() > this->Internal->BuildTime)
    {
      // Transfer Function
      double scalarRange[2];
      sa->GetRange(scalarRange);

      this->Internal->UpdateTransferFunction(vol, scalarRange[0], scalarRange[1]);
      anari_structured::TransferFunction* transferFunction = this->Internal->TransferFunction.get();

      anari::setParameter(
        anariDevice, anariVolume, "valueRange", ANARI_FLOAT32_BOX1, &transferFunction->valueRange);

      auto array1DColor = anari::newArray1D(
        anariDevice, transferFunction->color.data(), transferFunction->color.size());
      anari::setAndReleaseParameter(anariDevice, anariVolume, "color", array1DColor);

      auto array1DOpacity = anari::newArray1D(
        anariDevice, transferFunction->opacity.data(), transferFunction->opacity.size());
      anari::setAndReleaseParameter(anariDevice, anariVolume, "opacity", array1DOpacity);

      anari::commitParameters(anariDevice, anariVolume);
      this->Internal->PropertyTime.Modified();
    }

    if (sca)
    {
      sca->Delete();
    }
  }
  else
  {
    vtkDebugMacro(<< "Volume visibility off");

    if (this->Internal->AnariVolume != nullptr)
    {
      this->Internal->AnariRendererNode =
        static_cast<vtkAnariSceneGraph*>(this->GetFirstAncestorOfType("vtkAnariSceneGraph"));
      auto anariDevice = this->Internal->AnariRendererNode->GetDeviceHandle();
      anari::release(anariDevice, this->Internal->AnariVolume);
      this->Internal->AnariVolume = nullptr;
    }
    else
    {
      return;
    }
  }

  this->RenderTime = volNode->GetMTime();
  this->Internal->BuildTime.Modified();
}

//----------------------------------------------------------------------------
void vtkAnariVolumeMapperNode::Render(bool prepass)
{
  vtkAnariProfiling startProfiling("vtkAnariVolumeMapperNode::Render", vtkAnariProfiling::GREEN);

  if (!prepass)
  {
    return;
  }

  this->Internal->StageVolume();
}

vtkVolume* vtkAnariVolumeMapperNode::GetVtkVolume() const
{
  return static_cast<vtkVolume*>(this->Renderable);
}

bool vtkAnariVolumeMapperNode::VolumeWasModified() const
{
  return this->RenderTime < GetVtkVolume()->GetMTime();
}

VTK_ABI_NAMESPACE_END

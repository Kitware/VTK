//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <viskores/rendering/CanvasRayTracer.h>
#include <viskores/rendering/ConnectivityProxy.h>
#include <viskores/rendering/Mapper.h>
#include <viskores/rendering/raytracing/ConnectivityTracer.h>
#include <viskores/rendering/raytracing/Logger.h>
#include <viskores/rendering/raytracing/RayOperations.h>

#include <memory>


namespace viskores
{
namespace rendering
{
struct ConnectivityProxy::InternalsType
{
protected:
  using ColorMapType = viskores::cont::ArrayHandle<viskores::Vec4f_32>;
  using TracerType = viskores::rendering::raytracing::ConnectivityTracer;

  TracerType Tracer;
  std::string CoordinateName;
  std::string FieldName;
  std::string EmissionFieldName;
  RenderMode Mode;
  viskores::Bounds SpatialBounds;
  ColorMapType ColorMap;
  viskores::cont::DataSet Dataset;
  viskores::Range ScalarRange;
  bool CompositeBackground;

public:
  InternalsType(const viskores::cont::DataSet& dataSet,
                const std::string& coordinateName,
                const std::string& fieldName)
  {
    Dataset = dataSet;
    CoordinateName = coordinateName;
    Mode = RenderMode::Volume;
    CompositeBackground = true;
    if (!fieldName.empty())
    {
      this->SetScalarField(fieldName);
    }
  }

  VISKORES_CONT
  void SetUnitScalar(viskores::Float32 unitScalar) { Tracer.SetUnitScalar(unitScalar); }

  void SetSampleDistance(const viskores::Float32& distance)
  {
    if (this->Mode != RenderMode::Volume)
    {
      throw viskores::cont::ErrorBadValue(
        "Conn Proxy: volume mode must be set before sample distance set");
    }
    Tracer.SetSampleDistance(distance);
  }

  VISKORES_CONT
  void SetRenderMode(RenderMode mode) { Mode = mode; }

  VISKORES_CONT
  RenderMode GetRenderMode() { return Mode; }

  VISKORES_CONT
  void SetScalarField(const std::string& fieldName)
  {
    this->FieldName = fieldName;
    const viskores::cont::ArrayHandle<viskores::Range> range =
      this->Dataset.GetField(this->FieldName).GetRange();
    ScalarRange = range.ReadPortal().Get(0);
  }

  VISKORES_CONT
  void SetColorMap(viskores::cont::ArrayHandle<viskores::Vec4f_32>& colormap)
  {
    Tracer.SetColorMap(colormap);
  }

  VISKORES_CONT
  void SetCompositeBackground(bool on) { CompositeBackground = on; }

  VISKORES_CONT
  void SetDebugPrints(bool on) { Tracer.SetDebugOn(on); }

  VISKORES_CONT
  void SetEpsilon(viskores::Float64 epsilon) { Tracer.SetEpsilon(epsilon); }

  VISKORES_CONT
  void SetEmissionField(const std::string& fieldName)
  {
    if (this->Mode != RenderMode::Energy)
    {
      throw viskores::cont::ErrorBadValue(
        "Conn Proxy: energy mode must be set before setting emission field");
    }
    this->EmissionFieldName = fieldName;
  }

  VISKORES_CONT
  viskores::Bounds GetSpatialBounds() const { return SpatialBounds; }

  VISKORES_CONT
  viskores::Range GetScalarFieldRange()
  {
    const viskores::cont::ArrayHandle<viskores::Range> range =
      this->Dataset.GetField(this->FieldName).GetRange();
    ScalarRange = range.ReadPortal().Get(0);
    return ScalarRange;
  }

  VISKORES_CONT
  void SetScalarRange(const viskores::Range& range) { this->ScalarRange = range; }

  VISKORES_CONT
  viskores::Range GetScalarRange() { return this->ScalarRange; }

  VISKORES_CONT
  void Trace(viskores::rendering::raytracing::Ray<viskores::Float64>& rays)
  {

    if (this->Mode == RenderMode::Volume)
    {
      Tracer.SetVolumeData(this->Dataset.GetField(this->FieldName),
                           this->ScalarRange,
                           this->Dataset.GetCellSet(),
                           this->Dataset.GetCoordinateSystem(this->CoordinateName),
                           this->Dataset.GetGhostCellField());
    }
    else
    {
      Tracer.SetEnergyData(this->Dataset.GetField(this->FieldName),
                           rays.Buffers.at(0).GetNumChannels(),
                           this->Dataset.GetCellSet(),
                           this->Dataset.GetCoordinateSystem(this->CoordinateName),
                           this->Dataset.GetField(this->EmissionFieldName));
    }

    Tracer.FullTrace(rays);
  }

  VISKORES_CONT
  void Trace(viskores::rendering::raytracing::Ray<viskores::Float32>& rays)
  {
    if (this->Mode == RenderMode::Volume)
    {
      Tracer.SetVolumeData(this->Dataset.GetField(this->FieldName),
                           this->ScalarRange,
                           this->Dataset.GetCellSet(),
                           this->Dataset.GetCoordinateSystem(this->CoordinateName),
                           this->Dataset.GetGhostCellField());
    }
    else
    {
      Tracer.SetEnergyData(this->Dataset.GetField(this->FieldName),
                           rays.Buffers.at(0).GetNumChannels(),
                           this->Dataset.GetCellSet(),
                           this->Dataset.GetCoordinateSystem(this->CoordinateName),
                           this->Dataset.GetField(this->EmissionFieldName));
    }

    Tracer.FullTrace(rays);
  }

  VISKORES_CONT
  PartialVector64 PartialTrace(viskores::rendering::raytracing::Ray<viskores::Float64>& rays)
  {

    if (this->Mode == RenderMode::Volume)
    {
      Tracer.SetVolumeData(this->Dataset.GetField(this->FieldName),
                           this->ScalarRange,
                           this->Dataset.GetCellSet(),
                           this->Dataset.GetCoordinateSystem(this->CoordinateName),
                           this->Dataset.GetGhostCellField());
    }
    else
    {
      Tracer.SetEnergyData(this->Dataset.GetField(this->FieldName),
                           rays.Buffers.at(0).GetNumChannels(),
                           this->Dataset.GetCellSet(),
                           this->Dataset.GetCoordinateSystem(this->CoordinateName),
                           this->Dataset.GetField(this->EmissionFieldName));
    }

    return Tracer.PartialTrace(rays);
  }

  VISKORES_CONT
  PartialVector32 PartialTrace(viskores::rendering::raytracing::Ray<viskores::Float32>& rays)
  {
    if (this->Mode == RenderMode::Volume)
    {
      Tracer.SetVolumeData(this->Dataset.GetField(this->FieldName),
                           this->ScalarRange,
                           this->Dataset.GetCellSet(),
                           this->Dataset.GetCoordinateSystem(this->CoordinateName),
                           this->Dataset.GetGhostCellField());
    }
    else
    {
      Tracer.SetEnergyData(this->Dataset.GetField(this->FieldName),
                           rays.Buffers.at(0).GetNumChannels(),
                           this->Dataset.GetCellSet(),
                           this->Dataset.GetCoordinateSystem(this->CoordinateName),
                           this->Dataset.GetField(this->EmissionFieldName));
    }

    return Tracer.PartialTrace(rays);
  }

  VISKORES_CONT
  void Trace(const viskores::rendering::Camera& camera,
             viskores::rendering::CanvasRayTracer* canvas)
  {

    if (canvas == nullptr)
    {
      throw viskores::cont::ErrorBadValue("Conn Proxy: null canvas");
    }
    viskores::rendering::raytracing::Camera rayCamera;
    rayCamera.SetParameters(
      camera, (viskores::Int32)canvas->GetWidth(), (viskores::Int32)canvas->GetHeight());
    viskores::rendering::raytracing::Ray<viskores::Float32> rays;
    rayCamera.CreateRays(rays, this->Dataset.GetCoordinateSystem(this->CoordinateName).GetBounds());
    rays.Buffers.at(0).InitConst(0.f);
    raytracing::RayOperations::MapCanvasToRays(rays, camera, *canvas);

    if (this->Mode == RenderMode::Volume)
    {
      Tracer.SetVolumeData(this->Dataset.GetField(this->FieldName),
                           this->ScalarRange,
                           this->Dataset.GetCellSet(),
                           this->Dataset.GetCoordinateSystem(this->CoordinateName),
                           this->Dataset.GetGhostCellField());
    }
    else
    {
      throw viskores::cont::ErrorBadValue("ENERGY MODE Not implemented for this use case\n");
    }

    Tracer.FullTrace(rays);

    canvas->WriteToCanvas(rays, rays.Buffers.at(0).Buffer, camera);
    if (CompositeBackground)
    {
      canvas->BlendBackground();
    }
  }
};


VISKORES_CONT
ConnectivityProxy::ConnectivityProxy(const viskores::cont::DataSet& dataSet,
                                     const std::string& fieldName)
  : Internals(
      std::make_unique<InternalsType>(dataSet, dataSet.GetCoordinateSystemName(), fieldName))
{
}

VISKORES_CONT
ConnectivityProxy::ConnectivityProxy(const viskores::cont::DataSet& dataSet,
                                     const std::string& fieldName,
                                     const std::string& coordinateName)
  : Internals(std::make_unique<InternalsType>(dataSet, coordinateName, fieldName))
{
}


VISKORES_CONT
ConnectivityProxy::ConnectivityProxy(const viskores::cont::UnknownCellSet& cellset,
                                     const viskores::cont::CoordinateSystem& coords,
                                     const viskores::cont::Field& scalarField)
{
  viskores::cont::DataSet dataset;
  dataset.SetCellSet(cellset);
  dataset.AddCoordinateSystem(coords);
  dataset.AddField(scalarField);

  Internals = std::make_unique<InternalsType>(dataset, coords.GetName(), scalarField.GetName());
}

ConnectivityProxy::ConnectivityProxy(const ConnectivityProxy& rhs)
  : Internals(nullptr)
{
  // rhs might have been moved, its Internal would be nullptr
  if (rhs.Internals)
  {
    Internals = std::make_unique<InternalsType>(*rhs.Internals);
  }
}

ConnectivityProxy& ConnectivityProxy::operator=(const ConnectivityProxy& rhs)
{
  // both *this and rhs might have been moved.
  if (!rhs.Internals)
  {
    Internals.reset();
  }
  else if (!Internals)
  {
    Internals = std::make_unique<InternalsType>(*rhs.Internals);
  }
  else
  {
    *Internals = *rhs.Internals;
  }

  return *this;
}

VISKORES_CONT
ConnectivityProxy::ConnectivityProxy(ConnectivityProxy&&) noexcept = default;
VISKORES_CONT
ConnectivityProxy& ConnectivityProxy::operator=(viskores::rendering::ConnectivityProxy&&) noexcept =
  default;
VISKORES_CONT
ConnectivityProxy::~ConnectivityProxy() = default;

VISKORES_CONT
void ConnectivityProxy::SetSampleDistance(const viskores::Float32& distance)
{
  Internals->SetSampleDistance(distance);
}

VISKORES_CONT
void ConnectivityProxy::SetRenderMode(RenderMode mode)
{
  Internals->SetRenderMode(mode);
}

VISKORES_CONT
void ConnectivityProxy::SetScalarField(const std::string& fieldName)
{
  Internals->SetScalarField(fieldName);
}

VISKORES_CONT
void ConnectivityProxy::SetColorMap(viskores::cont::ArrayHandle<viskores::Vec4f_32>& colormap)
{
  Internals->SetColorMap(colormap);
}

VISKORES_CONT
void ConnectivityProxy::SetEmissionField(const std::string& fieldName)
{
  Internals->SetEmissionField(fieldName);
}

VISKORES_CONT
viskores::Bounds ConnectivityProxy::GetSpatialBounds()
{
  return Internals->GetSpatialBounds();
}

VISKORES_CONT
viskores::Range ConnectivityProxy::GetScalarFieldRange()
{
  return Internals->GetScalarFieldRange();
}

VISKORES_CONT
void ConnectivityProxy::SetCompositeBackground(bool on)
{
  return Internals->SetCompositeBackground(on);
}

VISKORES_CONT
void ConnectivityProxy::SetScalarRange(const viskores::Range& range)
{
  Internals->SetScalarRange(range);
}

VISKORES_CONT
viskores::Range ConnectivityProxy::GetScalarRange()
{
  return Internals->GetScalarRange();
}

VISKORES_CONT
void ConnectivityProxy::Trace(viskores::rendering::raytracing::Ray<viskores::Float64>& rays)
{
  raytracing::Logger* logger = raytracing::Logger::GetInstance();
  logger->OpenLogEntry("connectivity_trace_64");
  if (this->Internals->GetRenderMode() == RenderMode::Volume)
  {
    logger->AddLogData("volume_mode", "true");
  }
  else
  {
    logger->AddLogData("volume_mode", "false");
  }

  Internals->Trace(rays);
  logger->CloseLogEntry(-1.0);
}

VISKORES_CONT
PartialVector32 ConnectivityProxy::PartialTrace(
  viskores::rendering::raytracing::Ray<viskores::Float32>& rays)
{
  raytracing::Logger* logger = raytracing::Logger::GetInstance();
  logger->OpenLogEntry("connectivity_trace_32");
  if (this->Internals->GetRenderMode() == RenderMode::Volume)
  {
    logger->AddLogData("volume_mode", "true");
  }
  else
  {
    logger->AddLogData("volume_mode", "false");
  }

  PartialVector32 res = Internals->PartialTrace(rays);

  logger->CloseLogEntry(-1.0);
  return res;
}

VISKORES_CONT
void ConnectivityProxy::Trace(viskores::rendering::raytracing::Ray<viskores::Float32>& rays)
{
  raytracing::Logger* logger = raytracing::Logger::GetInstance();
  logger->OpenLogEntry("connectivity_trace_32");
  if (this->Internals->GetRenderMode() == RenderMode::Volume)
  {
    logger->AddLogData("volume_mode", "true");
  }
  else
  {
    logger->AddLogData("volume_mode", "false");
  }

  Internals->Trace(rays);

  logger->CloseLogEntry(-1.0);
}

VISKORES_CONT
PartialVector64 ConnectivityProxy::PartialTrace(
  viskores::rendering::raytracing::Ray<viskores::Float64>& rays)
{
  raytracing::Logger* logger = raytracing::Logger::GetInstance();
  logger->OpenLogEntry("connectivity_trace_64");
  if (this->Internals->GetRenderMode() == RenderMode::Volume)
  {
    logger->AddLogData("volume_mode", "true");
  }
  else
  {
    logger->AddLogData("volume_mode", "false");
  }

  PartialVector64 res = Internals->PartialTrace(rays);

  logger->CloseLogEntry(-1.0);
  return res;
}

VISKORES_CONT
void ConnectivityProxy::Trace(const viskores::rendering::Camera& camera,
                              viskores::rendering::CanvasRayTracer* canvas)
{
  raytracing::Logger* logger = raytracing::Logger::GetInstance();
  logger->OpenLogEntry("connectivity_trace_32");
  logger->AddLogData("volume_mode", "true");

  Internals->Trace(camera, canvas);

  logger->CloseLogEntry(-1.0);
}

VISKORES_CONT
void ConnectivityProxy::SetDebugPrints(bool on)
{
  Internals->SetDebugPrints(on);
}

VISKORES_CONT
void ConnectivityProxy::SetEpsilon(viskores::Float64 epsilon)
{
  Internals->SetEpsilon(epsilon);
}

VISKORES_CONT
void ConnectivityProxy::SetUnitScalar(viskores::Float32 unitScalar)
{
  Internals->SetUnitScalar(unitScalar);
}
}
} // namespace viskores::rendering

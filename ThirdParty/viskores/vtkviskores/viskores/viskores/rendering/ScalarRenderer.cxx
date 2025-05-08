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

#include <viskores/rendering/ScalarRenderer.h>

#include <viskores/cont/Timer.h>
#include <viskores/cont/TryExecute.h>

#include <viskores/rendering/raytracing/Camera.h>
#include <viskores/rendering/raytracing/Logger.h>
#include <viskores/rendering/raytracing/RayOperations.h>
#include <viskores/rendering/raytracing/ScalarRenderer.h>
#include <viskores/rendering/raytracing/SphereExtractor.h>
#include <viskores/rendering/raytracing/SphereIntersector.h>
#include <viskores/rendering/raytracing/TriangleExtractor.h>

namespace viskores
{
namespace rendering
{

struct ScalarRenderer::InternalsType
{
  bool ValidDataSet = false;
  viskores::Int32 Width = 1024;
  viskores::Int32 Height = 1024;
  viskores::Float32 DefaultValue = viskores::Nan32();
  viskores::cont::DataSet DataSet;
  viskores::rendering::raytracing::ScalarRenderer Tracer;
  viskores::Bounds ShapeBounds;
};

ScalarRenderer::ScalarRenderer()
  : Internals(std::make_unique<InternalsType>())
{
}

ScalarRenderer::ScalarRenderer(ScalarRenderer&&) noexcept = default;
ScalarRenderer& ScalarRenderer::operator=(ScalarRenderer&&) noexcept = default;
ScalarRenderer::~ScalarRenderer() = default;

void ScalarRenderer::SetWidth(viskores::Int32 width)
{
  if (width < 1)
  {
    throw viskores::cont::ErrorBadValue("ScalarRenderer: width must be greater than 0");
  }
  Internals->Width = width;
}

void ScalarRenderer::SetDefaultValue(viskores::Float32 value)
{
  Internals->DefaultValue = value;
}

void ScalarRenderer::SetHeight(viskores::Int32 height)
{
  if (height < 1)
  {
    throw viskores::cont::ErrorBadValue("ScalarRenderer: height must be greater than 0");
  }
  Internals->Height = height;
}

void ScalarRenderer::SetInput(viskores::cont::DataSet& dataSet)
{
  this->Internals->DataSet = dataSet;
  this->Internals->ValidDataSet = true;

  raytracing::TriangleExtractor triExtractor;
  viskores::cont::UnknownCellSet cellSet = this->Internals->DataSet.GetCellSet();
  viskores::cont::CoordinateSystem coords = this->Internals->DataSet.GetCoordinateSystem();
  triExtractor.ExtractCells(cellSet, dataSet.GetGhostCellField());

  if (triExtractor.GetNumberOfTriangles() > 0)
  {
    auto triIntersector = std::make_unique<raytracing::TriangleIntersector>();
    triIntersector->SetData(coords, triExtractor.GetTriangles());
    this->Internals->ShapeBounds = triIntersector->GetShapeBounds();
    this->Internals->Tracer.SetShapeIntersector(std::move(triIntersector));
  }
}

ScalarRenderer::Result ScalarRenderer::Render(const viskores::rendering::Camera& camera)
{
  if (!Internals->ValidDataSet)
  {
    throw viskores::cont::ErrorBadValue("ScalarRenderer: input never set");
  }

  raytracing::Logger* logger = raytracing::Logger::GetInstance();
  logger->OpenLogEntry("scalar_render");
  viskores::cont::Timer tot_timer;
  tot_timer.Start();
  viskores::cont::Timer timer;
  timer.Start();

  // Create rays
  viskores::rendering::raytracing::Camera cam;
  cam.SetParameters(camera, this->Internals->Width, this->Internals->Height);

  // FIXME: rays are created with an unused Buffers.at(0), that ChannelBuffer
  //  also has wrong number of channels, thus allocates memory that is wasted.
  viskores::rendering::raytracing::Ray<viskores::Float32> rays;
  cam.CreateRays(rays, this->Internals->ShapeBounds);
  rays.Buffers.at(0).InitConst(0.f);

  // add fields
  const viskores::Id numFields = this->Internals->DataSet.GetNumberOfFields();
  std::map<std::string, viskores::Range> rangeMap;
  for (viskores::Id i = 0; i < numFields; ++i)
  {
    const auto& field = this->Internals->DataSet.GetField(i);
    if (field.GetData().GetNumberOfComponents() == 1)
    {
      auto ranges = field.GetRange();
      rangeMap[field.GetName()] = ranges.ReadPortal().Get(0);
      this->Internals->Tracer.AddField(field);
    }
  }

  this->Internals->Tracer.Render(rays, Internals->DefaultValue, cam);

  using ArrayF32 = viskores::cont::ArrayHandle<viskores::Float32>;
  std::vector<ArrayF32> res;
  std::vector<std::string> names;
  const size_t numBuffers = rays.Buffers.size();
  viskores::Id expandSize = Internals->Width * Internals->Height;

  for (size_t i = 0; i < numBuffers; ++i)
  {
    const std::string name = rays.Buffers[i].GetName();
    if (name == "default")
      continue;
    raytracing::ChannelBuffer<viskores::Float32> buffer = rays.Buffers[i];
    raytracing::ChannelBuffer<viskores::Float32> expanded =
      buffer.ExpandBuffer(rays.PixelIdx, expandSize, Internals->DefaultValue);
    res.push_back(expanded.Buffer);
    names.push_back(name);
  }

  raytracing::ChannelBuffer<viskores::Float32> depthChannel(1, rays.NumRays);
  depthChannel.Buffer = rays.Distance;
  raytracing::ChannelBuffer<viskores::Float32> depthExpanded =
    depthChannel.ExpandBuffer(rays.PixelIdx, expandSize, Internals->DefaultValue);

  Result result;
  result.Width = Internals->Width;
  result.Height = Internals->Height;
  result.Scalars = res;
  result.ScalarNames = names;
  result.Ranges = rangeMap;
  result.Depths = depthExpanded.Buffer;

  viskores::Float64 time = timer.GetElapsedTime();
  logger->AddLogData("write_to_canvas", time);
  time = tot_timer.GetElapsedTime();
  logger->CloseLogEntry(time);

  return result;
}

viskores::cont::DataSet ScalarRenderer::Result::ToDataSet()
{
  if (Scalars.empty())
  {
    throw viskores::cont::ErrorBadValue("ScalarRenderer: result empty");
  }

  VISKORES_ASSERT(Width > 0);
  VISKORES_ASSERT(Height > 0);

  viskores::cont::DataSet result;
  viskores::Vec<viskores::Float32, 3> origin(0.f, 0.f, 0.f);
  viskores::Vec<viskores::Float32, 3> spacing(1.f, 1.f, 1.f);
  viskores::Id3 dims(Width + 1, Height + 1, 1);
  result.AddCoordinateSystem(viskores::cont::CoordinateSystem("coords", dims, origin, spacing));
  viskores::Id2 dims2(dims[0], dims[1]);
  viskores::cont::CellSetStructured<2> resCellSet;
  resCellSet.SetPointDimensions(dims2);
  result.SetCellSet(resCellSet);

  const size_t fieldSize = Scalars.size();
  for (size_t i = 0; i < fieldSize; ++i)
  {
    result.AddField(
      viskores::cont::Field(ScalarNames[i], viskores::cont::Field::Association::Cells, Scalars[i]));
  }

  result.AddField(
    viskores::cont::Field("depth", viskores::cont::Field::Association::Cells, Depths));

  return result;
}
}
} // viskores::rendering

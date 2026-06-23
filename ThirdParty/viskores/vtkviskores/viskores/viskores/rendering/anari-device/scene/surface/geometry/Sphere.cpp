//=============================================================================
//
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//
//=============================================================================

#include "Sphere.h"

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandleIndex.h>
#include <viskores/cont/ArrayRangeCompute.h>
#include <viskores/filter/MapFieldPermutation.h>
#include <viskores/rendering/CanvasRayTracer.h>
#include <viskores/rendering/raytracing/Camera.h>
#include <viskores/rendering/raytracing/Ray.h>
#include <viskores/rendering/raytracing/RayOperations.h>
#include <viskores/rendering/raytracing/RayTracer.h>
#include <viskores/rendering/raytracing/SphereExtractor.h>
#include <viskores/rendering/raytracing/SphereIntersector.h>


#include <numeric>

namespace viskores_device
{

Sphere::Sphere(ViskoresDeviceGlobalState* s)
  : Geometry(s)
  , m_index(this)
{
  this->m_vertexAttributes.setAttributes(
    this,
    { "position", "radius", "color", "attribute0", "attribute1", "attribute2", "attribute3" });
  this->m_vertexAttributes.setAnariAssociation("vertex");
  this->m_vertexAttributes.setViskoresAssociation(viskores::cont::Field::Association::Points);
}

void Sphere::commitParameters()
{
  this->Geometry::commitParameters();

  m_index = getParamObject<Array1D>("primitive.index");
  this->m_vertexAttributes.commitParameters();
}

void Sphere::finalize()
{
  this->Geometry::finalize();

  helium::ChangeObserverPtr<Array1D>& positionArray = this->m_vertexAttributes.getParam("position");
  if (!positionArray)
  {
    reportMessage(ANARI_SEVERITY_WARNING,
                  "missing required parameter 'vertex.position' on sphere geometry");
    return;
  }

  m_globalRadius = getParam<float>("radius", 0.01f);
  const auto numSpheres = m_index ? m_index->size() : positionArray->size();

  this->m_dataSet = viskores::cont::DataSet{};

  if (m_index)
  {
    this->SetupIndexBased();
  }
  else
  {
    this->m_vertexAttributes.setFields(this->m_dataSet);
  }
  this->m_dataSet.AddCoordinateSystem("position");
  // At this point, this->m_dataSet should have fields for all vertex arrays
  // passed through ANARI properties.

  if (this->m_dataSet.HasField("radius"))
  {
    this->m_radiusRange = this->m_dataSet.GetField("radius").GetRange().ReadPortal().Get(0);
  }

  auto connIdx = viskores::cont::make_ArrayHandleIndex(numSpheres);
  viskores::cont::ArrayHandle<viskores::Id> conn;
  viskores::cont::ArrayCopy(connIdx, conn);

  viskores::cont::CellSetSingleType<> cellSet;
  cellSet.Fill(static_cast<viskores::Id>(numSpheres), viskores::CELL_SHAPE_VERTEX, 1, conn);
  this->m_dataSet.SetCellSet(cellSet);
}

void Sphere::SetupIndexBased()
{
  viskores::cont::ArrayHandle<viskores::Id> indexArray;
  viskores::cont::ArrayHandle<viskores::Vec3f> vertices;

  auto viskoresArray = this->m_index->dataAsViskoresArray();
  viskores::cont::ArrayCopyShallowIfPossible(viskoresArray, indexArray);

  // KEN: Instead of permuting arrays, why not build a set of vertex cells and use
  // that to permute the values? The underlying raycaster already supports that.

  // Fill a temporary dataset with unpermuted arrays.
  viskores::cont::DataSet unpermutedData;
  this->m_vertexAttributes.setFields(unpermutedData);

  for (viskores::IdComponent fieldI = 0; fieldI < unpermutedData.GetNumberOfFields(); ++fieldI)
  {
    viskores::cont::Field unpermutedField = unpermutedData.GetField(fieldI);
    viskores::filter::MapFieldPermutation(unpermutedField, indexArray, this->m_dataSet);
  }
}

void Sphere::render(viskores::rendering::Canvas& canvas,
                    const viskores::rendering::Camera& camera,
                    const viskores::cont::Field& field,
                    const viskores::cont::ArrayHandle<viskores::Vec4f_32>& colorMap) const
{
  viskores::rendering::raytracing::RayTracer tracer;
  viskores::rendering::raytracing::SphereExtractor sphereExtractor;

  const viskores::cont::DataSet& data = this->getDataSet();
  viskores::cont::CoordinateSystem coords = data.GetCoordinateSystem();

  viskores::Bounds shapeBounds;
  viskores::Range scalarRange = field.GetRange().ReadPortal().Get(0);

  if (this->m_dataSet.HasField("radius"))
  {
    // This builds the radius array using an adjustment of the radius based on the desired
    // min and max radius. However, we want to take the radius at face value, so insert values
    // that compute back to the original value. This is silly, so we should implement a simple
    // version that just takes the array.
    sphereExtractor.ExtractCoordinates(coords,
                                       this->m_dataSet.GetField("radius"),
                                       static_cast<viskores::Float32>(this->m_radiusRange.Min),
                                       static_cast<viskores::Float32>(this->m_radiusRange.Max));
  }
  else
  {
    sphereExtractor.ExtractCoordinates(coords, this->m_globalRadius);
  }

  if (sphereExtractor.GetNumberOfSpheres() > 0)
  {
    auto sphereIntersector = std::make_shared<viskores::rendering::raytracing::SphereIntersector>();
    sphereIntersector->SetData(coords, sphereExtractor.GetPointIds(), sphereExtractor.GetRadii());
    tracer.AddShapeIntersector(sphereIntersector);
    shapeBounds.Include(sphereIntersector->GetShapeBounds());
  }

  //
  // Create rays
  //
  viskores::Int32 width = (viskores::Int32)canvas.GetWidth();
  viskores::Int32 height = (viskores::Int32)canvas.GetHeight();

  viskores::rendering::raytracing::Camera rayCamera = camera.CreateRaytracingCamera(width, height);

  viskores::rendering::raytracing::Ray<viskores::Float32> rays;
  viskores::rendering::CanvasRayTracer* canvasRT =
    dynamic_cast<viskores::rendering::CanvasRayTracer*>(&canvas);
  VISKORES_ASSERT(canvasRT != nullptr);

  rayCamera.CreateRays(rays, shapeBounds);
  rays.Buffers.at(0).InitConst(0.f);
  viskores::rendering::raytracing::RayOperations::MapCanvasToRays(
    rays, camera.CreateRaytracingCamera(width, height), canvasRT->GetDepthBuffer());

  tracer.SetField(field, scalarRange);
  tracer.GetCamera() = rayCamera;
  tracer.SetColorMap(colorMap);
  tracer.Render(rays);

  canvasRT->WriteToCanvas(rays, rays.Buffers.at(0).Buffer, camera);
}

} // namespace viskores_device

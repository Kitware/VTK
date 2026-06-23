//=============================================================================
//
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//
//=============================================================================

#include "Cylinder.h"
// Viskores
#include <viskores/CellShape.h>
#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandleConstant.h>
#include <viskores/cont/ArrayHandleRuntimeVec.h>
#include <viskores/cont/CellSetSingleType.h>
#include <viskores/cont/Invoker.h>
#include <viskores/rendering/CanvasRayTracer.h>
#include <viskores/rendering/raytracing/CylinderExtractor.h>
#include <viskores/rendering/raytracing/CylinderIntersector.h>
#include <viskores/rendering/raytracing/RayOperations.h>
#include <viskores/rendering/raytracing/RayTracer.h>
#include <viskores/worklet/WorkletMapField.h>
// std
#include <numeric>

namespace viskores_device
{
namespace
{

class BuildCylinderCapMasks : public viskores::worklet::WorkletMapField
{
public:
  viskores::UInt8 GlobalMask{ 0 };
  viskores::Id NumberOfVertices{ 0 };
  viskores::Id NumberOfCylinders{ 0 };
  viskores::Id NumberOfCaps{ 0 };

  VISKORES_CONT
  BuildCylinderCapMasks(viskores::UInt8 globalMask,
                        viskores::Id numberOfVertices,
                        viskores::Id numberOfCylinders,
                        viskores::Id numberOfCaps)
    : GlobalMask(globalMask)
    , NumberOfVertices(numberOfVertices)
    , NumberOfCylinders(numberOfCylinders)
    , NumberOfCaps(numberOfCaps)
  {
  }

  using ControlSignature = void(FieldIn, FieldOut, WholeArrayIn);
  using ExecutionSignature = void(_1, _2, _3, WorkIndex);

  template <typename CapPortalType>
  VISKORES_EXEC void operator()(const viskores::Id3& cylinderId,
                                viskores::UInt8& capMask,
                                const CapPortalType& caps,
                                viskores::Id workIndex) const
  {
    if (this->NumberOfCaps >= this->NumberOfVertices)
    {
      capMask = 0;
      if (caps.Get(cylinderId[1]) != 0)
        capMask |= 1;
      if (caps.Get(cylinderId[2]) != 0)
        capMask |= 2;
    }
    else if (this->NumberOfCaps >= this->NumberOfCylinders * 2)
    {
      capMask = 0;
      if (caps.Get(2 * workIndex) != 0)
        capMask |= 1;
      if (caps.Get(2 * workIndex + 1) != 0)
        capMask |= 2;
    }
    else if (this->NumberOfCaps >= this->NumberOfCylinders)
    {
      capMask = caps.Get(workIndex) != 0 ? viskores::UInt8(3) : viskores::UInt8(0);
    }
    else
    {
      capMask = this->GlobalMask;
    }
  }
};

} // anonymous namespace

Cylinder::Cylinder(ViskoresDeviceGlobalState* s)
  : Geometry(s)
  , m_index(this)
  , m_radius(this)
  , m_caps(this)
{
  this->m_vertexAttributes.setAttributes(
    this, { "position", "color", "attribute0", "attribute1", "attribute2", "attribute3" });
  this->m_vertexAttributes.setAnariAssociation("vertex");
  this->m_vertexAttributes.setViskoresAssociation(viskores::cont::Field::Association::Points);
}

void Cylinder::commitParameters()
{
  this->Geometry::commitParameters();

  this->m_index = getParamObject<Array1D>("primitive.index");
  this->m_radius = getParamObject<Array1D>("primitive.radius");
  this->m_caps = getParamObject<Array1D>("vertex.cap");
  this->m_globalRadius = getParam<float>("radius", 0.01f);
  this->m_globalCapMask = this->ParseCaps(this->getParamString("caps", "none"));
  this->m_vertexAttributes.commitParameters();
}

void Cylinder::finalize()
{
  this->m_dataSet = viskores::cont::DataSet{};
  this->m_cylinderIds = viskores::cont::ArrayHandle<viskores::Id3>{};
  this->m_radii = viskores::cont::ArrayHandle<viskores::Float32>{};
  this->m_capMasks = viskores::cont::ArrayHandle<viskores::UInt8>{};
  this->m_cylinderIntersector.reset();

  helium::ChangeObserverPtr<Array1D>& positionArray = this->m_vertexAttributes.getParam("position");
  if (!positionArray)
  {
    reportMessage(ANARI_SEVERITY_WARNING,
                  "'cylinder' geometry missing 'vertex.position' parameter");
    return;
  }

  if (!this->m_index)
  {
    reportMessage(ANARI_SEVERITY_INFO, "generating 'cylinder' index array");

    Array1DMemoryDescriptor md;
    md.appMemory = nullptr;
    md.deleter = nullptr;
    md.deleterPtr = nullptr;
    md.elementType = ANARI_UINT32_VEC2;
    md.numItems = positionArray->totalSize() / 2;

    this->m_index = new Array1D(this->deviceState(), md);
    this->m_index->refDec(helium::RefType::PUBLIC); // no public references

    auto* begin = (uint32_t*)this->m_index->map();
    auto* end = begin + this->m_index->totalSize() * 2;
    std::iota(begin, end, 0);
  }

  viskores::cont::ArrayHandleRuntimeVec<viskores::Id> connectionArray(2);
  viskores::cont::ArrayCopyShallowIfPossible(this->m_index->dataAsViskoresArray(), connectionArray);

  viskores::cont::CellSetSingleType<> cellSet;
  cellSet.Fill(static_cast<viskores::Id>(positionArray->size()),
               viskores::CELL_SHAPE_LINE,
               2,
               connectionArray.GetComponentsArray());
  this->m_dataSet.SetCellSet(cellSet);

  this->Geometry::finalize();
  this->m_vertexAttributes.setFields(this->m_dataSet);

  this->m_dataSet.AddCoordinateSystem("position");

  viskores::rendering::raytracing::CylinderExtractor cylinderExtractor;
  cylinderExtractor.ExtractCells(this->m_dataSet.GetCellSet(), this->m_globalRadius);

  this->m_cylinderIds = cylinderExtractor.GetCylIds();
  this->BuildRadii(cylinderExtractor.GetNumberOfCylinders());
  this->BuildCapMasks();

  if (this->m_cylinderIds.GetNumberOfValues() > 0)
  {
    this->m_cylinderIntersector =
      std::make_shared<viskores::rendering::raytracing::CylinderIntersector>();
    this->m_cylinderIntersector->SetData(
      this->m_dataSet.GetCoordinateSystem(), this->m_cylinderIds, this->m_radii, this->m_capMasks);
  }
}

void Cylinder::render(viskores::rendering::Canvas& canvas,
                      const viskores::rendering::Camera& camera,
                      const viskores::cont::Field& field,
                      const viskores::cont::ArrayHandle<viskores::Vec4f_32>& colorMap) const
{
  viskores::rendering::raytracing::RayTracer tracer;

  viskores::Bounds shapeBounds;
  viskores::Range scalarRange = field.GetRange().ReadPortal().Get(0);

  if (this->m_cylinderIntersector)
  {
    tracer.AddShapeIntersector(this->m_cylinderIntersector);
    shapeBounds.Include(this->m_cylinderIntersector->GetShapeBounds());
  }

  viskores::Int32 width = (viskores::Int32)canvas.GetWidth();
  viskores::Int32 height = (viskores::Int32)canvas.GetHeight();

  viskores::rendering::raytracing::Camera rayCamera = camera.CreateRaytracingCamera(width, height);

  viskores::rendering::raytracing::Ray<viskores::Float32> rays;
  viskores::rendering::CanvasRayTracer* canvasRT =
    dynamic_cast<viskores::rendering::CanvasRayTracer*>(&canvas);
  VISKORES_ASSERT(canvasRT != nullptr);

  rayCamera.CreateRays(rays, shapeBounds);
  tracer.GetCamera() = rayCamera;
  rays.Buffers.at(0).InitConst(0.f);
  viskores::rendering::raytracing::RayOperations::MapCanvasToRays(
    rays, camera.CreateRaytracingCamera(width, height), canvasRT->GetDepthBuffer());

  tracer.SetField(field, scalarRange);
  tracer.SetColorMap(colorMap);
  tracer.SetShadingOn(true);
  tracer.Render(rays);

  canvasRT->WriteToCanvas(rays, rays.Buffers.at(0).Buffer, camera);
}

bool Cylinder::isValid() const
{
  return this->m_vertexAttributes.getParam("position");
}

viskores::UInt8 Cylinder::ParseCaps(const std::string& caps)
{
  if (caps == "none")
    return 0;
  if (caps == "first")
    return 1;
  if (caps == "second")
    return 2;
  if (caps == "both")
    return 3;

  reportMessage(ANARI_SEVERITY_WARNING, "invalid 'caps' value on 'cylinder' geometry");
  return 0;
}

void Cylinder::BuildRadii(viskores::Id numberOfCylinders)
{
  if (this->m_radius && this->m_radius->size() >= static_cast<size_t>(numberOfCylinders))
  {
    viskores::cont::ArrayCopyShallowIfPossible(this->m_radius->dataAsViskoresArray(),
                                               this->m_radii);
  }
  else
  {
    this->m_radii.AllocateAndFill(numberOfCylinders, this->m_globalRadius);
  }
}

void Cylinder::BuildCapMasks()
{
  const viskores::Id numberOfCylinders = this->m_cylinderIds.GetNumberOfValues();

  if (this->m_caps)
  {
    auto& positionArray = this->m_vertexAttributes.getParam("position");
    viskores::cont::ArrayHandle<viskores::UInt8> caps;
    viskores::cont::ArrayCopyShallowIfPossible(this->m_caps->dataAsViskoresArray(), caps);

    viskores::cont::Invoker invoke;
    invoke(BuildCylinderCapMasks(this->m_globalCapMask,
                                 static_cast<viskores::Id>(positionArray->size()),
                                 numberOfCylinders,
                                 static_cast<viskores::Id>(this->m_caps->size())),
           this->m_cylinderIds,
           this->m_capMasks,
           caps);
  }
  else
  {
    this->m_capMasks.AllocateAndFill(numberOfCylinders, this->m_globalCapMask);
  }
}

} // namespace viskores_device

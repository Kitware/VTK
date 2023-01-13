// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDGRenderResponder.h"

#include "vtkActor.h"
#include "vtkCellAttributeInformation.h"
#include "vtkCellGrid.h"
#include "vtkCellGridMapper.h"
#include "vtkCellMetadata.h"
#include "vtkCollectionIterator.h"
#include "vtkDGAttributeInformation.h"
#include "vtkDGCell.h"
#include "vtkDataSetAttributes.h"
#include "vtkGLSLModCamera.h"
#include "vtkGLSLModCoincidentTopology.h"
#include "vtkGLSLModLight.h"
#include "vtkGLSLModPixelDebugger.h"
#include "vtkGLSLModifierFactory.h"
#include "vtkInformation.h"
#include "vtkLightingMapPass.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderPass.h"
#include "vtkOpenGLRenderer.h"
#include "vtkPBRFunctions.h"
#include "vtkPBRPrefilterTexture.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkShader.h"
#include "vtkShaderProgram.h"
#include "vtkShaderProperty.h"
#include "vtkSmartPointer.h"
#include "vtkTypeFloat32Array.h"
#include "vtkTypeInt32Array.h"
#include "vtkUniforms.h"

#include "vtk_fmt.h"
// Keep clang-format from adding spaces around the '/' path separator:
// clang-format off
#include VTK_FMT(fmt/args.h)
#include VTK_FMT(fmt/core.h)
#include VTK_FMT(fmt/format.h)
#include VTK_FMT(fmt/ostream.h)
// clang-format on

#include <sstream>
#include <string>
#include <unordered_set>

// Generated files (from glsl source)
#include "vtkCellGridShaderBases.h"
#include "vtkCellGridShaderCommonDefs.h"
#include "vtkCellGridShaderFragment.h"
#include "vtkCellGridShaderUtil.h"
#include "vtkCellGridShaderVertex.h"

// Uncomment to print shader/color info to cout
// #define vtkDGRenderResponder_DEBUG

VTK_ABI_NAMESPACE_BEGIN

using namespace vtk::literals;

namespace
{
void ReplaceShaderRenderPass(std::string& vsSrc, std::string& gsSrc, std::string& fsSrc,
  vtkAbstractMapper* mapper, vtkActor* actor, bool prePass)
{
  vtkInformation* info = actor->GetPropertyKeys();
  if (info && info->Has(vtkOpenGLRenderPass::RenderPasses()))
  {
    int numRenderPasses = info->Length(vtkOpenGLRenderPass::RenderPasses());
    for (int i = 0; i < numRenderPasses; ++i)
    {
      vtkObjectBase* rpBase = info->Get(vtkOpenGLRenderPass::RenderPasses(), i);
      vtkOpenGLRenderPass* rp = static_cast<vtkOpenGLRenderPass*>(rpBase);
      if (prePass)
      {
        if (!rp->PreReplaceShaderValues(vsSrc, gsSrc, fsSrc, mapper, actor))
        {
          vtkErrorWithObjectMacro(
            mapper, "vtkOpenGLRenderPass::ReplaceShaderValues failed for " << rp->GetClassName());
        }
      }
      else
      {
        if (!rp->PostReplaceShaderValues(vsSrc, gsSrc, fsSrc, mapper, actor))
        {
          vtkErrorWithObjectMacro(
            mapper, "vtkOpenGLRenderPass::ReplaceShaderValues failed for " << rp->GetClassName());
        }
      }
    }
  }
}

vtkMTimeType GetRenderPassStageMTime(vtkActor* actor, vtkInformation* lastRpInfo)
{
  vtkInformation* info = actor->GetPropertyKeys();
  vtkMTimeType renderPassMTime = 0;

  int curRenderPasses = 0;
  if (info && info->Has(vtkOpenGLRenderPass::RenderPasses()))
  {
    curRenderPasses = info->Length(vtkOpenGLRenderPass::RenderPasses());
  }

  int lastRenderPasses = 0;
  if (lastRpInfo->Has(vtkOpenGLRenderPass::RenderPasses()))
  {
    lastRenderPasses = lastRpInfo->Length(vtkOpenGLRenderPass::RenderPasses());
  }
  else // have no last pass
  {
    if (!info) // have no current pass
    {
      return 0; // short circuit
    }
  }

  // Determine the last time a render pass changed stages:
  if (curRenderPasses != lastRenderPasses)
  {
    // Number of passes changed, definitely need to update.
    // Fake the time to force an update:
    renderPassMTime = VTK_MTIME_MAX;
  }
  else
  {
    // Compare the current to the previous render passes:
    for (int i = 0; i < curRenderPasses; ++i)
    {
      vtkObjectBase* curRP = info->Get(vtkOpenGLRenderPass::RenderPasses(), i);
      vtkObjectBase* lastRP = lastRpInfo->Get(vtkOpenGLRenderPass::RenderPasses(), i);

      if (curRP != lastRP)
      {
        // Render passes have changed. Force update:
        renderPassMTime = VTK_MTIME_MAX;
        break;
      }
      else
      {
        // Render passes have not changed -- check MTime.
        vtkOpenGLRenderPass* rp = static_cast<vtkOpenGLRenderPass*>(curRP);
        renderPassMTime = std::max(renderPassMTime, rp->GetShaderStageMTime());
      }
    }
  }

  // Cache the current set of render passes for next time:
  if (info)
  {
    lastRpInfo->CopyEntry(info, vtkOpenGLRenderPass::RenderPasses());
  }
  else
  {
    lastRpInfo->Clear();
  }

  return renderPassMTime;
}
}

bool vtkDGRenderResponder::CacheEntry::IsUpToDate(vtkRenderer* renderer, vtkActor* actor,
  vtkMapper* mapper, vtkObject* debugAttachment /*=nullptr*/) const
{
  (void)debugAttachment; // Only referenced in debug builds.
  if (this->ShapeTime < this->Shape->GetMTime())
  {
    vtkDebugWithObjectMacro(debugAttachment, << "Shape is outdated");
    return false;
  }
  if (this->Color && this->ColorTime < this->Color->GetMTime())
  {
    vtkDebugWithObjectMacro(debugAttachment, << "Color is outdated");
    return false;
  }
  if (this->GridTime < this->CellType->GetCellGrid()->GetMTime())
  {
    vtkDebugWithObjectMacro(debugAttachment, << "CellGrid is outdated");
    return false;
  }
  if (this->PropertyTime < actor->GetProperty()->GetMTime())
  {
    vtkDebugWithObjectMacro(debugAttachment, << "Actor is outdated");
    return false;
  }
  if (this->MapperTime < mapper->GetMTime())
  {
    vtkDebugWithObjectMacro(debugAttachment, << "Mapper is outdated");
    return false;
  }
  if (this->RenderPassStageTime < ::GetRenderPassStageMTime(actor, this->LastRenderPassInfo))
  {
    vtkDebugWithObjectMacro(debugAttachment, << "RenderPassStage is outdated");
    return false;
  }
  auto modsIter = vtk::TakeSmartPointer(this->RenderHelper->GetGLSLModCollection()->NewIterator());
  auto oglRen = static_cast<vtkOpenGLRenderer*>(renderer);
  for (modsIter->InitTraversal(); !modsIter->IsDoneWithTraversal(); modsIter->GoToNextItem())
  {
    auto mod = static_cast<vtkGLSLModifierBase*>(modsIter->GetCurrentObject());
    if (!mod->IsUpToDate(oglRen, mapper, actor))
    {
      vtkDebugWithObjectMacro(nullptr, << mod->GetClassName() << " is outdated");
      // if any mod is outdated, entire shader program must be re-compiled.
      return false;
    }
  }
  return true;
}

void vtkDGRenderResponder::CacheEntry::PrepareHelper(
  vtkRenderer* renderer, vtkActor* actor, vtkMapper* mapper) const
{
  auto* cgMapper = vtkCellGridMapper::SafeDownCast(mapper);

  this->RenderHelper = std::unique_ptr<vtkDrawTexturedElements>(new vtkDrawTexturedElements);
  auto primType = vtkDGRenderResponder::PrimitiveFromShape(this->CellSource->SourceShape);
#ifdef vtkDGRenderResponder_DEBUG
  std::cout << "    Primitive type " << primType << " for shape "
            << vtkDGCell::GetShapeName(this->CellSource->SourceShape).Data() << "\n";
#endif
  this->RenderHelper->SetElementType(primType);
  switch (this->CellSource->SourceShape)
  {
      // Volume cells should never be rendered directly:
    case vtkDGCell::Shape::None:
    case vtkDGCell::Shape::Hexahedron:
    case vtkDGCell::Shape::Tetrahedron:
    case vtkDGCell::Shape::Wedge:
    case vtkDGCell::Shape::Pyramid:
    {
      vtkErrorWithObjectMacro(cgMapper, "Volume/invalid cells should never be rendered directly.");
    }
    break;
      // Prismatic 2-/3-d shapes require two triangles per side/cell (respectively).
    case vtkDGCell::Shape::Quadrilateral:
      this->RenderHelper->SetNumberOfElements(2);
      break;
      // Simplicial shapes require just 1 primitive per side/cell (of any dimension).
    case vtkDGCell::Shape::Triangle:
    case vtkDGCell::Shape::Edge:
    case vtkDGCell::Shape::Vertex:
      this->RenderHelper->SetNumberOfElements(1);
      break;
  }
  auto* vertShader = this->RenderHelper->GetShader(vtkShader::Vertex);
  auto* fragShader = this->RenderHelper->GetShader(vtkShader::Fragment);

  std::string shaderCommonTemplate = vtkCellGridShaderCommonDefs;
  std::string shaderBasisTemplate = vtkCellGridShaderBases;
  std::string shaderUtilTemplate = vtkCellGridShaderUtil;
  std::string vertShaderTemplate = vtkCellGridShaderVertex;
  std::string fragShaderTemplate = vtkCellGridShaderFragment;

  // Set up substitutions for shaders.
  fmt::dynamic_format_arg_store<fmt::format_context> store;
  store.push_back(fmt::arg("NumPtsPerSide",
    this->CellSource->SideType < 0
      ? this->CellType->GetNumberOfCorners()
      : vtkDGCell::GetShapeCornerCount(this->CellSource->SourceShape)));
  store.push_back(fmt::arg("NumPtsPerCell", this->CellType->GetNumberOfCorners()));
  store.push_back(fmt::arg(
    "ShapeIndex", this->CellSource->SideType + (this->CellType->GetDimension() < 3 ? 1 : 0)));
  store.push_back(fmt::arg("SideOffset",
    this->CellSource->SideType < 0
      ? 0
      : this->CellType->GetSideRangeForType(this->CellSource->SideType).first));
  store.push_back(fmt::arg("DrawingCellsNotSides", this->CellSource->SideType == -1));
  store.push_back(fmt::arg("HaveColors", !!this->Color));
  store.push_back(
    fmt::arg("ShapeName", vtkDGCell::GetShapeName(this->CellType->GetShape()).Data()));
  auto shapeInfo = this->CellType->GetCaches()->AttributeCalculator<vtkCellAttributeInformation>(
    this->CellType, this->Shape);
  if (shapeInfo)
  {
    store.push_back(fmt::arg("ShapeNumBasisFun", shapeInfo->GetNumberOfBasisFunctions()));
    store.push_back(fmt::arg("ShapeBasisSize", shapeInfo->GetBasisValueSize()));
    store.push_back(fmt::arg(
      "ShapeMultiplicity", shapeInfo->GetDegreeOfFreedomSize())); // NB: Only works for HGrad
    store.push_back(fmt::arg("ShapeCoeffPerCell",
      shapeInfo->GetNumberOfBasisFunctions() * shapeInfo->GetDegreeOfFreedomSize()));
    store.push_back(fmt::arg(
      "ShapeNumValPP", shapeInfo->GetBasisValueSize() * shapeInfo->GetDegreeOfFreedomSize()));
    store.push_back(fmt::arg("ShapeCellBasisSize",
      shapeInfo->GetNumberOfBasisFunctions() * shapeInfo->GetBasisValueSize()));
    store.push_back(fmt::arg("ShapeBasisName", shapeInfo->GetBasisName()));
  }
  else
  {
    store.push_back(fmt::arg("ShapeNumBasisFun", 1));
    store.push_back(fmt::arg("ShapeBasisSize", 1));
    store.push_back(fmt::arg("ShapeMultiplicity", 3)); // NB: Only works for HGrad
    store.push_back(fmt::arg("ShapeCoeffPerCell", this->CellType->GetNumberOfCorners() * 3));
    store.push_back(fmt::arg("ShapeNumValPP", 3));
    store.push_back(fmt::arg("ShapeCellBasisSize", 8));
    store.push_back(fmt::arg(
      "ShapeBasisName", "None" + vtkDGAttributeInformation::BasisShapeName(this->CellType) + "I0"));
  }
  vtkSmartPointer<vtkCellAttributeInformation> colorInfo;
  if (this->Color)
  {
    colorInfo = this->CellType->GetCaches()->AttributeCalculator<vtkCellAttributeInformation>(
      this->CellType, this->Color);
  }
  store.push_back(fmt::arg("ColorBasisName",
    colorInfo ? colorInfo->GetBasisName()
              : "None" + vtkDGAttributeInformation::BasisShapeName(this->CellType) + "I0"));
  store.push_back(fmt::arg("ColorBasisSize", colorInfo ? colorInfo->GetBasisValueSize() : 9));
  store.push_back(
    fmt::arg("ColorMultiplicity", colorInfo ? colorInfo->GetDegreeOfFreedomSize() : 1));
  store.push_back(
    fmt::arg("ColorNumBasisFun", colorInfo ? colorInfo->GetNumberOfBasisFunctions() : 1));
  store.push_back(
    fmt::arg("ColorContinuous", colorInfo ? colorInfo->GetSharedDegreesOfFreedom() : false));
  store.push_back(fmt::arg("ColorCoeffPerCell",
    colorInfo ? colorInfo->GetNumberOfBasisFunctions() * colorInfo->GetDegreeOfFreedomSize() : 24));
  store.push_back(fmt::arg("ColorNumValPP",
    colorInfo ? colorInfo->GetBasisValueSize() * colorInfo->GetDegreeOfFreedomSize() : 9));
  store.push_back(fmt::arg("ColorCellBasisSize",
    colorInfo ? colorInfo->GetNumberOfBasisFunctions() * colorInfo->GetBasisValueSize() : 1));
  // When we have a vector-valued basis function, we should scale by the shape's inverse Jacobian.
  store.push_back(fmt::arg(
    "ColorScaleInverseJacobian", colorInfo ? (colorInfo->GetBasisValueSize() == 3 ? 1 : 0) : 0));
  this->RenderHelper->SetIncludeColormap(!!this->Color);
#ifdef vtkDGRenderResponder_DEBUG
  std::cout << "Color cell-attribute: " << this->Color << "\n";
#endif
  std::string shaderCommonSource = fmt::vformat(shaderCommonTemplate, store);
  store.push_back(fmt::arg("commonDefs", shaderCommonSource));

  std::string shaderUtilSource = fmt::vformat(shaderUtilTemplate, store);
  store.push_back(fmt::arg("cellUtil", shaderUtilSource));

  std::string shaderBasisSource = fmt::vformat(shaderBasisTemplate, store);
  store.push_back(fmt::arg("cellEval", shaderBasisSource));

  std::string vertShaderSource = fmt::vformat(vertShaderTemplate, store);
  std::string fragShaderSource = fmt::vformat(fragShaderTemplate, store);

  auto oglRenderer = static_cast<vtkOpenGLRenderer*>(renderer);
  std::string emptyGS;
  // Pre-pass.
  ::ReplaceShaderRenderPass(vertShaderSource, emptyGS, fragShaderSource, mapper, actor, true);
  // Apply shader mods.
  for (const auto& modName : this->ModNames)
  {
    auto mod = vtk::TakeSmartPointer(vtkGLSLModifierFactory::CreateAMod(modName));
    mod->ReplaceShaderValues(
      oglRenderer, vertShaderSource, emptyGS, fragShaderSource, mapper, actor);
    this->RenderHelper->GetGLSLModCollection()->AddItem(mod);
  }
  // Post-pass.
  ::ReplaceShaderRenderPass(vertShaderSource, emptyGS, fragShaderSource, mapper, actor, false);

  vertShader->SetSource(vertShaderSource);
  fragShader->SetSource(fragShaderSource);
#if 0
  std::cout
    << "VertexShaderSource\n" << vertShaderSource << "\n"
    << "FragmentShaderSource\n" << fragShaderSource << "\n"
    ;
  std::cout.flush();
#endif

  vtkStringToken cellTypeToken = this->CellType->GetClassName();

  // Now that we've set our shader source strings, we can bind
  // vertex-buffer objects to samplers they reference.
  // 1. Bind arrays defining the shape attribute.
  auto* shapeConn = vtkDataArray::SafeDownCast(
    this->Shape->GetArrayForCellTypeAndRole(cellTypeToken, "connectivity"_token));
  auto* shapeVals = vtkDataArray::SafeDownCast(
    this->Shape->GetArrayForCellTypeAndRole(cellTypeToken, "values"_token));
#ifdef vtkDGRenderResponder_DEBUG
  std::cout << "Binding \"shape_conn\" to array " << shapeConn << " ("
            << shapeConn->GetNumberOfTuples() << "×" << shapeConn->GetNumberOfComponents() << ")\n";
#endif
  this->RenderHelper->BindArrayToTexture("shape_conn"_token, shapeConn, true);
  this->RenderHelper->BindArrayToTexture("shape_vals"_token, shapeVals, true);
  this->ShapeTime = this->Shape->GetMTime();

  // 2. If coloring by a cell-attribute, bind those arrays
  //    as well as the colormap texture and the scalar range
  //    to use for colormap lookups.
  if (this->Color)
  {
    auto* colorConn = vtkDataArray::SafeDownCast(
      this->Color->GetArrayForCellTypeAndRole(cellTypeToken, "connectivity"_token));
    auto* colorVals = vtkDataArray::SafeDownCast(
      this->Color->GetArrayForCellTypeAndRole(cellTypeToken, "values"_token));
    this->RenderHelper->BindArrayToTexture("color_conn"_token, colorConn, true);
    this->RenderHelper->BindArrayToTexture("color_vals"_token, colorVals, true);
    cgMapper->PrepareColormap(this->Color->GetColormap()); // TODO: Override with actor/mapper cmap?
    std::array<double, 3> compRange;
    this->CellType->GetCellGrid()->GetCellAttributeRange(
      this->Color, mapper->GetArrayComponent(), compRange.data(), true);
    if (compRange[0] > compRange[1])
    {
      compRange[0] = -1e-11;
      compRange[1] = +1e-11;
    }
    compRange[2] = compRange[1] - compRange[0];
#ifdef vtkDGRenderResponder_DEBUG
    std::cout << "  Color range: [" << compRange[0] << ", " << compRange[1] << "] delta "
              << compRange[2] << " comp " << mapper->GetArrayComponent() << "\n";
#endif
    actor->GetShaderProperty()->GetFragmentCustomUniforms()->SetUniformi(
      "color_component", mapper->GetArrayComponent());
    actor->GetShaderProperty()->GetFragmentCustomUniforms()->SetUniform3f(
      "color_range", compRange.data());
    this->ColorTime = this->Color->GetMTime();
  }

  // 3. Bind arrays that specify the reference cell and the linkage between
  //    the reference cell and the shape-attribute's connectivity array.
  this->RenderHelper->BindArrayToTexture(
    "side_offsets"_token, this->CellType->GetSideOffsetsAndShapes());
  this->RenderHelper->BindArrayToTexture("side_local"_token, this->CellType->GetSideConnectivity());
  this->RenderHelper->BindArrayToTexture(
    "cell_parametrics"_token, vtkDataArray::SafeDownCast(this->CellType->GetReferencePoints()));
  if (this->CellSource->SideType < 0)
  {
    this->RenderHelper->SetNumberOfInstances(shapeConn->GetNumberOfTuples());
  }
  else
  {
#ifdef vtkDGRenderResponder_DEBUG
    std::cout << "Binding \"sides\" to array " << this->CellSource->Connectivity << " ("
              << this->CellSource->Connectivity->GetNumberOfTuples() << "×"
              << this->CellSource->Connectivity->GetNumberOfComponents() << ")\n";
#endif
    this->RenderHelper->BindArrayToTexture(
      "sides"_token, vtkDataArray::SafeDownCast(this->CellSource->Connectivity));
    this->RenderHelper->SetNumberOfInstances(this->CellSource->Connectivity->GetNumberOfTuples());
  }
  this->GridTime = this->CellType->GetCellGrid()->GetMTime();
  this->PropertyTime = actor->GetMTime();
  this->MapperTime = mapper->GetMTime();
}

bool vtkDGRenderResponder::CacheEntry::operator<(
  const vtkDGRenderResponder::CacheEntry& other) const
{
  return (this->CellType < other.CellType ||
    (this->CellType == other.CellType &&
      (this->CellSource < other.CellSource ||
        (this->CellSource == other.CellSource &&
          (this->Shape < other.Shape ||
            (this->Shape == other.Shape && this->Color < other.Color))))));
}

vtkStandardNewMacro(vtkDGRenderResponder);

vtkDGRenderResponder::vtkDGRenderResponder()
{
  // Ensure the following tokens have strings in the dictionary so vtkStringToken::Data()
  // will be able to return them.
  vtkStringToken shape_conn = "shape_conn";
  vtkStringToken shape_vals = "shape_vals";
  vtkStringToken color_conn = "color_conn";
  vtkStringToken color_vals = "color_vals";
  vtkStringToken side_offsets = "side_offsets";
  vtkStringToken side_local = "side_local";
  vtkStringToken cell_parametrics = "cell_parametrics";
  vtkStringToken sides = "sides";

  (void)shape_conn;
  (void)shape_vals;
  (void)color_conn;
  (void)color_vals;
  (void)side_offsets;
  (void)side_local;
  (void)cell_parametrics;
  (void)sides;

#ifdef vtkDGRenderResponder_DEBUG
  this->DebugOn();
#endif
}

// When new default mods are added, make sure to register them in
// vtkDGRenderResponder::ResetModsToDefault below.
std::vector<std::string> vtkDGRenderResponder::DefaultModNames = { "vtkGLSLModCamera",
  "vtkGLSLModLight", "vtkGLSLModCoincidentTopology", "vtkGLSLModPixelDebugger" };

void vtkDGRenderResponder::ResetModsToDefault()
{
  // just to be sure.
  this->RemoveAllMods();
  this->AddMods(vtkDGRenderResponder::DefaultModNames);
  vtkGLSLModifierFactory::RegisterAMod(
    DefaultModNames[0], [](void*) { return vtkGLSLModCamera::New(); });
  vtkGLSLModifierFactory::RegisterAMod(
    DefaultModNames[1], [](void*) { return vtkGLSLModLight::New(); });
  vtkGLSLModifierFactory::RegisterAMod(
    DefaultModNames[2], [](void*) { return vtkGLSLModCoincidentTopology::New(); });
  vtkGLSLModifierFactory::RegisterAMod(
    DefaultModNames[3], [](void*) { return vtkGLSLModPixelDebugger::New(); });
}

void vtkDGRenderResponder::AddMod(const std::string& className)
{
  if (!this->ModNamesUnique.count(className))
  {
    this->ModNames.emplace_back(className);
    this->ModNamesUnique.insert(className);
  }
}

void vtkDGRenderResponder::AddMods(const std::vector<std::string>& classNames)
{
  for (const auto& modName : classNames)
  {
    this->AddMod(modName);
  }
}

void vtkDGRenderResponder::RemoveMod(const std::string& className)
{
  if (this->ModNamesUnique.count(className))
  {
    this->ModNamesUnique.erase(className);
    this->ModNames.erase(
      std::remove(this->ModNames.begin(), this->ModNames.end(), className), this->ModNames.end());
  }
}

void vtkDGRenderResponder::RemoveAllMods()
{
  this->ModNamesUnique.clear();
  this->ModNames.clear();
}

bool vtkDGRenderResponder::Query(
  vtkCellGridRenderRequest* request, vtkCellMetadata* metadata, vtkCellGridResponders* caches)
{
  (void)caches;

  if (request->GetIsReleasingResources())
  {
    return this->ReleaseResources(request, metadata);
  }

  return this->DrawCells(request, metadata);
}

bool vtkDGRenderResponder::DrawCells(vtkCellGridRenderRequest* request, vtkCellMetadata* metadata)
{
  bool didDraw = false;
  auto* dgCell = dynamic_cast<vtkDGCell*>(metadata);
  if (!dgCell || !dgCell->GetNumberOfCells())
  {
    return didDraw;
  }

  // Find or create cached vtkDrawTexturedElement objects, {Di}.
  // Update Di as needed (when the following have changed):
  //   + cell metadata or involved arrays have been modified since last render.
  //   + render request has been modified since last render.
  // Invoke render method on Di

  // Iterate over CellSpec and SideSpecs, drawing each of them
  // that is well-defined, unblanked, and (TODO) requested.
  if (dgCell->GetCellSpec().Connectivity && !dgCell->GetCellSpec().Blanked)
  {
    didDraw |= this->DrawShapes(request, dgCell, dgCell->GetCellSpec());
  }
  for (const auto& sideSpec : dgCell->GetSideSpecs())
  {
    if (sideSpec.Connectivity && !sideSpec.Blanked)
    {
      didDraw |= this->DrawShapes(request, dgCell, sideSpec);
    }
  }

  return didDraw;
}

bool vtkDGRenderResponder::ReleaseResources(
  vtkCellGridRenderRequest* request, vtkCellMetadata* metadata)
{
  // Destroy the cache entry when its resources are released.
  // If we don't do this, objects it references (e.g., vtkDrawTexturedElements::ColorTextureGL)
  // will live beyond the scope of information keys those objects refer to.
  auto curr = this->Helpers.begin();
  for (auto next = curr; curr != this->Helpers.end(); curr = next)
  {
    ++next;
    if (curr->CellType == metadata)
    {
      curr->RenderHelper->ReleaseResources(request->GetWindow());
      this->Helpers.erase(curr);
    }
  }
  // TODO
  return true;
}

bool vtkDGRenderResponder::DrawShapes(
  vtkCellGridRenderRequest* request, vtkDGCell* metadata, const vtkDGCell::Source& shape)
{
  if (vtkDGCell::GetShapeDimension(shape.SourceShape) > 2)
  {
    // Do not attempt to render any volumetric shape directly.
    return false;
  }
#ifdef vtkDGRenderResponder_DEBUG
  bool justPrepared = false;
#endif // vtkDGRenderResponder_DEBUG
  CacheEntry dummy;
  dummy.CellType = metadata;
  dummy.CellSource = &shape;
  dummy.Shape = metadata->GetCellGrid()->GetShapeAttribute();
  dummy.ModNames = this->ModNames;
  auto* actor = request->GetActor();
  auto* renderer = request->GetRenderer();
  auto* mapper = request->GetMapper();
  if (mapper && mapper->GetScalarVisibility() &&
    (mapper->GetScalarMode() == VTK_SCALAR_MODE_USE_CELL_FIELD_DATA))
  {
    auto* attributeName = mapper->GetArrayName();
    if (attributeName && attributeName[0])
    {
      dummy.Color = metadata->GetCellGrid()->GetCellAttributeByName(attributeName);
    }
  }
  auto cacheEntryIt = this->Helpers.find(dummy);
  // For now, if the cache entry is stale, just delete it.
  if (cacheEntryIt != this->Helpers.end() &&
    !cacheEntryIt->IsUpToDate(renderer, actor, mapper, /*debugAttachment=*/this))
  {
    // std::cout << "Cache hit: but entry is outdated" << std::endl;
    this->Helpers.erase(cacheEntryIt);
    cacheEntryIt = this->Helpers.end();
  }
  if (cacheEntryIt == this->Helpers.end())
  {
    // Insert and prepare the helper we created for the search.
    cacheEntryIt = this->Helpers.emplace(std::move(dummy)).first;
    cacheEntryIt->PrepareHelper(renderer, actor, mapper);
#ifdef vtkDGRenderResponder_DEBUG
    justPrepared = true;
#endif // vtkDGRenderResponder_DEBUG
  }
  if (cacheEntryIt == this->Helpers.end())
  {
    // We couldn't prepare a helper.
    return false;
  }
  // Now we can render.
  // TODO: Do not render if translucent during opaque pass or vice-versa.
  cacheEntryIt->RenderHelper->DrawInstancedElements(renderer, actor, mapper);
#ifdef vtkDGRenderResponder_DEBUG
  if (justPrepared)
  {
    std::cout << "***\n***\n  shader " << cacheEntryIt->RenderHelper->GetShaderProgram() << " "
              << cacheEntryIt->CellType->GetClassName() << " "
              << vtkDGCell::GetShapeName(shape.SourceShape).Data() << " " << &shape << "\n***\n";

    std::ostringstream dbgName;
    dbgName << "/tmp/dbg-" << metadata->GetClassName() << "-"
            << vtkDGCell::GetShapeName(shape.SourceShape).Data() << "-"
            << request->GetMapper()->GetArrayName() << "-";
    cacheEntryIt->RenderHelper->GetShaderProgram()->SetFileNamePrefixForDebugging(
      dbgName.str().c_str());
  }
#endif
  return true;
}

vtkDrawTexturedElements::ElementShape vtkDGRenderResponder::PrimitiveFromShape(
  vtkDGCell::Shape shape)
{
  switch (shape)
  {
    case vtkDGCell::Shape::Hexahedron:
    case vtkDGCell::Shape::Quadrilateral:
      return vtkDrawTexturedElements::ElementShape::TriangleFan;
    case vtkDGCell::Shape::Tetrahedron:
    case vtkDGCell::Shape::Triangle:
      return vtkDrawTexturedElements::ElementShape::Triangle;
    case vtkDGCell::Shape::Edge:
      return vtkDrawTexturedElements::ElementShape::Line;
    case vtkDGCell::Shape::Vertex:
    default:
      return vtkDrawTexturedElements::ElementShape::Point;
  }
}

VTK_ABI_NAMESPACE_END

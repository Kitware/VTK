// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkOpenGLLowMemoryPolyDataMapper.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCollectionIterator.h"
#include "vtkExecutive.h"
#include "vtkFloatArray.h"
#include "vtkGLSLModCamera.h"
#include "vtkGLSLModCoincidentTopology.h"
#include "vtkGLSLModLight.h"
#include "vtkGLSLModifierBase.h"
#include "vtkGLSLModifierFactory.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkLookupTable.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLLowMemoryLinesAgent.h"
#include "vtkOpenGLLowMemoryPolygonsAgent.h"
#include "vtkOpenGLLowMemoryStripsAgent.h"
#include "vtkOpenGLLowMemoryVerticesAgent.h"
#include "vtkOpenGLRenderPass.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLShaderDeclaration.h"
#include "vtkOpenGLShaderProperty.h"
#include "vtkOpenGLState.h"
#include "vtkOpenGLVertexBufferObject.h"
#include "vtkPBRFunctions.h"
#include "vtkPointData.h"
#include "vtkPolyDataFS.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataVS.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkScalarsToColors.h"
#include "vtkSetGet.h"
#include "vtkShaderProgram.h"
#include "vtkTextureObject.h"
#include "vtkTransform.h"
#include "vtkUnsignedIntArray.h"
#include "vtk_glew.h"

#include <limits>
#include <sstream>

VTK_ABI_NAMESPACE_BEGIN

// Uncomment to print shader/color info to cout
// #define vtkOpenGLLowMemoryPolyDataMapper_DEBUG
namespace
{
//------------------------------------------------------------------------------
// helper to get the state of picking
int getPickState(vtkRenderer* ren)
{
  vtkHardwareSelector* selector = ren->GetSelector();
  if (selector)
  {
    return selector->GetCurrentPass();
  }

  return vtkHardwareSelector::MIN_KNOWN_PASS - 1;
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkOpenGLLowMemoryPolyDataMapper);

//------------------------------------------------------------------------------
vtkOpenGLLowMemoryPolyDataMapper::vtkOpenGLLowMemoryPolyDataMapper()
{
  // turns off color map textures from vtkDrawTexturedElements. (we use our own)
  this->IncludeColormap = false;

  // Initialize agent and primitive generator function for all primitives (verts, lines, polys and
  // strips)
  this->Primitives[0].Agent.reset(new vtkOpenGLLowMemoryVerticesAgent());
  this->Primitives[0].GeneratorFunction = &(vtkCellGraphicsPrimitiveMap::ProcessVertices);

  this->Primitives[1].Agent.reset(new vtkOpenGLLowMemoryLinesAgent());
  this->Primitives[1].GeneratorFunction = &(vtkCellGraphicsPrimitiveMap::ProcessLines);

  this->Primitives[2].Agent.reset(new vtkOpenGLLowMemoryPolygonsAgent());
  this->Primitives[2].GeneratorFunction = &(vtkCellGraphicsPrimitiveMap::ProcessPolygons);

  this->Primitives[3].Agent.reset(new vtkOpenGLLowMemoryStripsAgent());
  this->Primitives[3].GeneratorFunction = &(vtkCellGraphicsPrimitiveMap::ProcessStrips);

  // Reset list of mods
  this->ResetModsToDefault();

  // Ensure the following tokens have strings in the dictionary so vtkStringToken::Data()
  // will be able to return them.
  using namespace vtk::literals;
  vtkStringToken positions = "positions";
  vtkStringToken colors = "colors";
  vtkStringToken pointNormals = "pointNormals";
  vtkStringToken tangents = "tangents";
  vtkStringToken tcoords = "tcoords";
  vtkStringToken colorTCoords = "colorTCoords";
  vtkStringToken cellNormals = "cellNormals";
  vtkStringToken vertexIdBuffer = "vertexIdBuffer";
  vtkStringToken primitiveToCellBuffer = "primitiveToCellBuffer";
  vtkStringToken edgeValueBuffer = "edgeValueBuffer";
  vtkStringToken cellIdOffset = "cellIdOffset";
  vtkStringToken vertexIdOffset = "vertexIdOffset";
  vtkStringToken edgeValueBufferOffset = "edgeValueBufferOffset";
  vtkStringToken pointIdOffset = "pointIdOffset";
  vtkStringToken primitiveIdOffset = "primitiveIdOffset";
  vtkStringToken cellType = "cellType";
  vtkStringToken usesCellMap = "usesCellMap";

  (void)positions;
  (void)colors;
  (void)pointNormals;
  (void)tangents;
  (void)tcoords;
  (void)colorTCoords;
  (void)cellNormals;
  (void)vertexIdBuffer;
  (void)primitiveToCellBuffer;
  (void)edgeValueBuffer;
  (void)cellIdOffset;
  (void)vertexIdOffset;
  (void)edgeValueBufferOffset;
  (void)pointIdOffset;
  (void)primitiveIdOffset;
  (void)cellType;
  (void)usesCellMap;
}

//------------------------------------------------------------------------------
vtkOpenGLLowMemoryPolyDataMapper::~vtkOpenGLLowMemoryPolyDataMapper()
{
  if (this->InternalColorTexture)
  {
    this->InternalColorTexture->Delete();
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryPolyDataMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  return this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryPolyDataMapper::ShallowCopy(vtkAbstractMapper* mapper)
{
  vtkOpenGLLowMemoryPolyDataMapper* m = vtkOpenGLLowMemoryPolyDataMapper::SafeDownCast(mapper);
  if (m != nullptr)
  {
    this->SetPointIdArrayName(m->GetPointIdArrayName());
    this->SetCompositeIdArrayName(m->GetCompositeIdArrayName());
    this->SetProcessIdArrayName(m->GetProcessIdArrayName());
    this->SetCellIdArrayName(m->GetCellIdArrayName());
  }

  // Now do superclass
  this->vtkPolyDataMapper::ShallowCopy(mapper);
}

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryPolyDataMapper::ReleaseGraphicsResources(vtkWindow* window)
{
  if (this->InternalColorTexture)
  {
    this->InternalColorTexture->ReleaseGraphicsResources(window);
  }
  this->ReleaseResources(window);
}

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryPolyDataMapper::SetVBOShiftScaleMethod(int method)
{
  if (this->ShiftScaleMethod == method)
  {
    return;
  }
  this->ShiftScaleMethod = method;
}

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryPolyDataMapper::AddMod(const std::string& className)
{
  if (!this->ModNamesUnique.count(className))
  {
    this->ModNames.emplace_back(className);
    this->ModNamesUnique.insert(className);
  }
}

//------------------------------------------------------------------------------
// When new default mods are added, make sure to register them in
// vtkDGRenderResponder::ResetModsToDefault below.
std::vector<std::string> vtkOpenGLLowMemoryPolyDataMapper::DefaultModNames = { "vtkGLSLModCamera",
  "vtkGLSLModLight", "vtkGLSLModCoincidentTopology" };

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryPolyDataMapper::ResetModsToDefault()
{
  // just to be sure.
  this->RemoveAllMods();
  this->AddMods(vtkOpenGLLowMemoryPolyDataMapper::DefaultModNames);
  vtkGLSLModifierFactory::RegisterAMod(
    DefaultModNames[0], [](void*) { return vtkGLSLModCamera::New(); });
  vtkGLSLModifierFactory::RegisterAMod(
    DefaultModNames[1], [](void*) { return vtkGLSLModLight::New(); });
  vtkGLSLModifierFactory::RegisterAMod(
    DefaultModNames[2], [](void*) { return vtkGLSLModCoincidentTopology::New(); });
}

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryPolyDataMapper::AddMods(const std::vector<std::string>& classNames)
{
  for (const auto& modName : classNames)
  {
    this->AddMod(modName);
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryPolyDataMapper::RemoveMod(const std::string& className)
{
  if (this->ModNamesUnique.count(className))
  {
    this->ModNamesUnique.erase(className);
    this->ModNames.erase(
      std::remove(this->ModNames.begin(), this->ModNames.end(), className), this->ModNames.end());
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryPolyDataMapper::RemoveAllMods()
{
  this->ModNamesUnique.clear();
  this->ModNames.clear();
}

//------------------------------------------------------------------------------
bool vtkOpenGLLowMemoryPolyDataMapper::IsUpToDate(vtkRenderer*, vtkActor*)
{
  if (this->RenderTimeStamp < this->GetMTime())
  {
    return false;
  }
  if (this->RenderTimeStamp < this->ShiftScaleTimeStamp)
  {
    return false;
  }
  if (!this->IsDataObjectUpToDate())
  {
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkOpenGLLowMemoryPolyDataMapper::IsDataObjectUpToDate()
{
  return this->RenderTimeStamp > this->CurrentInput->GetMTime();
}

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryPolyDataMapper::RenderPiece(vtkRenderer* renderer, vtkActor* actor)
{
  // Make sure that we have been properly initialized.
  if (renderer->GetRenderWindow()->CheckAbortStatus())
  {
    return;
  }
  this->CurrentInput = this->GetInput();

  if (this->CurrentInput == nullptr)
  {
    vtkErrorMacro(<< "No input!");
    return;
  }
  // Update upstream algorithm if we're not static.
  this->InvokeEvent(vtkCommand::StartEvent, nullptr);
  if (!this->Static)
  {
    vtkDebugMacro(<< "Updating upstream algorithm.");
    this->GetInputAlgorithm()->Update();
  }
  this->InvokeEvent(vtkCommand::EndEvent, nullptr);
  // if there are no points then we are done
  if (!this->CurrentInput->GetPoints())
  {
    vtkDebugMacro(<< "There are no points on the input mesh.");
    return;
  }
  this->ComputeCameraBasedShiftScale(renderer, actor, this->CurrentInput);
  this->ComputeShiftScaleTransform(renderer, actor);
  this->RenderPieceStart(renderer, actor);
  this->RenderPieceDraw(renderer, actor);
  this->RenderPieceFinish(renderer, actor);
}

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryPolyDataMapper::RenderPieceStart(vtkRenderer* renderer, vtkActor* actor)
{
  if (!this->IsUpToDate(renderer, actor))
  {
    this->DeleteTextureBuffers();
    vtkCellGraphicsPrimitiveMap::CellTypeMapperOffsets offsets;
    this->UpdateShiftScale(renderer, actor);
    this->ComputeShiftScaleTransform(renderer, actor);
    this->BindArraysToTextureBuffers(renderer, actor, offsets);
    // remove all shader declarations.
    this->ShaderDecls.clear();
    this->InstallArrayTextureShaderDeclarations();
    if (!this->IsShaderColorSourceUpToDate(actor))
    {
      this->ShaderProgram = nullptr;
    }
  }
  int picking = getPickState(renderer);
  if (this->LastSelectionState != picking)
  {
    this->SelectionStateTimeStamp.Modified();
    this->LastSelectionState = picking;
  }
  // render points for point picking in a special way
  // all cell types should be rendered as points
  vtkHardwareSelector* selector = renderer->GetSelector();
  this->PointPicking = false;
  if (selector && selector->GetFieldAssociation() == vtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    this->PointPicking = true;
  }
  if (selector && selector->GetFieldAssociation() == vtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    static_cast<vtkOpenGLRenderer*>(renderer)->GetState()->vtkglDepthMask(GL_FALSE);
  }
  if (selector && this->PopulateSelectionSettings)
  {
    selector->BeginRenderProp();
    if (selector->GetCurrentPass() == vtkHardwareSelector::COMPOSITE_INDEX_PASS)
    {
      selector->RenderCompositeIndex(1);
    }

    this->UpdateMaximumPointCellIds(renderer, actor);
  }
  this->UpdatePBRStateCache(renderer, actor);
  if (!this->IsShaderUpToDate(renderer, actor))
  {
    this->UpdateShaders(renderer, actor);
    this->ShaderBuildTimeStamp.Modified();
  }
  this->UpdateGLSLMods(renderer, actor);
  // If we are coloring by texture, then load the texture map.
  // Use Map as indicator, because texture hangs around.
  if (this->ColorTextureMap)
  {
    this->InternalColorTexture->Load(renderer);
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryPolyDataMapper::RenderPieceDraw(vtkRenderer* renderer, vtkActor* actor)
{
  this->ReadyShaderProgram(renderer);
  this->SetShaderParameters(renderer, actor);
  for (auto& primitive : this->Primitives)
  {
    auto& agent = primitive.Agent;
    agent->PreDraw(renderer, actor, this);
    agent->Draw(renderer, actor, this, primitive.CellGroups);
    agent->PostDraw(renderer, actor, this);
    // vertex visibility pass
    if (actor->GetProperty()->GetVertexVisibility() && agent->ImplementsVertexVisibilityPass())
    {
      this->DrawingVertices = true; // should we UpdateShader now? it could be slow.
      agent->BeginVertexVisibilityPass();
      agent->PreDraw(renderer, actor, this);
      agent->Draw(renderer, actor, this, primitive.CellGroups);
      agent->PostDraw(renderer, actor, this);
      agent->EndVertexVisibilityPass();
      this->DrawingVertices = false;
    }
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryPolyDataMapper::RenderPieceFinish(vtkRenderer* renderer, vtkActor*)
{
  vtkHardwareSelector* selector = renderer->GetSelector();
  // render points for point picking in a special way
  if (selector && selector->GetFieldAssociation() == vtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    static_cast<vtkOpenGLRenderer*>(renderer)->GetState()->vtkglDepthMask(GL_TRUE);
  }
  if (selector && this->PopulateSelectionSettings)
  {
    selector->EndRenderProp();
  }
  if (this->ColorTextureMap)
  {
    this->InternalColorTexture->PostRender(renderer);
  }
  this->RenderTimeStamp.Modified();
}

//------------------------------------------------------------------------------
vtkPolyDataMapper::MapperHashType vtkOpenGLLowMemoryPolyDataMapper::GenerateHash(
  vtkPolyData* polydata)
{
  int cellFlag = 0;
  vtkAbstractArray* scalars = this->GetAbstractScalars(
    polydata, this->ScalarMode, this->ArrayAccessMode, this->ArrayId, this->ArrayName, cellFlag);
  bool hasScalars = this->ScalarVisibility && (scalars != nullptr);
  bool hasPointScalars = hasScalars && !cellFlag;
  bool hasCellScalars = hasScalars && cellFlag == 1;

  bool usesPointNormals = polydata->GetPointData()->GetNormals() != nullptr;
  bool usesPointTexCoords = polydata->GetPointData()->GetTCoords() != nullptr;
  bool usesPointColorsWithTextureMaps =
    this->CanUseTextureMapForColoring(polydata) && hasPointScalars;
  bool usesPointColors = !usesPointColorsWithTextureMaps && hasPointScalars;
  bool usesCellColorTexture = !usesPointColorsWithTextureMaps && !usesPointColors && hasCellScalars;
  bool usesCellNormalTexture =
    !usesPointNormals && (polydata->GetCellData()->GetNormals() != nullptr);

  // The hash is seeded from the address of the lookup table.
  // WARNING: Technically, hash will overflow when
  //  &(lut) >= max_n_bit_ptr_address - 126, where n == 32 or n == 64
  MapperHashType hash = 0;
  // Get the lookup table.
  vtkDataArray* dataArray = vtkArrayDownCast<vtkDataArray>(scalars);
  vtkScalarsToColors* lut = nullptr;
  if (dataArray && dataArray->GetLookupTable())
  {
    lut = vtkScalarsToColors::SafeDownCast(dataArray->GetLookupTable());
  }
  else
  {
    lut = this->LookupTable;
  }
  hash = std::hash<MapperHashType>{}(reinterpret_cast<std::uintptr_t>(lut));
  hash += (usesPointColors << 1);
  hash += (usesPointNormals << 2);
  hash += (usesPointTexCoords << 3);
  hash += (usesPointColorsWithTextureMaps << 4);
  hash += (usesCellColorTexture << 5);
  hash += (usesCellNormalTexture << 6);
#ifdef vtkOpenGLLowMemoryPolyDataMapper_DEBUG
  std::cout << "hash: " << hash << " for " << polydata << '\n';
#endif
  return hash;
}

//------------------------------------------------------------------------------
bool vtkOpenGLLowMemoryPolyDataMapper::BindArraysToTextureBuffers(
  vtkRenderer*, vtkActor*, vtkCellGraphicsPrimitiveMap::CellTypeMapperOffsets& offsets)
{
  auto mesh = this->CurrentInput;
  if (!mesh)
  {
    vtkWarningMacro(<< this->GetObjectDescription() << " does not have a vtkPolyData input.");
    return false;
  }
  using namespace vtk::literals;

  const auto numPoints = mesh->GetNumberOfPoints();
  const auto numCells = mesh->GetNumberOfCells();
  vtkSmartPointer<vtkDataArray> positions;
  positions = mesh->GetPoints()->GetData();
  if (this->CoordinateShiftAndScaleInUse)
  {
    vtkNew<vtkFloatArray> ssArray;
    ssArray->SetNumberOfComponents(positions->GetNumberOfComponents());
    ssArray->SetNumberOfTuples(positions->GetNumberOfTuples());
    for (vtkIdType i = 0; i < positions->GetNumberOfTuples(); ++i)
    {
      for (int c = 0; c < positions->GetNumberOfComponents(); ++c)
      {
        ssArray->SetComponent(
          i, c, (positions->GetComponent(i, c) - this->ShiftValues[c]) * this->ScaleValues[c]);
      }
    }
    positions = ssArray;
  }
  if (!positions)
  {
    vtkErrorMacro(<< "positions is null!");
    return false;
  }
  auto colors = this->GetColors(mesh);
  auto pointNormals = this->GetPointNormals(mesh);
  auto tangents = this->GetPointTangents(mesh);
  auto tCoords = this->GetTextureCoordinates(mesh);
  auto colorTCoords = this->GetColorTextureCoordinates(mesh);
  auto cellNormals = this->GetCellNormals(mesh);
  // If we are coloring by texture, then load the texture map.
  if (this->ColorTextureMap)
  {
    if (this->InternalColorTexture == nullptr)
    {
      this->InternalColorTexture = vtkOpenGLTexture::New();
      this->InternalColorTexture->RepeatOff();
    }
    this->InternalColorTexture->SetInputData(this->ColorTextureMap);
  }
  // 1. bind positions
  this->AppendArrayToTexture("positions"_token, positions);
  // 2, bind colors
  if (colors &&
    (colors->GetNumberOfTuples() == numPoints || colors->GetNumberOfTuples() == numCells))
  {
    this->AppendArrayToTexture("colors"_token, colors);
    this->HasColors = true;
  }
  // 3. bind pointNormals
  if (pointNormals && pointNormals->GetNumberOfTuples() == numPoints)
  {
    this->AppendArrayToTexture("pointNormals"_token, pointNormals);
    this->HasPointNormals = true;
  }
  // 3. bind tangents
  if (tangents && tangents->GetNumberOfTuples() == numPoints)
  {
    this->AppendArrayToTexture("tangents"_token, tangents);
    this->HasTangents = true;
  }
  // 4. bind tcoords
  if (tCoords && tCoords->GetNumberOfTuples() == numPoints)
  {
    this->AppendArrayToTexture("tcoords"_token, tCoords);
    this->HasPointTextureCoordinates = true;
  }
  // 5. bind colorTCoords
  if (colorTCoords && colorTCoords->GetNumberOfTuples() == numPoints)
  {
    this->AppendArrayToTexture("colorTCoords"_token, colorTCoords);
  }
  // 6. bind cellNormals
  if (cellNormals && cellNormals->GetNumberOfTuples() == numCells)
  {
    this->AppendArrayToTexture("cellNormals"_token, cellNormals);
    this->HasCellNormals = true;
  }
  // 7. Compute primitive indices.
  for (auto& primitive : this->Primitives)
  {
    auto& cellGroups = primitive.CellGroups;
    // can this cell group be rendered?
    const auto primDesc = primitive.GeneratorFunction(mesh);
    CellGroupInformation cellGroup;
    if ((primDesc.VertexIDs == nullptr) || (primDesc.VertexIDs->GetNumberOfValues() == 0))
    {
      cellGroup.CanRender = false;
      cellGroups.emplace_back(cellGroup);
      continue;
    }
    cellGroup.CanRender = true;
    // bind the vertex indices. this buffer holds the point ids which index into
    // polydata->GetPoints()
    this->AppendArrayToTexture("vertexIdBuffer"_token, primDesc.VertexIDs);
    if ((primDesc.PrimitiveToCell != nullptr) &&
      (primDesc.PrimitiveToCell->GetNumberOfValues() > 0))
    {
      // bind the cell map. this buffer holds the cell ids per graphics primitive.
      this->AppendArrayToTexture("primitiveToCellBuffer"_token, primDesc.PrimitiveToCell);
      cellGroup.NumberOfElements = primDesc.PrimitiveToCell->GetNumberOfValues();
      cellGroup.UsesCellMapBuffer = true;
    }
    else
    {
      // fast low memory path! no need for a cell map. it is implicitly calculated from
      // cellIdOffset.
      auto placeholder = vtk::TakeSmartPointer(vtkTypeInt32Array::New());
      placeholder->SetNumberOfComponents(1);
      placeholder->InsertNextValue(0);
      this->AppendArrayToTexture("primitiveToCellBuffer"_token, placeholder);
      cellGroup.NumberOfElements = primDesc.VertexIDs->GetNumberOfValues() / primDesc.PrimitiveSize;
      cellGroup.UsesCellMapBuffer = false;
    }
    if ((primDesc.EdgeArray != nullptr) && (primDesc.EdgeArray->GetNumberOfValues() > 0))
    {
      // edgeValues need to be used to mask out edges of the triangles inside a polygon.
      this->AppendArrayToTexture("edgeValueBuffer"_token, primDesc.EdgeArray);
      cellGroup.UsesEdgeValueBuffer = true;
    }
    else
    {
      // fast low memory path! no need for edge values because all the polygons are triangles.
      auto placeholder = vtk::TakeSmartPointer(vtkTypeUInt8Array::New());
      placeholder->SetNumberOfComponents(1);
      placeholder->InsertNextValue(0);
      this->AppendArrayToTexture("edgeValueBuffer"_token, placeholder);
      cellGroup.UsesEdgeValueBuffer = false;
    }
    // apply local values on top of global offsets.
    auto& cgOffsets = cellGroup.Offsets;
    cgOffsets.CellIdOffset = offsets.CellIdOffset + primDesc.LocalCellIdOffset;
    cgOffsets.PointIdOffset = offsets.PointIdOffset;
    cgOffsets.VertexIdOffset = offsets.VertexIdOffset;
    cgOffsets.EdgeValueBufferOffset = offsets.EdgeValueBufferOffset;
    cgOffsets.PrimitiveIdOffset = offsets.PrimitiveIdOffset;

    // bump global offsets to the end of current cell group.
    offsets.VertexIdOffset += primDesc.VertexIDs->GetNumberOfValues();
    // compensate for 1 placeholder element or all number of elements.
    offsets.PrimitiveIdOffset += cellGroup.UsesCellMapBuffer ? cellGroup.NumberOfElements : 1;
    offsets.EdgeValueBufferOffset += cellGroup.UsesEdgeValueBuffer ? cellGroup.NumberOfElements : 1;
    // store the information of this particular cell group for use at the time of draw.
    cellGroups.emplace_back(cellGroup);
  }
  // bump pointIdOffset to the end of current mesh.
  offsets.PointIdOffset += mesh->GetNumberOfPoints();
  offsets.CellIdOffset += mesh->GetNumberOfCells();

  // Handle extra attributes.
  for (auto& itr : this->ExtraAttributes)
  {
    vtkDataArray* da = mesh->GetPointData()->GetArray(itr.second.DataArrayName.c_str());
    this->AppendArrayToTexture(vtkStringToken(itr.first), da);
  }
  return true;
}

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryPolyDataMapper::InstallArrayTextureShaderDeclarations()
{
  using namespace vtk::literals;
  this->ShaderDecls.emplace_back(
    /*qualifier=*/GLSLQualifierType::Uniform,
    /*precision=*/GLSLPrecisionType::High,
    /*dataType=*/GLSLDataType::Float,
    /*attributeType=*/GLSLAttributeType::SamplerBuffer,
    /*variableName=*/"positions"_token);
  if (this->HasColors)
  {
    this->ShaderDecls.emplace_back(
      /*qualifier=*/GLSLQualifierType::Uniform,
      /*precision=*/GLSLPrecisionType::Low,
      /*dataType=*/GLSLDataType::Unsigned,
      /*attributeType=*/GLSLAttributeType::SamplerBuffer,
      /*variableName=*/"colors"_token);
  }
  if (this->HasPointNormals)
  {
    this->ShaderDecls.emplace_back(
      /*qualifier=*/GLSLQualifierType::Uniform,
      /*precision=*/GLSLPrecisionType::High,
      /*dataType=*/GLSLDataType::Float,
      /*attributeType=*/GLSLAttributeType::SamplerBuffer,
      /*variableName=*/"pointNormals"_token);
  }
  if (this->HasTangents)
  {
    this->ShaderDecls.emplace_back(
      /*qualifier=*/GLSLQualifierType::Uniform,
      /*precision=*/GLSLPrecisionType::High,
      /*dataType=*/GLSLDataType::Float,
      /*attributeType=*/GLSLAttributeType::SamplerBuffer,
      /*variableName=*/"tangents"_token);
  }
  if (this->HasCellNormals)
  {
    this->ShaderDecls.emplace_back(
      /*qualifier=*/GLSLQualifierType::Uniform,
      /*precision=*/GLSLPrecisionType::High,
      /*dataType=*/GLSLDataType::Float,
      /*attributeType=*/GLSLAttributeType::SamplerBuffer,
      /*variableName=*/"cellNormals"_token);
  }
  this->ShaderDecls.emplace_back(
    /*qualifier=*/GLSLQualifierType::Uniform,
    /*precision=*/GLSLPrecisionType::High,
    /*dataType=*/GLSLDataType::Integer,
    /*attributeType=*/GLSLAttributeType::SamplerBuffer,
    /*variableName=*/"vertexIdBuffer"_token);
  this->ShaderDecls.emplace_back(
    /*qualifier=*/GLSLQualifierType::Uniform,
    /*precision=*/GLSLPrecisionType::High,
    /*dataType=*/GLSLDataType::Integer,
    /*attributeType=*/GLSLAttributeType::SamplerBuffer,
    /*variableName=*/"primitiveToCellBuffer"_token);
  this->ShaderDecls.emplace_back(
    /*qualifier=*/GLSLQualifierType::Uniform,
    /*precision=*/GLSLPrecisionType::Low,
    /*dataType=*/GLSLDataType::Unsigned,
    /*attributeType=*/GLSLAttributeType::SamplerBuffer,
    /*variableName=*/"edgeValueBuffer"_token);
  this->ShaderDecls.emplace_back(
    /*qualifier=*/GLSLQualifierType::Uniform,
    /*precision=*/GLSLPrecisionType::High,
    /*dataType=*/GLSLDataType::Integer,
    /*attributeType=*/GLSLAttributeType::Scalar,
    /*variableName=*/"cellIdOffset"_token);
  this->ShaderDecls.emplace_back(
    /*qualifier=*/GLSLQualifierType::Uniform,
    /*precision=*/GLSLPrecisionType::High,
    /*dataType=*/GLSLDataType::Integer,
    /*attributeType=*/GLSLAttributeType::Scalar,
    /*variableName=*/"vertexIdOffset"_token);
  this->ShaderDecls.emplace_back(
    /*qualifier=*/GLSLQualifierType::Uniform,
    /*precision=*/GLSLPrecisionType::High,
    /*dataType=*/GLSLDataType::Integer,
    /*attributeType=*/GLSLAttributeType::Scalar,
    /*variableName=*/"edgeValueBufferOffset"_token);
  this->ShaderDecls.emplace_back(
    /*qualifier=*/GLSLQualifierType::Uniform,
    /*precision=*/GLSLPrecisionType::High,
    /*dataType=*/GLSLDataType::Integer,
    /*attributeType=*/GLSLAttributeType::Scalar,
    /*variableName=*/"pointIdOffset"_token);
  this->ShaderDecls.emplace_back(
    /*qualifier=*/GLSLQualifierType::Uniform,
    /*precision=*/GLSLPrecisionType::High,
    /*dataType=*/GLSLDataType::Integer,
    /*attributeType=*/GLSLAttributeType::Scalar,
    /*variableName=*/"primitiveIdOffset"_token);
  this->ShaderDecls.emplace_back(
    /*qualifier=*/GLSLQualifierType::Uniform,
    /*precision=*/GLSLPrecisionType::High,
    /*dataType=*/GLSLDataType::Integer,
    /*attributeType=*/GLSLAttributeType::Scalar,
    /*variableName=*/"cellType"_token);
  this->ShaderDecls.emplace_back(
    /*qualifier=*/GLSLQualifierType::Uniform,
    /*precision=*/GLSLPrecisionType::High,
    /*dataType=*/GLSLDataType::Integer,
    /*attributeType=*/GLSLAttributeType::Scalar,
    /*variableName=*/"usesCellMap"_token);
}

//------------------------------------------------------------------------------
bool vtkOpenGLLowMemoryPolyDataMapper::IsShaderUpToDate(vtkRenderer* renderer, vtkActor* actor)
{
  // Have the renderpasses changed?
  if (this->ShaderBuildTimeStamp < ::GetRenderPassStageMTime(actor, this->LastRenderPassInfo))
  {
    vtkDebugMacro(<< "RenderPassStage is outdated");
    return false;
  }
  // Have the mods changed?
  auto modsIter = vtk::TakeSmartPointer(this->GetGLSLModCollection()->NewIterator());
  auto oglRen = static_cast<vtkOpenGLRenderer*>(renderer);
  for (modsIter->InitTraversal(); !modsIter->IsDoneWithTraversal(); modsIter->GoToNextItem())
  {
    auto mod = static_cast<vtkGLSLModifierBase*>(modsIter->GetCurrentObject());
    if (!mod->IsUpToDate(oglRen, this, actor))
    {
      vtkDebugMacro(<< mod->GetClassName() << " is outdated");
      // if any mod is outdated, entire shader program must be re-compiled.
      return false;
    }
  }
  // Have the normal sources changed?
  if (!this->IsShaderNormalSourceUpToDate(actor))
  {
    return false;
  }
  // has the shader program previously been nullified and it needs to be rebuilt?
  if (this->ShaderProgram == nullptr)
  {
    return false;
  }
  // has the selection state changed?
  if (this->SelectionStateTimeStamp > this->ShaderBuildTimeStamp)
  {
    return false;
  }
  // has the pbr state changed?
  if (this->PBRStateTimeStamp > this->ShaderBuildTimeStamp)
  {
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryPolyDataMapper::DeleteTextureBuffers()
{
  // remove all arrays that we may've bound.
  using namespace vtk::literals;
  for (auto& arrayToken : { "positions"_token, "colors"_token, "pointNormals"_token,
         "tangents"_token, "tcoords"_token, "colorTCoords"_token, "cellNormals"_token,
         "vertexIdBuffer"_token, "primitiveToCellBuffer"_token, "edgeValueBuffer"_token })
  {
    this->Arrays.erase(arrayToken);
  }
  for (auto& itr : this->ExtraAttributes)
  {
    this->Arrays.erase(vtkStringToken(itr.first));
  }
  // reset cell groups
  for (auto& primitive : this->Primitives)
  {
    primitive.CellGroups.clear();
  }
  // reset cache information about the samplerbuffers
  this->HasColors = false;
  this->HasPointNormals = false;
  this->HasTangents = false;
  this->HasPointTextureCoordinates = false;
  this->HasCellNormals = false;
}

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryPolyDataMapper::UpdateShaders(vtkRenderer* renderer, vtkActor* actor)
{
  // remove existing mods.
  this->GLSLMods->RemoveAllItems();

  auto vertShader = this->GetShader(vtkShader::Vertex);
  auto fragShader = this->GetShader(vtkShader::Fragment);
  vertShader->SetSource(vtkPolyDataVS);
  fragShader->SetSource(vtkPolyDataFS);
  // user specified pre replacements
  vtkOpenGLShaderProperty* sp = vtkOpenGLShaderProperty::SafeDownCast(actor->GetShaderProperty());
  vtkOpenGLShaderProperty::ReplacementMap repMap = sp->GetAllShaderReplacements();
  for (const auto& i : repMap)
  {
    if (i.first.ReplaceFirst)
    {
      std::string ssrc = this->Shaders[i.first.ShaderType]->GetSource();
      vtkShaderProgram::Substitute(
        ssrc, i.first.OriginalValue, i.second.Replacement, i.second.ReplaceAll);
      this->Shaders[i.first.ShaderType]->SetSource(ssrc);
    }
  }
  auto vsSource = vertShader->GetSource();
  auto fsSource = fragShader->GetSource();
  this->ReplaceShaderValues(renderer, actor, vsSource, fsSource);
  vertShader->SetSource(vsSource);
  fragShader->SetSource(fsSource);
#ifdef vtkOpenGLLowMemoryPolyDataMapper_DEBUG
  std::cout << "VS: " << vsSource << std::endl;
  std::cout << "FS: " << fsSource << std::endl;
#endif
}

//------------------------------------------------------------------------------
bool vtkOpenGLLowMemoryPolyDataMapper::IsShaderColorSourceUpToDate(vtkActor*)
{
  auto mesh = this->CurrentInput;
  if (!mesh)
  {
    vtkWarningMacro(<< this->GetObjectDescription() << " does not have a vtkPolyData input.");
    return false;
  }
  auto colorSrc = this->DetermineShaderColorSource(mesh);
  // have the color source attribute changed? i.e, now it comes from pointdata instead of celldata?
  if (colorSrc != this->ShaderColorSource)
  {
    this->ShaderColorSource = colorSrc;
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkOpenGLLowMemoryPolyDataMapper::IsShaderNormalSourceUpToDate(vtkActor* actor)
{
  auto mesh = this->CurrentInput;
  if (!mesh)
  {
    vtkWarningMacro(<< this->GetObjectDescription() << " does not have a vtkPolyData input.");
    return false;
  }
  auto normalSrc = this->DetermineShaderNormalSource(actor, mesh);
  // have the normal source attribute changed?
  if (normalSrc != this->ShaderNormalSource)
  {
    this->ShaderNormalSource = normalSrc;
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryPolyDataMapper::ReplaceShaderValues(
  vtkRenderer* renderer, vtkActor* actor, std::string& vsSource, std::string& fsSource)
{
  // Pre-pass.
  std::string emptyGS;
  ::ReplaceShaderRenderPass(vsSource, emptyGS, fsSource, this, actor, true);
  this->ReplaceShaderPosition(renderer, actor, vsSource, fsSource);
  this->ReplaceShaderNormal(renderer, actor, vsSource, fsSource);
  this->ReplaceShaderColor(renderer, actor, vsSource, fsSource);
  this->ReplaceShaderImplementationCustomUniforms(renderer, actor, vsSource, fsSource);
  this->ReplaceShaderPointSize(renderer, actor, vsSource, fsSource);
  this->ReplaceShaderWideLines(renderer, actor, vsSource, fsSource);
  this->ReplaceShaderEdges(renderer, actor, vsSource, fsSource);
  this->ReplaceShaderSelection(renderer, actor, vsSource, fsSource);
  // encapsulate the whole light stuff inside an if clause.
  vtkShaderProgram::Substitute(fsSource, "//VTK::Light::Dec",
    "//VTK::Light::Dec\n"
    "uniform int enable_lights;\n");
  vtkShaderProgram::Substitute(fsSource, "//VTK::Light::Impl",
    "  gl_FragData[0] = vec4(ambientColor + diffuseColor, opacity);\n"
    "   if (enable_lights == 1)\n"
    "   {\n"
    "   //VTK::Light::Impl\n"
    "   }\n");
  auto oglRenderer = static_cast<vtkOpenGLRenderer*>(renderer);
  // Apply shader mods.
  for (const auto& modName : this->ModNames)
  {
    auto mod = vtk::TakeSmartPointer(vtkGLSLModifierFactory::CreateAMod(modName));
    if (auto lightMod = vtkGLSLModLight::SafeDownCast(mod))
    {
      // light mod needs additional information before it can replace shader values.
      this->UpdatePBRStateCache(renderer, actor);
      lightMod->SetUsePBRTextures(this->HasPointTextureCoordinates && !this->DrawingVertices);
      lightMod->SetUseAnisotropy(this->HasPointNormals && this->HasTangents && this->HasAnisotropy);
      lightMod->SetUseClearCoat(this->HasClearCoat);
    }
    mod->ReplaceShaderValues(oglRenderer, vsSource, emptyGS, fsSource, this, actor);
    this->GetGLSLModCollection()->AddItem(mod);
  }
  this->ReplaceShaderTCoord(renderer, actor, vsSource, fsSource);
  // Post-pass.
  ::ReplaceShaderRenderPass(vsSource, emptyGS, fsSource, this, actor, false);
}

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryPolyDataMapper::ReplaceShaderPosition(
  vtkRenderer*, vtkActor*, std::string& vsSource, std::string&)
{
  std::ostringstream oss;
  for (auto& decl : this->ShaderDecls)
  {
    using namespace vtk::literals;
    // Skip pointNormals when normals are sourced from elsewhere.
    if (decl.VariableName.GetHash() == "pointNormals"_hash &&
      this->ShaderNormalSource != ShaderNormalSourceAttribute::Point)
    {
      continue;
    }
    // Skip cellNormals when normals are sourced from elsewhere.
    if (decl.VariableName.GetHash() == "cellNormals"_hash &&
      this->ShaderNormalSource != ShaderNormalSourceAttribute::Cell)
    {
      continue;
    }
    oss << decl << "\n";
  }
  // Remove hard-coded vertexMC attribute.
  vtkShaderProgram::Substitute(vsSource, "in vec4 vertexMC;", oss.str());
  // Write code to populate the integers `pointId` and `cellId`.
  oss.str("");
  oss << R"(
  int pointId = 0;
  int primitiveId = 0;
  int cellId = 0;
  int vertexId = gl_VertexID - vertexIdOffset;
  // pull the vtk point id from vertexIdBuffer
  pointId = texelFetchBuffer(vertexIdBuffer, gl_VertexID).x + pointIdOffset;
  // compute primitive id
  if (cellType == 1) // VTK_VERTEX
  {
    primitiveId = vertexId;
  }
  else if (cellType == 3) // VTK_LINE
  {
    primitiveId = vertexId >> 1;
  }
  else if (cellType == 5) // VTK_TRIANGLE
  {
    primitiveId = vertexId / 3;
  }
  // fast path by default.
  cellId = primitiveId + cellIdOffset;
  // cell id can be implicitly computed from primitiveId in a fast path, low memory case.
  if (usesCellMap == 1)
  {
    // pull the vtk cell id from primitiveToCellBuffer.
    cellId = texelFetchBuffer(primitiveToCellBuffer, primitiveId + primitiveIdOffset).x + cellIdOffset;
  }
)";
  // Write code to pull coordinates.
  oss << "  vec4 vertexMC = vec4(texelFetchBuffer(positions, pointId).xyz, 1.0);\n";
  vtkShaderProgram::Substitute(vsSource, "//VTK::CustomBegin::Impl", oss.str());
  // Assign position vector outputs.
  vtkShaderProgram::Substitute(vsSource, "//VTK::PositionVC::Impl",
    "vertexPositionVCVS = MCVCMatrix * vertexMC;\n"
    "  gl_Position = MCDCMatrix * vertexMC;\n");
}

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryPolyDataMapper::ReplaceShaderNormal(
  vtkRenderer*, vtkActor*, std::string& vsSource, std::string& fsSource)
{
  // Assign normal vector outputs.
  switch (this->ShaderNormalSource)
  {
    case ShaderNormalSourceAttribute::Point:
    {
      vtkShaderProgram::Substitute(vsSource, "//VTK::Normal::Dec",
        "//VTK::Normal::Dec\n"
        "out vec3 normalVCVSOutput;");
      vtkShaderProgram::Substitute(vsSource, "//VTK::Normal::Impl",
        "  vec3 normalMC = texelFetchBuffer(pointNormals, pointId).xyz;\n"
        "  normalVCVSOutput = normalize(normalMatrix * normalMC);\n"
        "//VTK::Normal::Impl");
      vtkShaderProgram::Substitute(fsSource, "//VTK::Normal::Dec",
        "//VTK::Normal::Dec\n"
        "in vec3 normalVCVSOutput;");
      vtkShaderProgram::Substitute(fsSource, "//VTK::Normal::Impl",
        " vec3 vertexNormalVCVS = normalVCVSOutput;\n"
        " if (gl_FrontFacing == false) vertexNormalVCVS = -vertexNormalVCVS;\n"
        "//VTK::Normal::Impl");
      if (this->HasClearCoat)
      {
        vtkShaderProgram::Substitute(fsSource, "//VTK::Normal::Impl",
          "vec3 coatNormalVCVSOutput = normalVCVSOutput;\n"
          "//VTK::Normal::Impl");
      }
      // Write code to pull tangents if they exist.
      if (this->HasTangents)
      {
        vtkShaderProgram::Substitute(vsSource, "//VTK::Normal::Dec",
          "//VTK::Normal::Dec\n"
          "out vec3 tangentVCVS;\n");
        vtkShaderProgram::Substitute(fsSource, "//VTK::Normal::Dec",
          "//VTK::Normal::Dec\n"
          "in vec3 tangentVCVS;\n");
        vtkShaderProgram::Substitute(fsSource, "//VTK::Normal::Impl",
          " vec3 tangentVC = tangentVCVS;\n"
          "//VTK::Normal::Impl");
        vtkShaderProgram::Substitute(vsSource, "//VTK::Normal::Impl",
          "  vec3 tangentMC = texelFetchBuffer(tangents, pointId).xyz;\n"
          "  tangentVCVS = normalMatrix * tangentMC;");
      }
      // normal mapping
      // if we have points tangents, we need it for normal mapping, coat normal mapping and
      // anisotropy
      if (this->HasTangents && !this->DrawingVertices &&
        (this->UsesNormalMap || this->UsesCoatNormalMap || this->HasAnisotropy))
      {
        if (this->HasAnisotropy)
        {
          // We need to rotate the anisotropy direction (the tangent) by anisotropyRotation * 2 *
          // PI
          vtkShaderProgram::Substitute(fsSource, "//VTK::Normal::Dec",
            "//VTK::Normal::Dec\n"
            "uniform float anisotropyRotationUniform;\n");

          if (this->UsesRotationMap)
          {
            // Sample the texture
            vtkShaderProgram::Substitute(fsSource, "//VTK::Normal::Impl",
              "  vec2 anisotropySample = texture(anisotropyTex, tcoordVCVSOutput).rg;\n"
              "  float anisotropy = anisotropySample.x * anisotropyUniform;\n"
              "  float anisotropyRotation = anisotropySample.y * anisotropyRotationUniform;\n"
              "//VTK::Normal::Impl");
          }
          else
          {
            vtkShaderProgram::Substitute(fsSource, "//VTK::Normal::Impl",
              "  float anisotropy = anisotropyUniform;\n"
              "  float anisotropyRotation = anisotropyRotationUniform;\n"
              "//VTK::Normal::Impl");
          }
          vtkShaderProgram::Substitute(fsSource, "//VTK::Normal::Impl",
            "  // Rotate the anisotropy direction (tangent) around the normal with a rotation "
            "factor\n"
            "  float r2pi = anisotropyRotation * 2.0 * PI;\n"
            "  float s = - sin(r2pi);\n" // Counter clockwise (as in
                                         // OSPray)
            "  float c = cos(r2pi);\n"
            "  vec3 Nn = normalize(normalVCVSOutput);\n"
            "  tangentVC = (1.0-c) * dot(tangentVCVS,Nn) * Nn\n"
            "+ c * tangentVCVS - s * cross(Nn, tangentVCVS);\n"
            "//VTK::Normal::Impl");
        }

        vtkShaderProgram::Substitute(fsSource, "//VTK::Normal::Impl",
          "  tangentVC = normalize(tangentVC - dot(tangentVC, "
          "normalVCVSOutput) * normalVCVSOutput);\n"
          "  vec3 bitangentVC = cross(normalVCVSOutput, tangentVC);\n"
          "//VTK::Normal::Impl");

        if (this->UsesNormalMap || this->UsesCoatNormalMap)
        {
          vtkShaderProgram::Substitute(fsSource, "//VTK::Normal::Impl",
            "  mat3 tbn = mat3(tangentVC, bitangentVC, normalVCVSOutput);\n"
            "//VTK::Normal::Impl");

          if (this->UsesNormalMap)
          {
            vtkShaderProgram::Substitute(fsSource, "//VTK::Normal::Dec",
              "//VTK::Normal::Dec\n"
              "uniform float normalScaleUniform;\n");

            vtkShaderProgram::Substitute(fsSource, "//VTK::Normal::Impl",
              "  vec3 normalTS = texture(normalTex, tcoordVCVSOutput).xyz * 2.0 - 1.0;\n"
              "  normalTS = normalize(normalTS * vec3(normalScaleUniform, normalScaleUniform, "
              "1.0));\n"
              "  vertexNormalVCVS = normalize(tbn * normalTS);\n"
              "//VTK::Normal::Impl");
          }
          if (this->UsesCoatNormalMap)
          {
            vtkShaderProgram::Substitute(fsSource, "//VTK::Normal::Dec",
              "//VTK::Normal::Dec\n"
              "uniform float coatNormalScaleUniform;\n");

            vtkShaderProgram::Substitute(fsSource, "//VTK::Normal::Impl",
              "  vec3 coatNormalTS = texture(coatNormalTex, tcoordVCVSOutput).xyz * 2.0 - 1.0;\n"
              "  coatNormalTS = normalize(coatNormalTS * vec3(coatNormalScaleUniform, "
              "coatNormalScaleUniform, "
              "1.0));\n"
              "  coatNormalVCVSOutput = normalize(tbn * coatNormalTS);\n"
              "//VTK::Normal::Impl");
          }
        }
      }
      break;
    }
    case ShaderNormalSourceAttribute::Cell:
      vtkShaderProgram::Substitute(vsSource, "//VTK::Normal::Dec",
        "//VTK::Normal::Dec\n"
        "out vec3 normalVCVSOutput;");
      vtkShaderProgram::Substitute(vsSource, "//VTK::Normal::Impl",
        "vec3 normalMC = texelFetchBuffer(cellNormals, cellId).xyz;\n"
        "  normalVCVSOutput = normalize(normalMatrix * normalMC);\n");
      vtkShaderProgram::Substitute(fsSource, "//VTK::Normal::Dec",
        "//VTK::Normal::Dec\n"
        "in vec3 normalVCVSOutput;");
      vtkShaderProgram::Substitute(fsSource, "//VTK::Normal::Impl",
        "vec3 vertexNormalVCVS = normalVCVSOutput;\n"
        "if (gl_FrontFacing == false) vertexNormalVCVS = -vertexNormalVCVS;\n"
        "//VTK::Normal::Impl");
      if (this->HasClearCoat)
      {
        vtkShaderProgram::Substitute(fsSource, "//VTK::Normal::Impl",
          "vec3 coatNormalVCVSOutput = normalVCVSOutput;\n"
          "//VTK::Normal::Impl");
      }
      break;
    case ShaderNormalSourceAttribute::Primitive:
    {
      // We have no point or cell normals, so compute something.
      // Caveat: this assumes that neighboring fragments are present,
      // result is undefined (maybe NaN?) if neighbors are missing.
      vtkShaderProgram::Substitute(fsSource, "//VTK::UniformFlow::Impl",
        "vec3 fdx = dFdx(vertexVC.xyz);\n"
        "  vec3 fdy = dFdy(vertexVC.xyz);\n"
        "  //VTK::UniformFlow::Impl\n" // For further replacements
      );
      std::ostringstream fsImpl;
      // here, orient the view coordinate normal such that it always points out of the screen.
      fsImpl << "vec3 primitiveNormal;\n";
      fsImpl << "if (primitiveSize == 1) { primitiveNormal = vec3(0.0, 0.0, 1.0); }\n";
      // Generate a normal for a line that is perpendicular to the line and
      // maximally aligned with the camera view direction.  Basic approach
      // is as follows.  Start with the gradients dFdx and dFdy (see above),
      // both of these gradients will point along the line but may have
      // different magnitudes and directions, either gradient might be zero.
      // Sum them to get a good measurement of the line direction vector,
      // use a dot product to check if they point in opposite directions.
      // Cross this line vector with (0, 0, 1) to get a vector orthogonal to
      // the camera view and the line, result is (lineVec.y, -lineVec.x, 0).
      // Cross this vector with the line vector again to get a normal that
      // is orthogonal to the line and maximally aligned with the camera.
      fsImpl << "else if (primitiveSize == 2)\n"
                "{\n"
                "  float addOrSubtract = (dot(fdx, fdy) >= 0.0) ? 1.0 : -1.0;\n"
                "  vec3 lineVec = addOrSubtract*fdy + fdx;\n"
                "  primitiveNormal = normalize(cross(vec3(lineVec.y, -lineVec.x, 0.0), "
                "lineVec));\n"
                "}\n";
      // for primitives with 3 or more points (i.e triangles and triangle strips in our
      // mapper, we don't do line loops or line strips)
      fsImpl << "else\n"
                "{\n"
                "  primitiveNormal = normalize(cross(fdx,fdy));\n"
                "  if (cameraParallel == 1 && primitiveNormal.z < 0.0) { primitiveNormal = "
                "-1.0*primitiveNormal; }\n"
                "  if (cameraParallel == 0 && dot(primitiveNormal,vertexVC.xyz) > 0.0) { "
                "primitiveNormal "
                "= -1.0*primitiveNormal; }\n"
                "}\n";
      fsImpl << "vec3 vertexNormalVCVS = primitiveNormal;\n";
      if (this->HasClearCoat)
      {
        fsImpl << "vec3 coatNormalVCVSOutput = normalVCVSOutput;\n";
      }
      fsImpl << "//VTK::Normal::Impl";
      vtkShaderProgram::Substitute(fsSource, "//VTK::Normal::Impl", fsImpl.str());
      break;
    }
    default:
      break;
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryPolyDataMapper::ReplaceShaderColor(
  vtkRenderer* renderer, vtkActor* actor, std::string& vsSource, std::string& fsSource)
{
  // Assign color outputs.
  std::ostringstream oss;
  // these are always defined
  std::string colorDec = R"(
uniform float intensity_ambient; // the material ambient
uniform float intensity_diffuse; // the material diffuse
uniform float intensity_opacity; // the fragment opacity
uniform float intensity_specular; // the material specular intensity
uniform vec3 color_specular; // intensity weighted color
uniform float power_specular;
uniform vec3 color_ambient; // ambient color
uniform vec3 color_diffuse; // diffuse color
uniform vec3 vertex_color; // vertex color
uniform int vertex_pass;)";
  std::string vertexPassColorImpl = R"(
  if (vertex_pass == 1)
  {
    ambientColor = intensity_ambient * vertex_color;
    diffuseColor = intensity_diffuse * vertex_color;
    specularColor = intensity_specular * vertex_color;
  })";
  switch (this->ShaderColorSource)
  {
    case ShaderColorSourceAttribute::Point:
    case ShaderColorSourceAttribute::Cell:
      // Write vertex shader
      vtkShaderProgram::Substitute(vsSource, "//VTK::Color::Dec", "out vec4 vertexColorVS;");
      oss.str("");
      if (this->ShaderColorSource == ShaderColorSourceAttribute::Point)
      {
        oss << "int colorID = pointId;\n";
      }
      else
      {
        oss << "int colorID = cellId;\n";
      }
      oss << "vertexColorVS = vec4(texelFetchBuffer(colors, colorID)) / vec4(255.0, 255.0, 255.0, "
             "255.0);\n";
      vtkShaderProgram::Substitute(vsSource, "//VTK::Color::Impl", oss.str());
      // Write fragment shader
      oss.str("");
      oss << colorDec << "\n"
          << "in vec4 vertexColorVS;\n";
      vtkShaderProgram::Substitute(fsSource, "//VTK::Color::Dec", oss.str());
      oss.str("");
      oss << R"(
  vec3 ambientColor = intensity_ambient * vertexColorVS.rgb;
  vec3 diffuseColor = intensity_diffuse * vertexColorVS.rgb;
  vec3 specularColor = intensity_specular * color_specular;
  float specularPower = power_specular;
  float opacity = intensity_opacity * vertexColorVS.a;
)";
      oss << vertexPassColorImpl;
      vtkShaderProgram::Substitute(fsSource, "//VTK::Color::Impl", oss.str());
      break;
    case ShaderColorSourceAttribute::PointTexture:
      // TODO: Handle texture coordinate transforms and populate tcoordVCVSOutput from vertex
      // shader.
      vtkShaderProgram::Substitute(fsSource, "//VTK::Color::Dec", colorDec);
      oss.str("");
      oss << R"(
  vec4 texColor = texture(colortexture, colorTCoordVCVSOutput.st);
  vec3 ambientColor = intensity_ambient * texColor.rgb;
  vec3 diffuseColor = intensity_diffuse * texColor.rgb;
  vec3 specularColor = intensity_specular * color_specular;
  float specularPower = power_specular;
  float opacity = intensity_opacity * texColor.a;
)";
      oss << vertexPassColorImpl;
      vtkShaderProgram::Substitute(fsSource, "//VTK::Color::Impl", oss.str());
      break;
    case ShaderColorSourceAttribute::Uniform:
    default:
    {
      auto oglRen = static_cast<vtkOpenGLRenderer*>(renderer);
      auto stats = vtkGLSLModLight::GetBasicLightStats(oglRen, actor);
      std::string colorImpl;
      colorImpl += "  vec3 specularColor = intensity_specular * color_specular;\n"
                   "  float specularPower = power_specular;\n";

      colorImpl += "  vec3 ambientColor = intensity_ambient * color_ambient;\n"
                   "  vec3 diffuseColor = intensity_diffuse * color_diffuse;\n"
                   "  float opacity = intensity_opacity;\n";
      colorImpl += vertexPassColorImpl;
      if (actor->GetBackfaceProperty())
      {
        colorDec += "uniform float intensity_opacity_bf; // the fragment opacity\n"
                    "uniform float intensity_ambient_bf; // the material ambient\n"
                    "uniform float intensity_diffuse_bf; // the material diffuse\n"
                    "uniform vec3 color_ambient_bf; // ambient material color\n"
                    "uniform vec3 color_diffuse_bf; // diffuse material color\n";
        if (stats.Complexity > 0)
        {
          colorDec += "uniform float intensity_specular_bf; // the material specular intensity\n"
                      "uniform vec3 color_specular_bf; // intensity weighted color\n"
                      "uniform float power_specular_bf;\n";
          colorImpl +=
            "  if (gl_FrontFacing == false && vertex_pass != 1 && primitiveSize != 1) {\n"
            "    ambientColor = intensity_ambient_bf * color_ambient_bf;\n"
            "    diffuseColor = intensity_diffuse_bf * color_diffuse_bf;\n"
            "    specularColor = intensity_specular_bf * color_specular_bf;\n"
            "    specularPower = power_specular_bf;\n"
            "    opacity = intensity_opacity_bf; }\n";
        }
        else
        {
          colorImpl +=
            "  if (gl_FrontFacing == false && vertex_pass != 1 && primitiveSize != 1) {\n"
            "    ambientColor = intensity_ambient_bf * color_ambient_bf;\n"
            "    diffuseColor = intensity_diffuse_bf * color_diffuse_bf;\n"
            "    opacity = intensity_opacity_bf; }\n";
        }
      }
      vtkShaderProgram::Substitute(fsSource, "//VTK::Color::Dec", colorDec);
      vtkShaderProgram::Substitute(fsSource, "//VTK::Color::Impl", colorImpl);
      break;
    }
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryPolyDataMapper::ReplaceShaderImplementationCustomUniforms(
  vtkRenderer*, vtkActor*, std::string& vsSource, std::string& fsSource)
{
  // Sends primitiveSize as a uniform
  vtkShaderProgram::Substitute(vsSource, "//VTK::CustomUniforms::Dec",
    "//VTK::CustomUniforms::Dec;\n"
    "uniform highp int primitiveSize;\n"
    "uniform highp int usesEdgeValues;\n");
  vtkShaderProgram::Substitute(fsSource, "//VTK::CustomUniforms::Dec",
    "//VTK::CustomUniforms::Dec;\n"
    "uniform highp int primitiveSize;\n"
    "uniform highp int usesEdgeValues;\n");
}

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryPolyDataMapper::ReplaceShaderPointSize(
  vtkRenderer*, vtkActor*, std::string& vsSource, std::string&)
{
  // Point size
  vtkShaderProgram::Substitute(vsSource, "//VTK::PointSizeGLES30::Dec", "uniform float pointSize;");
  vtkShaderProgram::Substitute(
    vsSource, "//VTK::PointSizeGLES30::Impl", "gl_PointSize = pointSize;");
}

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryPolyDataMapper::ReplaceShaderWideLines(
  vtkRenderer*, vtkActor*, std::string& vsSource, std::string&)
{
  // Wide lines only when primitiveSize == 2
  vtkShaderProgram::Substitute(vsSource, "//VTK::LineWidthGLES30::Dec",
    "uniform vec4 viewportDimensions;\n"
    "uniform float lineWidthStepSize;\n"
    "uniform float halfLineWidth;");
  vtkShaderProgram::Substitute(vsSource, "//VTK::LineWidthGLES30::Impl",
    "if (primitiveSize == 2) {"
    "if (halfLineWidth > 0.0)\n"
    "{\n"
    "  float offset = float(gl_InstanceID / 2) * lineWidthStepSize - halfLineWidth;\n"
    "  vec4 tmpPos = gl_Position;\n"
    "  vec3 tmpPos2 = tmpPos.xyz / tmpPos.w;\n"
    "  tmpPos2.x = tmpPos2.x + 2.0 * mod(float(gl_InstanceID), 2.0) * offset / "
    "viewportDimensions[2];\n"
    "  tmpPos2.y = tmpPos2.y + 2.0 * mod(float(gl_InstanceID + 1), 2.0) * offset / "
    "viewportDimensions[3];\n"
    "  gl_Position = vec4(tmpPos2.xyz * tmpPos.w, tmpPos.w);\n"
    "}\n"
    "}\n");
}

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryPolyDataMapper::ReplaceShaderEdges(
  vtkRenderer*, vtkActor*, std::string& vsSource, std::string& fsSource)
{
  // Edges and wireframe.
  vtkShaderProgram::Substitute(vsSource, "//VTK::EdgesGLES30::Dec",
    R"(flat out mat4 edgeEqn;
uniform highp int wireframe;
uniform highp int edgeVisibility;)");
  std::ostringstream vsImpl;
  vsImpl
    << R"(// only compute edge equation for provoking vertex i.e p3 in a triangle made of p1, p2, p3
  if ((((edgeVisibility == 1) || (wireframe == 1)) && (primitiveSize == 3)) && (vertexId % 3 == 2))
  {
    int p0 = texelFetchBuffer(vertexIdBuffer, gl_VertexID - 2).x + pointIdOffset;
    int p1 = texelFetchBuffer(vertexIdBuffer, gl_VertexID - 1).x + pointIdOffset;
    vec4 p0MC = vec4(texelFetchBuffer(positions, p0).xyz, 1.0);
    vec4 p1MC = vec4(texelFetchBuffer(positions, p1).xyz, 1.0);
    vec4 p0DC = MCDCMatrix * p0MC;
    vec4 p1DC = MCDCMatrix * p1MC;
    vec2 pos[4];
    pos[0] = p0DC.xy/p0DC.w;
    pos[1] = p1DC.xy/p1DC.w;
    pos[2] = gl_Position.xy/gl_Position.w;
    for(int i = 0; i < 3; ++i)
    {
      pos[i] = pos[i]*vec2(0.5) + vec2(0.5);
      pos[i] = pos[i]*viewportDimensions.zw + viewportDimensions.xy;
    }
    pos[3] = pos[0];
    float ccw = sign(cross(vec3(pos[1] - pos[0], 0.0), vec3(pos[2] - pos[0], 0.0)).z);
    for (int i = 0; i < 3; i++)
    {
      vec2 tmp = normalize(pos[i+1] - pos[i]);
      tmp = ccw*vec2(-tmp.y, tmp.x);
      float d = dot(pos[i], tmp);
      edgeEqn[i] = vec4(tmp.x, tmp.y, 0.0, -d);
    }
    if (usesEdgeValues == 1)
    {
      float nudge = halfLineWidth * 2.0 + 0.5;
      int edgeValue = int(texelFetchBuffer(edgeValueBuffer, primitiveId + edgeValueBufferOffset).x);
      // all but last triangle in a polygon's implicit triangulation
      if (edgeValue < 4) edgeEqn[2].z = nudge;
      // these are triangles which have edge flag array.
      if ((edgeValue % 4) < 2) edgeEqn[1].z = nudge;
      // all but first triangle in a polygon's implicit triangulation
      if ((edgeValue % 2) < 1) edgeEqn[0].z = nudge;
    }
  })";
  vtkShaderProgram::Substitute(vsSource, "//VTK::EdgesGLES30::Impl", vsImpl.str());

  vtkShaderProgram::Substitute(fsSource, "//VTK::Edges::Dec",
    R"(flat in mat4 edgeEqn;
uniform vec3 edgeColor;
uniform float edgeOpacity;
uniform highp int wireframe;
uniform highp int edgeVisibility;
uniform float halfLineWidth;
)");

  std::ostringstream fsImpl;
  fsImpl << R"(
  if (((edgeVisibility == 1) || (wireframe == 1)) && (primitiveSize == 3))
  {
    // distance gets larger as you go inside the polygon
    float edist[3];
    edist[0] = dot(edgeEqn[0].xy, gl_FragCoord.xy) + edgeEqn[0].w;
    edist[1] = dot(edgeEqn[1].xy, gl_FragCoord.xy) + edgeEqn[1].w;
    edist[2] = dot(edgeEqn[2].xy, gl_FragCoord.xy) + edgeEqn[2].w;
    if (usesEdgeValues == 1)
    {
      if (edist[0] < -0.5 && edgeEqn[0].z > 0.0) discard;
      if (edist[1] < -0.5 && edgeEqn[1].z > 0.0) discard;
      if (edist[2] < -0.5 && edgeEqn[2].z > 0.0) discard;
      edist[0] += edgeEqn[0].z;
      edist[1] += edgeEqn[1].z;
      edist[2] += edgeEqn[2].z;
    }
    float emix = clamp(0.5 + halfLineWidth - min(min(edist[0], edist[1]), edist[2]), 0.0, 1.0);
    if (wireframe == 1)
    {
      opacity = mix(0.0, opacity, emix);
    }
    else
    {
      diffuseColor = mix(diffuseColor, vec3(0.0), emix * edgeOpacity);
      ambientColor = mix(ambientColor, edgeColor, emix * edgeOpacity);
    }
  })";
  vtkShaderProgram::Substitute(fsSource, "//VTK::Edges::Impl", fsImpl.str());
}

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryPolyDataMapper::ReplaceShaderSelection(
  vtkRenderer*, vtkActor*, std::string& vsSource, std::string& fsSource)
{
  // selections
  if (this->LastSelectionState >= vtkHardwareSelector::MIN_KNOWN_PASS)
  {
    switch (this->LastSelectionState)
    {
      // point ID low and high are always just gl_VertexId
      case vtkHardwareSelector::POINT_ID_LOW24:
        vtkShaderProgram::Substitute(
          vsSource, "//VTK::Picking::Dec", "flat out int vertexIDVSOutput;");
        // undo the pointIdOffset addition so that selector can work with it right away.
        vtkShaderProgram::Substitute(
          vsSource, "//VTK::Picking::Impl", "  vertexIDVSOutput = pointId - pointIdOffset;");
        vtkShaderProgram::Substitute(
          fsSource, "//VTK::Picking::Dec", "flat in int vertexIDVSOutput;");
        vtkShaderProgram::Substitute(fsSource, "//VTK::Picking::Impl",
          "  int idx = vertexIDVSOutput;\n"
          "  gl_FragData[0] = vec4(float(idx%256)/255.0, float((idx/256)%256)/255.0, "
          "float((idx/65536)%256)/255.0, 1.0);");
        break;

      case vtkHardwareSelector::POINT_ID_HIGH24:
        vtkShaderProgram::Substitute(
          vsSource, "//VTK::Picking::Dec", "flat out int vertexIDVSOutput;\n");
        // undo the pointIdOffset addition so that selector can work with it right away.
        vtkShaderProgram::Substitute(
          vsSource, "//VTK::Picking::Impl", "  vertexIDVSOutput = pointId - pointIdOffset;\n");
        vtkShaderProgram::Substitute(
          fsSource, "//VTK::Picking::Dec", "flat in int vertexIDVSOutput;\n");
        vtkShaderProgram::Substitute(fsSource, "//VTK::Picking::Impl",
          "  int idx = vertexIDVSOutput;\n idx = ((idx & 0xff000000) >> 24);\n"
          "  gl_FragData[0] = vec4(float(idx)/255.0, 0.0, 0.0, 1.0);\n");
        break;

      // cell ID is just gl_PrimitiveID
      case vtkHardwareSelector::CELL_ID_LOW24:
        vtkShaderProgram::Substitute(
          vsSource, "//VTK::Picking::Dec", "flat out int cellIDVSOutput;");
        // undo the cellIdOffset addition so that selector can work with it right away.
        vtkShaderProgram::Substitute(
          vsSource, "//VTK::Picking::Impl", "  cellIDVSOutput = cellId - cellIdOffset;");
        vtkShaderProgram::Substitute(
          fsSource, "//VTK::Picking::Dec", "flat in int cellIDVSOutput;");
        vtkShaderProgram::Substitute(fsSource, "//VTK::Picking::Impl",
          "  int idx = cellIDVSOutput;\n"
          "  gl_FragData[0] = vec4(float(idx%256)/255.0, float((idx/256)%256)/255.0, "
          "float((idx/65536)%256)/255.0, 1.0);");
        break;

      case vtkHardwareSelector::CELL_ID_HIGH24:
        // if (selector &&
        //     selector->GetFieldAssociation() == vtkDataObject::FIELD_ASSOCIATION_POINTS)
        vtkShaderProgram::Substitute(
          vsSource, "//VTK::Picking::Dec", "flat out int cellIDVSOutput;");
        // undo the cellIdOffset addition so that selector can work with it right away.
        vtkShaderProgram::Substitute(
          vsSource, "//VTK::Picking::Impl", "  cellIDVSOutput = cellId - cellIdOffset;");
        vtkShaderProgram::Substitute(
          fsSource, "//VTK::Picking::Dec", "flat in int cellIDVSOutput;");
        vtkShaderProgram::Substitute(fsSource, "//VTK::Picking::Impl",
          "  int idx = cellIDVSOutput;\n"
          "  idx = ((idx & 0xff000000) >> 24);\n"
          "  gl_FragData[0] = vec4(float(idx)/255.0, 0.0, 0.0, 1.0);");
        break;

      default: // actor process and composite
        vtkShaderProgram::Substitute(fsSource, "//VTK::Picking::Dec", "uniform vec3 mapperIndex;");
        vtkShaderProgram::Substitute(
          fsSource, "//VTK::Picking::Impl", "  gl_FragData[0] = vec4(mapperIndex,1.0);\n");
    }
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryPolyDataMapper::ReplaceShaderTCoord(
  vtkRenderer*, vtkActor* actor, std::string& vsSource, std::string& fsSource)
{
  // Apply texture coordinates.
  std::ostringstream oss;
  // always define texture maps if we have them
  oss << "uniform bool showTexturesOnBackface;\n";
  auto textures = this->GetTextures(actor);
  for (auto it : textures)
  {
    vtkOpenGLShaderDeclaration decl = {};
    decl.AttributeType =
      it.first->GetCubeMap() ? GLSLAttributeType::SamplerCube : GLSLAttributeType::Sampler2D;
    decl.DataType = GLSLDataType::Float;
    decl.PrecisionType = GLSLPrecisionType::None;
    decl.QualifierType = GLSLQualifierType::Uniform;
    decl.VariableName = it.second;
    oss << decl << "\n";
  }
  vtkShaderProgram::Substitute(fsSource, "//VTK::TMap::Dec", oss.str());
  // now handle each texture coordinate
  // if no texture coordinates then we are done
  std::set<std::pair<std::string, std::string>> tcoordAndSamplers;
  for (const auto& it : textures)
  {
    // do we have special tcoords for this texture?
    auto tcoordAndSampler = this->GetTextureCoordinateAndSamplerBufferNames(it.second.c_str());
    const auto& tcoordname = tcoordAndSampler.first;
    const auto& samplerBufferName = tcoordAndSampler.second;
    auto texBufIter = this->Arrays.find(samplerBufferName);
    if (texBufIter == this->Arrays.end())
    {
      vtkWarningMacro(<< "No array for " << samplerBufferName << " | " << tcoordname);
      continue;
    }
    int tcoordComps = texBufIter->second.Arrays.front()->GetNumberOfComponents();
    if (tcoordComps == 1 || tcoordComps == 2)
    {
      tcoordAndSamplers.emplace(tcoordname, samplerBufferName);
    }
  }
  // if no texture coordinates then we are done
  if (!tcoordAndSamplers.empty())
  {
    // handle texture transformation matrix and create the
    // vertex shader texture coordinate implementation
    // code for all texture coordinates.
    vtkInformation* info = actor->GetPropertyKeys();
    std::string vsimpl;
    for (const auto& it : tcoordAndSamplers)
    {
      const auto& tcoordname = it.first;
      const auto& samplerBufferName = it.second;
      int tcoordComps =
        this->Arrays.find(samplerBufferName)->second.Arrays.front()->GetNumberOfComponents();
      std::string tCoordType;
      std::string suffix;
      if (tcoordComps == 1)
      {
        tCoordType = "float";
        suffix = ".x";
      }
      else
      {
        tCoordType = "vec2";
        suffix = ".st";
      }
      vsimpl += tCoordType + " " + tcoordname + " = texelFetchBuffer(" + samplerBufferName +
        ", pointId)" + suffix + ";\n";
    }
    if (info && info->Has(vtkProp::GeneralTextureTransform()))
    {
      vtkShaderProgram::Substitute(vsSource, "//VTK::TCoord::Dec",
        "//VTK::TCoord::Dec\n"
        "uniform mat4 tcMatrix;",
        false);
      for (const auto& it : tcoordAndSamplers)
      {
        const auto& tcoordname = it.first;
        const auto& samplerBufferName = it.second;
        int tcoordComps =
          this->Arrays.find(samplerBufferName)->second.Arrays.front()->GetNumberOfComponents();
        if (tcoordComps == 1)
        {
          vsimpl += "vec4 " + tcoordname + "Tmp = tcMatrix*vec4(" + tcoordname +
            ",0.0,0.0,1.0);\n" + tcoordname + "VCVSOutput = " + tcoordname + "Tmp.x/" + tcoordname +
            "Tmp.w;\n";
          if (this->SeamlessU)
          {
            vsimpl += tcoordname + "VCVSOutputU1 = fract(" + tcoordname + "VCVSOutput.x);\n" +
              tcoordname + "VCVSOutputU2 = fract(" + tcoordname + "VCVSOutput.x+0.5)-0.5;\n";
          }
        }
        else
        {
          vsimpl += "vec4 " + tcoordname + "Tmp = tcMatrix*vec4(" + tcoordname + ",0.0,1.0);\n" +
            tcoordname + "VCVSOutput = " + tcoordname + "Tmp.xy/" + tcoordname + "Tmp.w;\n";
          if (this->SeamlessU)
          {
            vsimpl += tcoordname + "VCVSOutputU1 = fract(" + tcoordname + "VCVSOutput.x);\n" +
              tcoordname + "VCVSOutputU2 = fract(" + tcoordname + "VCVSOutput.x+0.5)-0.5;\n";
          }
          if (this->SeamlessV)
          {
            vsimpl += tcoordname + "VCVSOutputV1 = fract(" + tcoordname + "VCVSOutput.y);\n" +
              tcoordname + "VCVSOutputV2 = fract(" + tcoordname + "VCVSOutput.y+0.5)-0.5;\n";
          }
        }
      }
    }
    else
    {
      for (const auto& it : tcoordAndSamplers)
      {
        const auto& tcoordname = it.first;
        vsimpl += tcoordname + "VCVSOutput = " + tcoordname + ";\n";
        if (this->SeamlessU)
        {
          vsimpl += tcoordname + "VCVSOutputU1 = fract(" + tcoordname + "VCVSOutput.x);\n" +
            tcoordname + "VCVSOutputU2 = fract(" + tcoordname + "VCVSOutput.x+0.5)-0.5;\n";
        }
        if (this->SeamlessV)
        {
          vsimpl += tcoordname + "VCVSOutputV1 = fract(" + tcoordname + "VCVSOutput.y);\n" +
            tcoordname + "VCVSOutputV2 = fract(" + tcoordname + "VCVSOutput.y+0.5)-0.5;\n";
        }
      }
    }

    vtkShaderProgram::Substitute(vsSource, "//VTK::TCoord::Impl", vsimpl);

    // now create the rest of the vertex and geometry shader code
    std::string vsdec;
    std::string fsdec;
    for (const auto& it : tcoordAndSamplers)
    {
      const auto& tcoordname = it.first;
      const auto& samplerBufferName = it.second;
      int tcoordComps =
        this->Arrays.find(samplerBufferName)->second.Arrays.front()->GetNumberOfComponents();
      std::string tCoordType;
      if (tcoordComps == 1)
      {
        tCoordType = "float";
      }
      else
      {
        tCoordType = "vec2";
      }
      vsdec += "uniform highp samplerBuffer " + samplerBufferName + ";\n";
      vsdec += "out " + tCoordType + " " + tcoordname + "VCVSOutput;\n";
      if (this->SeamlessU)
      {
        vsdec += "out float " + tcoordname + "VCVSOutputU1;\n";
        vsdec += "out float " + tcoordname + "VCVSOutputU2;\n";
      }
      if (this->SeamlessV && tcoordComps > 1)
      {
        vsdec += "out float " + tcoordname + "VCVSOutputV1;\n";
        vsdec += "out float " + tcoordname + "VCVSOutputV2;\n";
      }
      fsdec += "in " + tCoordType + " " + tcoordname + "VCVSOutput;\n";
      if (this->SeamlessU)
      {
        fsdec += "in float " + tcoordname + "VCVSOutputU1;\n";
        fsdec += "in float " + tcoordname + "VCVSOutputU2;\n";
      }
      if (this->SeamlessV && tcoordComps > 1)
      {
        fsdec += "in float " + tcoordname + "VCVSOutputV1;\n";
        fsdec += "in float " + tcoordname + "VCVSOutputV2;\n";
      }
    }

    vtkShaderProgram::Substitute(vsSource, "//VTK::TCoord::Dec", vsdec);
    vtkShaderProgram::Substitute(fsSource, "//VTK::TCoord::Dec", fsdec);

    int nbTex2d = 0;

    // OK now handle the fragment shader implementation
    // everything else has been done.
    std::string tCoordImpFS;
    for (size_t i = 0; i < textures.size(); ++i)
    {
      vtkTexture* texture = textures[i].first;

      // ignore cubemaps
      if (texture->GetCubeMap())
      {
        continue;
      }

      // ignore special textures
      if (textures[i].second == "albedoTex" || textures[i].second == "normalTex" ||
        textures[i].second == "materialTex" || textures[i].second == "brdfTex" ||
        textures[i].second == "emissiveTex" || textures[i].second == "anisotropyTex" ||
        textures[i].second == "coatNormalTex" || textures[i].second == "colortexture")
      {
        continue;
      }

      nbTex2d++;

      std::stringstream ss;

      // do we have special tcoords for this texture?
      auto tcoordAndSampler =
        this->GetTextureCoordinateAndSamplerBufferNames(textures[i].second.c_str());
      const auto& tcoordname = tcoordAndSampler.first;
      const auto& samplerBufferName = tcoordAndSampler.second;
      int tcoordComps =
        this->Arrays.find(samplerBufferName)->second.Arrays.front()->GetNumberOfComponents();

      std::string tCoordImpFSPre;
      std::string tCoordImpFSPost;
      if (tcoordComps == 1)
      {
        tCoordImpFSPre = "vec2(";
        tCoordImpFSPost = ", 0.0)";
      }
      else
      {
        tCoordImpFSPre = "";
        tCoordImpFSPost = "";
      }

      // Read texture color
      if (this->SeamlessU || (this->SeamlessV && tcoordComps > 1))
      {
        // Implementation of "Cylindrical and Toroidal Parameterizations Without Vertex Seams"
        // Marco Turini, 2011
        if (tcoordComps == 1)
        {
          ss << "  float texCoord;\n";
        }
        else
        {
          ss << "  vec2 texCoord;\n";
        }
        if (this->SeamlessU)
        {
          ss << "  if (fwidth(" << tCoordImpFSPre << tcoordname << "VCVSOutputU1" << tCoordImpFSPost
             << ") <= fwidth(" << tCoordImpFSPre << tcoordname << "VCVSOutputU2" << tCoordImpFSPost
             << "))\n  {\n"
             << "    texCoord.x = " << tCoordImpFSPre << tcoordname << "VCVSOutputU1"
             << tCoordImpFSPost << ";\n  }\n  else\n  {\n"
             << "    texCoord.x = " << tCoordImpFSPre << tcoordname << "VCVSOutputU2"
             << tCoordImpFSPost << ";\n  }\n";
        }
        else
        {
          ss << "  texCoord.x = " << tCoordImpFSPre << tcoordname << "VCVSOutput" << tCoordImpFSPost
             << ".x"
             << ";\n";
        }
        if (tcoordComps > 1)
        {
          if (this->SeamlessV)
          {
            ss << "  if (fwidth(" << tCoordImpFSPre << tcoordname << "VCVSOutputV1"
               << tCoordImpFSPost << ") <= fwidth(" << tCoordImpFSPre << tcoordname
               << "VCVSOutputV2" << tCoordImpFSPost << "))\n  {\n"
               << "    texCoord.y = " << tCoordImpFSPre << tcoordname << "VCVSOutputV1"
               << tCoordImpFSPost << ";\n  }\n  else\n  {\n"
               << "    texCoord.y = " << tCoordImpFSPre << tcoordname << "VCVSOutputV2"
               << tCoordImpFSPost << ";\n  }\n";
          }
          else
          {
            ss << "  texCoord.y = " << tCoordImpFSPre << tcoordname << "VCVSOutput"
               << tCoordImpFSPost << ".y"
               << ";\n";
          }
        }
        ss << "  vec4 tcolor_" << i << " = texture(" << textures[i].second
           << ", texCoord); // Read texture color\n";
      }
      else
      {
        ss << "vec4 tcolor_" << i << " = texture(" << textures[i].second << ", " << tCoordImpFSPre
           << tcoordname << "VCVSOutput" << tCoordImpFSPost << "); // Read texture color\n";
      }

      vtkTextureObject* textureObject = vtkOpenGLTexture::SafeDownCast(texture)->GetTextureObject();
      if (!textureObject)
      {
        vtkErrorMacro("Could not find the vtkTextureObject");
        return;
      }

      // Update color based on texture number of components
      int tNumComp = textureObject->GetComponents();
      switch (tNumComp)
      {
        case 1:
          ss << "tcolor_" << i << " = vec4(tcolor_" << i << ".r,tcolor_" << i << ".r,tcolor_" << i
             << ".r,1.0)";
          break;
        case 2:
          ss << "tcolor_" << i << " = vec4(tcolor_" << i << ".r,tcolor_" << i << ".r,tcolor_" << i
             << ".r,tcolor_" << i << ".g)";
          break;
        case 3:
          ss << "tcolor_" << i << " = vec4(tcolor_" << i << ".r,tcolor_" << i << ".g,tcolor_" << i
             << ".b,1.0)";
      }
      ss << "; // Update color based on texture nbr of components \n";

      // Define final color based on texture blending
      if (nbTex2d == 1)
      {
        ss << "vec4 tcolor = tcolor_" << i << "; // BLENDING: None (first texture) \n\n";
      }
      else
      {
        int tBlending = vtkOpenGLTexture::SafeDownCast(texture)->GetBlendingMode();
        switch (tBlending)
        {
          case vtkTexture::VTK_TEXTURE_BLENDING_MODE_REPLACE:
            ss << "tcolor.rgb = tcolor_" << i << ".rgb * tcolor_" << i << ".a + "
               << "tcolor.rgb * (1 - tcolor_" << i << " .a); // BLENDING: Replace\n"
               << "tcolor.a = tcolor_" << i << ".a + tcolor.a * (1 - tcolor_" << i
               << " .a); // BLENDING: Replace\n\n";
            break;
          case vtkTexture::VTK_TEXTURE_BLENDING_MODE_MODULATE:
            ss << "tcolor *= tcolor_" << i << "; // BLENDING: Modulate\n\n";
            break;
          case vtkTexture::VTK_TEXTURE_BLENDING_MODE_ADD:
            ss << "tcolor.rgb = tcolor_" << i << ".rgb * tcolor_" << i << ".a + "
               << "tcolor.rgb * tcolor.a; // BLENDING: Add\n"
               << "tcolor.a += tcolor_" << i << ".a; // BLENDING: Add\n\n";
            break;
          case vtkTexture::VTK_TEXTURE_BLENDING_MODE_ADD_SIGNED:
            ss << "tcolor.rgb = tcolor_" << i << ".rgb * tcolor_" << i << ".a + "
               << "tcolor.rgb * tcolor.a - 0.5; // BLENDING: Add signed\n"
               << "tcolor.a += tcolor_" << i << ".a - 0.5; // BLENDING: Add signed\n\n";
            break;
          case vtkTexture::VTK_TEXTURE_BLENDING_MODE_INTERPOLATE:
            vtkDebugMacro(<< "Interpolate blending mode not supported for OpenGL2 backend.");
            break;
          case vtkTexture::VTK_TEXTURE_BLENDING_MODE_SUBTRACT:
            ss << "tcolor.rgb -= tcolor_" << i << ".rgb * tcolor_" << i
               << ".a; // BLENDING: Subtract\n\n";
            break;
          default:
            vtkDebugMacro(<< "No blending mode given, ignoring this texture colors.");
            ss << "// NO BLENDING MODE: ignoring this texture colors\n";
        }
      }
      tCoordImpFS += ss.str();
    }

    if (nbTex2d > 0)
    {
      vtkShaderProgram::Substitute(fsSource, "//VTK::TCoord::Impl",
        tCoordImpFS +
          "if (gl_FrontFacing == true || showTexturesOnBackface) {"
          "gl_FragData[0] = gl_FragData[0] * tcolor; }");
    }
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryPolyDataMapper::SetShaderParameters(vtkRenderer* renderer, vtkActor* actor)
{
  if (!this->ShaderProgram)
  {
    return;
  }

  // set uniform values
  int vp[4] = {};
  auto renWin = vtkOpenGLRenderWindow::SafeDownCast(renderer->GetRenderWindow());
  vtkOpenGLState* ostate = renWin->GetState();
  ostate->vtkglGetIntegerv(GL_VIEWPORT, vp);
  float vpDims[4];
  for (int i = 0; i < 4; ++i)
  {
    vpDims[i] = vp[i];
  }
  const float lineWidth = actor->GetProperty()->GetLineWidth();

  this->ShaderProgram->SetUniform4f("viewportDimensions", vpDims);
  this->ShaderProgram->SetUniformf("lineWidthStepSize", lineWidth / vtkMath::Ceil(lineWidth));
  this->ShaderProgram->SetUniformf("halfLineWidth", lineWidth / 2.0);
  this->ShaderProgram->SetUniform3f("vertex_color", actor->GetProperty()->GetVertexColor());
  this->ShaderProgram->SetUniform3f("edgeColor", actor->GetProperty()->GetEdgeColor());
  this->ShaderProgram->SetUniformf("edgeOpacity", actor->GetProperty()->GetEdgeOpacity());
  this->ShaderProgram->SetUniformi("edgeVisibility", actor->GetProperty()->GetEdgeVisibility());
  this->ShaderProgram->SetUniformi(
    "wireframe", actor->GetProperty()->GetRepresentation() == VTK_WIREFRAME);

  vtkHardwareSelector* selector = renderer->GetSelector();
  if (selector && this->ShaderProgram->IsUniformUsed("mapperIndex"))
  {
    this->ShaderProgram->SetUniform3f("mapperIndex", selector->GetPropColorValue());
  }

  // textures
  if (this->HaveTextures(actor))
  {
    this->ShaderProgram->SetUniformi(
      "showTexturesOnBackface", actor->GetProperty()->GetShowTexturesOnBackface() ? 1 : 0);

    std::vector<TextureInfo> textures = this->GetTextures(actor);
    for (size_t i = 0; i < textures.size(); ++i)
    {
      vtkTexture* texture = textures[i].first;
      if (texture && this->ShaderProgram->IsUniformUsed(textures[i].second.c_str()))
      {
        int tunit = vtkOpenGLTexture::SafeDownCast(texture)->GetTextureUnit();
        this->ShaderProgram->SetUniformi(textures[i].second.c_str(), tunit);
      }
    }

    // check for tcoord transform matrix
    vtkInformation* info = actor->GetPropertyKeys();
    vtkOpenGLCheckErrorMacro("failed after Render");
    if (info && info->Has(vtkProp::GeneralTextureTransform()) &&
      this->ShaderProgram->IsUniformUsed("tcMatrix"))
    {
      double* dmatrix = info->Get(vtkProp::GeneralTextureTransform());
      float fmatrix[16];
      for (int i = 0; i < 4; i++)
      {
        for (int j = 0; j < 4; j++)
        {
          fmatrix[j * 4 + i] = dmatrix[i * 4 + j];
        }
      }
      this->ShaderProgram->SetUniformMatrix4x4("tcMatrix", fmatrix);
      vtkOpenGLCheckErrorMacro("failed after Render");
    }
  }
}

//------------------------------------------------------------------------------
bool vtkOpenGLLowMemoryPolyDataMapper::GetCoordShiftAndScaleEnabled()
{
  auto value = vtkOpenGLVertexBufferObject::GetGlobalCoordShiftAndScaleEnabled()
    ? this->CoordinateShiftAndScaleInUse
    : false;
  vtkDebugMacro(<< this->GetClassName() << " (" << this
                << "): returning CoordShiftAndScaleEnabled of " << value);
  return value;
}

//------------------------------------------------------------------------------
int vtkOpenGLLowMemoryPolyDataMapper::GetCoordShiftAndScaleMethod()
{
  auto value = vtkOpenGLVertexBufferObject::GetGlobalCoordShiftAndScaleEnabled()
    ? this->ShiftScaleMethod
    : ShiftScaleMethodType::DISABLE_SHIFT_SCALE;
  vtkDebugMacro(<< this->GetClassName() << " (" << this
                << "): returning CoordShiftAndScaleMethod of " << static_cast<int>(value));
  return value;
}

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryPolyDataMapper::ComputeShiftScaleTransform(vtkRenderer*, vtkActor*)
{
  if (this->CoordinateShiftAndScaleInUse)
  {
    this->SSInverseTransform->Identity();
    this->SSInverseTransform->Translate(
      this->ShiftValues[0], this->ShiftValues[1], this->ShiftValues[2]);
    this->SSInverseTransform->Scale(
      1.0 / this->ScaleValues[0], 1.0 / this->ScaleValues[1], 1.0 / this->ScaleValues[2]);
    this->SSInverseTransform->GetTranspose(this->SSMatrix);
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryPolyDataMapper::UpdateShiftScale(vtkRenderer* renderer, vtkActor* actor)
{
  this->ComputeShiftScale(renderer, actor, this->CurrentInput->GetPoints()->GetData());
}

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryPolyDataMapper::ComputeShiftScale(
  vtkRenderer* renderer, vtkActor* actor, vtkDataArray* array)
{
  if (array->GetNumberOfComponents() != 3)
  {
    vtkWarningMacro(<< "Shift scale cannot be computed for " << array->GetObjectDescription()
                    << " because it does not have 3 components.");
    return;
  }

  // first consider auto
  bool useSS = false;
  if (this->GetCoordShiftAndScaleMethod() == ShiftScaleMethodType::AUTO_SHIFT_SCALE)
  {
    // first compute the diagonal size and distance from origin for this data
    // we use squared values to avoid sqrt calls
    double diag2 = 0.0;
    double dist2 = 0.0;
    for (int i = 0; i < 3; ++i)
    {
      double range[2];
      array->GetRange(range, i);
      double delta = range[1] - range[0];
      diag2 += (delta * delta);
      double dshift = 0.5 * (range[1] + range[0]);
      dist2 += (dshift * dshift);
    }
    // if the data is far from the origin relative to it's size
    // or if the size itself is huge when not far from the origin
    // or if it is a point, but far from the origin
    if ((diag2 > 0 && (fabs(dist2) / diag2 > 1.0e6 || fabs(log10(diag2)) > 3.0)) ||
      (diag2 == 0 && dist2 > 1.0e6))
    {
      useSS = true;
    }
    else if (this->CoordinateShiftAndScaleInUse)
    {
      // make sure to reset if we go far away and come back.
      this->CoordinateShiftAndScaleInUse = false;
      this->ShiftValues.fill(0);
      this->ScaleValues.fill(1);
      return;
    }
  }

  if (useSS || this->GetCoordShiftAndScaleMethod() == ShiftScaleMethodType::ALWAYS_AUTO_SHIFT_SCALE)
  {
    std::array<double, 3> shift;
    std::array<double, 3> scale;
    for (int i = 0; i < 3; ++i)
    {
      double range[2];
      array->GetRange(range, i);
      shift[i] = (0.5 * (range[1] + range[0]));
      double delta = range[1] - range[0];
      if (delta > 0)
      {
        scale[i] = (1.0 / delta);
      }
      else
      {
        scale[i] = (1.0);
      }
    }
    this->SetShiftValues(shift[0], shift[1], shift[2]);
    this->SetScaleValues(scale[0], scale[1], scale[2]);
    return;
  }

  if (this->GetCoordShiftAndScaleMethod() == ShiftScaleMethodType::AUTO_SHIFT)
  {
    std::array<double, 3> shift;
    for (int i = 0; i < 3; ++i)
    {
      double range[2];
      array->GetRange(range, i);
      shift[i] = (0.5 * (range[1] + range[0]));
    }
    this->SetScaleValues(1.0, 1.0, 1.0);
    this->SetShiftValues(shift[0], shift[1], shift[2]);
    return;
  }

  auto camera = renderer->GetActiveCamera();
  if (camera && actor &&
    (this->GetCoordShiftAndScaleMethod() == ShiftScaleMethodType::NEAR_PLANE_SHIFT_SCALE ||
      this->GetCoordShiftAndScaleMethod() == ShiftScaleMethodType::FOCAL_POINT_SHIFT_SCALE))
  {
    vtkCamera* cam = camera;
    double amatrix[16];
    actor->GetMatrix(amatrix);

    double* ishift = cam->GetNearPlaneShift();
    double iscale = cam->GetNearPlaneScale();
    if (this->GetCoordShiftAndScaleMethod() == ShiftScaleMethodType::FOCAL_POINT_SHIFT_SCALE)
    {
      ishift = cam->GetFocalPointShift();
      iscale = cam->GetFocalPointScale();
    }

    // push camera values through inverse actor matrix
    double imatrix[16];
    vtkMatrix4x4::Invert(amatrix, imatrix);

    double tmp[4];
    tmp[0] = ishift[0];
    tmp[1] = ishift[1];
    tmp[2] = ishift[2];
    tmp[3] = 1;
    vtkMatrix4x4::MultiplyPoint(imatrix, tmp, tmp);
    this->SetShiftValues(tmp[0] / tmp[3], tmp[1] / tmp[3], tmp[2] / tmp[3]);

    tmp[0] = iscale;
    tmp[1] = iscale;
    tmp[2] = iscale;
    tmp[3] = 1;
    vtkMatrix4x4::MultiplyPoint(imatrix, tmp, tmp);
    this->SetScaleValues(tmp[0] ? tmp[3] / tmp[0] : 1.0, tmp[1] ? tmp[3] / tmp[1] : 1.0,
      tmp[2] ? tmp[3] / tmp[2] : 1.0);
    return;
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryPolyDataMapper::SetShiftValues(double x, double y, double z)
{
  if (x == this->ShiftValues[0] && y == this->ShiftValues[1] && z == this->ShiftValues[2])
  {
    return;
  }

  this->ShiftScaleTimeStamp.Modified();
  this->CoordinateShiftAndScaleInUse = false;
  this->ShiftValues[0] = x;
  this->ShiftValues[1] = y;
  this->ShiftValues[2] = z;
  for (std::size_t i = 0; i < 3; ++i)
  {
    if (this->ShiftValues.at(i) != 0.0)
    {
      this->CoordinateShiftAndScaleInUse = true;
      return;
    }
  }
  for (unsigned int i = 0; i < this->ScaleValues.size(); ++i)
  {
    if (this->ScaleValues.at(i) != 1.0)
    {
      this->CoordinateShiftAndScaleInUse = true;
      return;
    }
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryPolyDataMapper::SetScaleValues(double x, double y, double z)
{
  if (x == this->ScaleValues[0] && y == this->ScaleValues[1] && z == this->ScaleValues[2])
  {
    return;
  }

  this->ShiftScaleTimeStamp.Modified();
  this->CoordinateShiftAndScaleInUse = false;
  this->ScaleValues[0] = x;
  this->ScaleValues[1] = y;
  this->ScaleValues[2] = z;
  for (std::size_t i = 0; i < 3; ++i)
  {
    if (this->ShiftValues.at(i) != 0.0)
    {
      this->CoordinateShiftAndScaleInUse = true;
      return;
    }
  }
  for (unsigned int i = 0; i < this->ScaleValues.size(); ++i)
  {
    if (this->ScaleValues.at(i) != 1.0)
    {
      this->CoordinateShiftAndScaleInUse = true;
      return;
    }
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryPolyDataMapper::ComputeCameraBasedShiftScale(
  vtkRenderer* renderer, vtkActor* actor, vtkPolyData* mesh)
{
  if (this->PauseShiftScale)
  {
    return;
  }
  // only when shift scale method is one of these two.
  switch (this->ShiftScaleMethod)
  {
    case ShiftScaleMethodType::NEAR_PLANE_SHIFT_SCALE:
    case ShiftScaleMethodType::FOCAL_POINT_SHIFT_SCALE:
    {
      using namespace vtk::literals;
      auto positionsIt = this->Arrays.find("positions"_token);
      if (positionsIt != this->Arrays.end())
      {
        this->ComputeShiftScale(renderer, actor, mesh->GetPoints()->GetData());
      }
      break;
    }
    default:
      break;
  }
}

//------------------------------------------------------------------------------
vtkOpenGLLowMemoryPolyDataMapper::ShaderColorSourceAttribute
vtkOpenGLLowMemoryPolyDataMapper::DetermineShaderColorSource(vtkPolyData* mesh)
{
  auto colors = this->GetColors(mesh);
  // Determine where the colors come from.
  auto result = ShaderColorSourceAttribute::Uniform;
  if (this->ScalarVisibility)
  {
    if (colors)
    {
      result = ShaderColorSourceAttribute::Point;
    }
    // We must figure out how the scalars should be mapped to the polydata.
    if ((this->ScalarMode == VTK_SCALAR_MODE_USE_CELL_DATA ||
          this->ScalarMode == VTK_SCALAR_MODE_USE_CELL_FIELD_DATA ||
          this->ScalarMode == VTK_SCALAR_MODE_USE_FIELD_DATA ||
          !mesh->GetPointData()->GetScalars()) &&
      this->ScalarMode != VTK_SCALAR_MODE_USE_POINT_FIELD_DATA && colors &&
      colors->GetNumberOfTuples() > 0)
    {
      result = ShaderColorSourceAttribute::Cell;
    }
  }
  if (this->InterpolateScalarsBeforeMapping && this->ColorCoordinates)
  {
    result = ShaderColorSourceAttribute::PointTexture;
  }
  return result;
}

//------------------------------------------------------------------------------
vtkOpenGLLowMemoryPolyDataMapper::ShaderNormalSourceAttribute
vtkOpenGLLowMemoryPolyDataMapper::DetermineShaderNormalSource(vtkActor* actor, vtkPolyData* mesh)
{
  // Determine where the normals come from.
  auto result = ShaderNormalSourceAttribute::Primitive;
  if (actor->GetProperty()->GetInterpolation() != VTK_FLAT)
  {
    if (this->GetPointNormals(mesh))
    {
      result = ShaderNormalSourceAttribute::Point;
    }
  }
  // if we have cell normals, use those.
  if (result == ShaderNormalSourceAttribute::Primitive && this->GetCellNormals(mesh) != nullptr)
  {
    result = ShaderNormalSourceAttribute::Cell;
  }
  return result;
}

//------------------------------------------------------------------------------
vtkDataArray* vtkOpenGLLowMemoryPolyDataMapper::GetColors(vtkPolyData* mesh)
{
  int cellFlag; // not used
  this->MapScalars(mesh, 1.0, cellFlag);
  return this->Colors;
}

//------------------------------------------------------------------------------
vtkDataArray* vtkOpenGLLowMemoryPolyDataMapper::GetPointNormals(vtkPolyData* mesh)
{
  return mesh->GetPointData()->GetNormals();
}

//------------------------------------------------------------------------------
vtkDataArray* vtkOpenGLLowMemoryPolyDataMapper::GetPointTangents(vtkPolyData* mesh)
{
  return mesh->GetPointData()->GetTangents();
}

//------------------------------------------------------------------------------
vtkDataArray* vtkOpenGLLowMemoryPolyDataMapper::GetTextureCoordinates(vtkPolyData* mesh)
{
  return mesh->GetPointData()->GetTCoords();
}

//------------------------------------------------------------------------------
vtkDataArray* vtkOpenGLLowMemoryPolyDataMapper::GetColorTextureCoordinates(vtkPolyData*)
{
  vtkDataArray* colorTCoords = nullptr;
  if (this->InterpolateScalarsBeforeMapping && this->ColorCoordinates)
  {
    colorTCoords = this->ColorCoordinates;
  }
  return colorTCoords;
}

//------------------------------------------------------------------------------
vtkDataArray* vtkOpenGLLowMemoryPolyDataMapper::GetCellNormals(vtkPolyData* mesh)
{
  return mesh->GetCellData()->GetNormals();
}

//------------------------------------------------------------------------------
bool vtkOpenGLLowMemoryPolyDataMapper::HaveTextures(vtkActor* actor)
{
  return (this->GetNumberOfTextures(actor) > 0);
}

//------------------------------------------------------------------------------
unsigned int vtkOpenGLLowMemoryPolyDataMapper::GetNumberOfTextures(vtkActor* actor)
{
  unsigned int res = 0;
  if (this->ColorTextureMap)
  {
    res++;
  }
  if (actor->GetTexture())
  {
    res++;
  }
  res += actor->GetProperty()->GetNumberOfTextures();
  return res;
}

//------------------------------------------------------------------------------
std::vector<vtkOpenGLLowMemoryPolyDataMapper::TextureInfo>
vtkOpenGLLowMemoryPolyDataMapper::GetTextures(vtkActor* actor)
{
  std::vector<TextureInfo> result;
  if (this->ColorTextureMap)
  {
    result.emplace_back(this->InternalColorTexture, "colortexture");
  }
  if (actor->GetTexture())
  {
    result.emplace_back(actor->GetTexture(), "actortexture");
  }
  auto textures = actor->GetProperty()->GetAllTextures();
  for (const auto& ti : textures)
  {
    result.emplace_back(ti.second, ti.first);
  }
  return result;
}

//------------------------------------------------------------------------------
std::pair<std::string, std::string>
vtkOpenGLLowMemoryPolyDataMapper::GetTextureCoordinateAndSamplerBufferNames(const char* tname)
{
  for (const auto& it : this->ExtraAttributes)
  {
    if (it.second.TextureName == tname)
    {
      return { it.first, it.second.DataArrayName };
    }
  }

  // Return the attribute name of the specific tcoords used for scalar coloring with texture
  if (tname == std::string("colortexture"))
  {
    return { "colorTCoord", "colorTCoords" };
  }

  return { "tcoord", "tcoords" };
}

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryPolyDataMapper::UpdatePBRStateCache(vtkRenderer*, vtkActor* actor)
{
  bool hasAnisotropy = actor->GetProperty()->GetInterpolation() == VTK_PBR &&
    actor->GetProperty()->GetAnisotropy() != 0.0;
  bool hasClearCoat = actor->GetProperty()->GetInterpolation() == VTK_PBR &&
    actor->GetProperty()->GetCoatStrength() > 0.0;

  std::vector<TextureInfo> textures = this->GetTextures(actor);
  bool usesNormalMap = std::find_if(textures.begin(), textures.end(), [](const TextureInfo& tex) {
    return tex.second == "normalTex";
  }) != textures.end();
  bool usesCoatNormalMap = this->HasClearCoat &&
    std::find_if(textures.begin(), textures.end(),
      [](const TextureInfo& tex) { return tex.second == "coatNormalTex"; }) != textures.end();
  bool usesRotationMap = std::find_if(textures.begin(), textures.end(), [](const TextureInfo& tex) {
    return tex.second == "anisotropyTex";
  }) != textures.end();

  if (hasAnisotropy != this->HasAnisotropy)
  {
    this->HasAnisotropy = hasAnisotropy;
    this->PBRStateTimeStamp.Modified();
  }
  if (hasClearCoat != this->HasClearCoat)
  {
    this->HasClearCoat = hasClearCoat;
    this->PBRStateTimeStamp.Modified();
  }
  if (usesNormalMap != this->UsesNormalMap)
  {
    this->UsesNormalMap = usesNormalMap;
    this->PBRStateTimeStamp.Modified();
  }
  if (usesCoatNormalMap != this->UsesCoatNormalMap)
  {
    this->UsesCoatNormalMap = usesCoatNormalMap;
    this->PBRStateTimeStamp.Modified();
  }
  if (usesRotationMap != this->UsesRotationMap)
  {
    this->UsesRotationMap = usesRotationMap;
    this->PBRStateTimeStamp.Modified();
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryPolyDataMapper::UpdateGLSLMods(vtkRenderer*, vtkActor*)
{
  auto modsIter = vtk::TakeSmartPointer(this->GLSLMods->NewIterator());
  for (modsIter->InitTraversal(); !modsIter->IsDoneWithTraversal(); modsIter->GoToNextItem())
  {
    if (auto cameraMod = vtkGLSLModCamera::SafeDownCast(modsIter->GetCurrentObject()))
    {
      // camera mod needs additional information before they can set shader parameters.
      if (this->CoordinateShiftAndScaleInUse)
      {
        cameraMod->EnableShiftScale(this->CoordinateShiftAndScaleInUse, this->SSMatrix);
      }
      else
      {
        cameraMod->DisableShiftScale();
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryPolyDataMapper::MapDataArrayToVertexAttribute(
  const char* vertexAttributeName, const char* dataArrayName, int fieldAssociation, int componentno)
{
  this->MapDataArray(vertexAttributeName, dataArrayName, "", fieldAssociation, componentno);
}

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryPolyDataMapper::MapDataArrayToMultiTextureAttribute(
  const char* tname, const char* dataArrayName, int fieldAssociation, int componentno)
{
  std::string coordname = tname;
  coordname += "_coord";
  this->MapDataArray(coordname.c_str(), dataArrayName, tname, fieldAssociation, componentno);
}

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryPolyDataMapper::MapDataArray(const char* vertexAttributeName,
  const char* dataArrayName, const char* tname, int fieldAssociation, int componentno)
{
  if (!vertexAttributeName)
  {
    return;
  }

  // store the mapping in the map
  this->RemoveVertexAttributeMapping(vertexAttributeName);
  if (!dataArrayName)
  {
    return;
  }

  vtkOpenGLLowMemoryPolyDataMapper::ExtraAttributeValue aval;
  aval.DataArrayName = dataArrayName;
  aval.FieldAssociation = fieldAssociation;
  aval.ComponentNumber = componentno;
  aval.TextureName = tname;

  this->ExtraAttributes.insert(std::make_pair(vertexAttributeName, aval));

  this->Modified();
}

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryPolyDataMapper::RemoveVertexAttributeMapping(const char* vertexAttributeName)
{
  auto itr = this->ExtraAttributes.find(vertexAttributeName);
  if (itr != this->ExtraAttributes.end())
  {
    this->UnbindArray(vtkStringToken(vertexAttributeName));
    this->ExtraAttributes.erase(itr);
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryPolyDataMapper::RemoveAllVertexAttributeMappings()
{
  for (auto itr = this->ExtraAttributes.begin(); itr != this->ExtraAttributes.end();
       itr = this->ExtraAttributes.begin())
  {
    this->RemoveVertexAttributeMapping(itr->first.c_str());
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryPolyDataMapper::ProcessSelectorPixelBuffers(
  vtkHardwareSelector* sel, std::vector<unsigned int>& pixeloffsets, vtkProp*)
{
  vtkPolyData* mesh = this->CurrentInput;

  if (!this->PopulateSelectionSettings || !mesh)
  {
    return;
  }

  // which pass are we processing ?
  int currPass = sel->GetCurrentPass();

  // get some common useful values
  vtkPointData* pd = mesh->GetPointData();
  vtkCellData* cd = mesh->GetCellData();
  unsigned char* rawplowdata = sel->GetRawPixelBuffer(vtkHardwareSelector::POINT_ID_LOW24);
  unsigned char* rawphighdata = sel->GetRawPixelBuffer(vtkHardwareSelector::POINT_ID_HIGH24);

  // handle process pass
  if (currPass == vtkHardwareSelector::PROCESS_PASS)
  {
    vtkUnsignedIntArray* processArray = nullptr;

    // point data is used for process_pass which seems odd
    if (sel->GetUseProcessIdFromData())
    {
      processArray = !this->ProcessIdArrayName.empty()
        ? vtkArrayDownCast<vtkUnsignedIntArray>(pd->GetArray(this->ProcessIdArrayName.c_str()))
        : nullptr;
    }

    // do we need to do anything to the process pass data?
    unsigned char* processdata = sel->GetRawPixelBuffer(vtkHardwareSelector::PROCESS_PASS);
    if (processArray && processdata && rawplowdata)
    {
      // get the buffer pointers we need
      for (auto pos : pixeloffsets)
      {
        unsigned int inval = 0;
        if (rawphighdata)
        {
          inval = rawphighdata[pos];
          inval = inval << 8;
        }
        inval |= rawplowdata[pos + 2];
        inval = inval << 8;
        inval |= rawplowdata[pos + 1];
        inval = inval << 8;
        inval |= rawplowdata[pos];
        unsigned int outval = processArray->GetValue(inval) + 1;
        processdata[pos] = outval & 0xff;
        processdata[pos + 1] = (outval & 0xff00) >> 8;
        processdata[pos + 2] = (outval & 0xff0000) >> 16;
      }
    }
  }

  if (currPass == vtkHardwareSelector::POINT_ID_LOW24)
  {
    vtkIdTypeArray* pointArrayId = !this->PointIdArrayName.empty()
      ? vtkArrayDownCast<vtkIdTypeArray>(pd->GetArray(this->PointIdArrayName.c_str()))
      : nullptr;

    // do we need to do anything to the point id data?
    if (rawplowdata && pointArrayId)
    {
      unsigned char* plowdata = sel->GetPixelBuffer(vtkHardwareSelector::POINT_ID_LOW24);

      for (auto pos : pixeloffsets)
      {
        unsigned int inval = 0;
        if (rawphighdata)
        {
          inval = rawphighdata[pos];
          inval = inval << 8;
        }
        inval |= rawplowdata[pos + 2];
        inval = inval << 8;
        inval |= rawplowdata[pos + 1];
        inval = inval << 8;
        inval |= rawplowdata[pos];
        vtkIdType outval = pointArrayId->GetValue(inval);
        plowdata[pos] = outval & 0xff;
        plowdata[pos + 1] = (outval & 0xff00) >> 8;
        plowdata[pos + 2] = (outval & 0xff0000) >> 16;
      }
    }
  }

  if (currPass == vtkHardwareSelector::POINT_ID_HIGH24)
  {
    vtkIdTypeArray* pointArrayId = !this->PointIdArrayName.empty()
      ? vtkArrayDownCast<vtkIdTypeArray>(pd->GetArray(this->PointIdArrayName.c_str()))
      : nullptr;

    // do we need to do anything to the point id data?
    if (rawphighdata && pointArrayId)
    {
      unsigned char* phighdata = sel->GetPixelBuffer(vtkHardwareSelector::POINT_ID_HIGH24);

      for (auto pos : pixeloffsets)
      {
        unsigned int inval = 0;
        inval = rawphighdata[pos];
        inval = inval << 8;
        inval |= rawplowdata[pos + 2];
        inval = inval << 8;
        inval |= rawplowdata[pos + 1];
        inval = inval << 8;
        inval |= rawplowdata[pos];
        vtkIdType outval = pointArrayId->GetValue(inval);
        phighdata[pos] = (outval & 0xff000000) >> 24;
        phighdata[pos + 1] = (outval & 0xff00000000) >> 32;
        phighdata[pos + 2] = (outval & 0xff0000000000) >> 40;
      }
    }
  }

  unsigned char* rawclowdata = sel->GetRawPixelBuffer(vtkHardwareSelector::CELL_ID_LOW24);
  unsigned char* rawchighdata = sel->GetRawPixelBuffer(vtkHardwareSelector::CELL_ID_HIGH24);

  // do we need to do anything to the composite pass data?
  if (currPass == vtkHardwareSelector::COMPOSITE_INDEX_PASS)
  {
    unsigned char* compositedata = sel->GetPixelBuffer(vtkHardwareSelector::COMPOSITE_INDEX_PASS);

    vtkUnsignedIntArray* compositeArray = !this->CompositeIdArrayName.empty()
      ? vtkArrayDownCast<vtkUnsignedIntArray>(cd->GetArray(this->CompositeIdArrayName.c_str()))
      : nullptr;

    if (compositedata && compositeArray && rawclowdata)
    {
      for (auto pos : pixeloffsets)
      {
        unsigned int inval = 0;
        if (rawchighdata)
        {
          inval = rawchighdata[pos];
          inval = inval << 8;
        }
        inval |= rawclowdata[pos + 2];
        inval = inval << 8;
        inval |= rawclowdata[pos + 1];
        inval = inval << 8;
        inval |= rawclowdata[pos];
        vtkIdType cellId = inval;
        unsigned int outval = compositeArray->GetValue(cellId);
        compositedata[pos] = outval & 0xff;
        compositedata[pos + 1] = (outval & 0xff00) >> 8;
        compositedata[pos + 2] = (outval & 0xff0000) >> 16;
      }
    }
  }

  // process the cellid array?
  if (currPass == vtkHardwareSelector::CELL_ID_LOW24)
  {
    vtkIdTypeArray* cellArrayId = !this->CellIdArrayName.empty()
      ? vtkArrayDownCast<vtkIdTypeArray>(cd->GetArray(this->CellIdArrayName.c_str()))
      : nullptr;
    unsigned char* clowdata = sel->GetPixelBuffer(vtkHardwareSelector::CELL_ID_LOW24);

    if (rawclowdata)
    {
      for (auto pos : pixeloffsets)
      {
        unsigned int inval = 0;
        if (rawchighdata)
        {
          inval = rawchighdata[pos];
          inval = inval << 8;
        }
        inval |= rawclowdata[pos + 2];
        inval = inval << 8;
        inval |= rawclowdata[pos + 1];
        inval = inval << 8;
        inval |= rawclowdata[pos];
        vtkIdType outval = inval;
        if (cellArrayId)
        {
          outval = cellArrayId->GetValue(outval);
        }
        clowdata[pos] = outval & 0xff;
        clowdata[pos + 1] = (outval & 0xff00) >> 8;
        clowdata[pos + 2] = (outval & 0xff0000) >> 16;
      }
    }
  }

  if (currPass == vtkHardwareSelector::CELL_ID_HIGH24)
  {
    vtkIdTypeArray* cellArrayId = !this->CellIdArrayName.empty()
      ? vtkArrayDownCast<vtkIdTypeArray>(cd->GetArray(this->CellIdArrayName.c_str()))
      : nullptr;
    unsigned char* chighdata = sel->GetPixelBuffer(vtkHardwareSelector::CELL_ID_HIGH24);

    if (rawchighdata)
    {
      for (auto pos : pixeloffsets)
      {
        unsigned int inval = 0;
        inval = rawchighdata[pos];
        inval = inval << 8;
        inval |= rawclowdata[pos + 2];
        inval = inval << 8;
        inval |= rawclowdata[pos + 1];
        inval = inval << 8;
        inval |= rawclowdata[pos];
        vtkIdType outval = inval;
        if (cellArrayId)
        {
          outval = cellArrayId->GetValue(outval);
        }
        chighdata[pos] = (outval & 0xff000000) >> 24;
        chighdata[pos + 1] = (outval & 0xff00000000) >> 32;
        chighdata[pos + 2] = (outval & 0xff0000000000) >> 40;
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryPolyDataMapper::UpdateMaximumPointCellIds(vtkRenderer* ren, vtkActor*)
{
  vtkPolyData* mesh = this->CurrentInput;
  vtkHardwareSelector* selector = ren->GetSelector();

  // our maximum point id is the is the index of the max of
  // 1) the maximum used value in our points array
  // 2) the largest used value in a provided pointIdArray
  // To make this quicker we use the number of points for (1)
  // and the max range for (2)
  vtkIdType maxPointId = mesh->GetPoints()->GetNumberOfPoints() - 1;
  if (mesh && mesh->GetPointData())
  {
    vtkIdTypeArray* pointArrayId = !this->PointIdArrayName.empty()
      ? vtkArrayDownCast<vtkIdTypeArray>(
          mesh->GetPointData()->GetArray(this->PointIdArrayName.c_str()))
      : nullptr;
    if (pointArrayId)
    {
      maxPointId =
        maxPointId < pointArrayId->GetRange()[1] ? pointArrayId->GetRange()[1] : maxPointId;
    }
  }
  selector->UpdateMaximumPointId(maxPointId);

  // the maximum number of cells in a draw call is the max of
  // 1) the number of cells
  // 2) the max of any used call in a cellIdArray
  vtkIdType maxCellId = mesh->GetNumberOfCells() - 1;
  if (mesh && mesh->GetCellData())
  {
    vtkIdTypeArray* cellArrayId = !this->CellIdArrayName.empty()
      ? vtkArrayDownCast<vtkIdTypeArray>(
          mesh->GetCellData()->GetArray(this->CellIdArrayName.c_str()))
      : nullptr;
    if (cellArrayId)
    {
      maxCellId = maxCellId < cellArrayId->GetRange()[1] ? cellArrayId->GetRange()[1] : maxCellId;
    }
  }
  selector->UpdateMaximumCellId(maxCellId);
}
VTK_ABI_NAMESPACE_END;

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositePolyDataMapper2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtk_glew.h"

#include "vtkCompositePolyDataMapper2.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCommand.h"
#include "vtkCompositeDataDisplayAttributes.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkFloatArray.h"
#include "vtkHardwareSelector.h"
#include "vtkImageData.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLBufferObject.h"
#include "vtkOpenGLIndexBufferObject.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLTexture.h"
#include "vtkOpenGLVertexBufferObject.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkLookupTable.h"
#include "vtkShaderProgram.h"
#include "vtkTextureObject.h"

vtkStandardNewMacro(vtkCompositePolyDataMapper2);

//----------------------------------------------------------------------------
vtkCompositePolyDataMapper2::vtkCompositePolyDataMapper2()
{
  this->UseGeneric = true;
}

//----------------------------------------------------------------------------
vtkCompositePolyDataMapper2::~vtkCompositePolyDataMapper2()
{
}

//----------------------------------------------------------------------------
void vtkCompositePolyDataMapper2::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

void vtkCompositePolyDataMapper2::FreeStructures()
{
  this->VertexOffsets.resize(0);
  this->IndexOffsets.resize(0);
  this->IndexArray.resize(0);
  this->EdgeIndexArray.resize(0);
  this->EdgeIndexOffsets.resize(0);
  this->RenderValues.resize(0);
}

void vtkCompositePolyDataMapper2::ReplaceShaderColor(
  std::map<vtkShader::Type, vtkShader *> shaders,
  vtkRenderer *ren, vtkActor *actor)
{
  std::string FSSource = shaders[vtkShader::Fragment]->GetSource();

  vtkShaderProgram::Substitute(FSSource,"//VTK::Color::Dec",
    "uniform bool OverridesColor;\n"
    "//VTK::Color::Dec",false);

  vtkShaderProgram::Substitute(FSSource,"//VTK::Color::Impl",
    "//VTK::Color::Impl\n"
    "  if (OverridesColor) {\n"
    "    ambientColor = ambientColorUniform;\n"
    "    diffuseColor = diffuseColorUniform; }\n",
    false);

  shaders[vtkShader::Fragment]->SetSource(FSSource);

  this->Superclass::ReplaceShaderColor(shaders,ren,actor);
}

// ---------------------------------------------------------------------------
// Description:
// Method initiates the mapping process. Generally sent by the actor
// as each frame is rendered.
void vtkCompositePolyDataMapper2::Render(
  vtkRenderer *ren, vtkActor *actor)
{
  vtkCompositeDataSet *input = vtkCompositeDataSet::SafeDownCast(
    this->GetInputDataObject(0, 0));

  // do we need to do a generic render?
  bool lastUseGeneric = this->UseGeneric;
  if (this->GenericTestTime < this->GetInputDataObject(0, 0)->GetMTime())
    {
    this->UseGeneric = false;

    // is the data not composite
    if (!input)
      {
      this->UseGeneric = true;
      }
    else
      {
      vtkSmartPointer<vtkDataObjectTreeIterator> iter =
        vtkSmartPointer<vtkDataObjectTreeIterator>::New();
      iter->SetDataSet(input);
      iter->SkipEmptyNodesOn();
      iter->VisitOnlyLeavesOn();
      for (iter->InitTraversal(); !iter->IsDoneWithTraversal();
          iter->GoToNextItem())
        {
        vtkDataObject *dso = iter->GetCurrentDataObject();
        vtkPolyData *pd = vtkPolyData::SafeDownCast(dso);
        if (!pd ||
            pd->GetVerts()->GetNumberOfCells() ||
            pd->GetLines()->GetNumberOfCells() ||
            pd->GetStrips()->GetNumberOfCells())
          {
          this->UseGeneric = true;
          break;
          }
        }
      }

    // check if this system is subject to the apple primID bug
    // if so don't even try using the fast path, just go slow

#ifdef __APPLE__
    std::string vendor = (const char *)glGetString(GL_VENDOR);
    if (vendor.find("ATI") != std::string::npos ||
        vendor.find("AMD") != std::string::npos ||
        vendor.find("amd") != std::string::npos)
      {
      this->UseGeneric = true;
      }
#endif

    // clear old structures if the render method changed
    if (lastUseGeneric != this->UseGeneric)
      {
      if (lastUseGeneric)
        {
        this->FreeGenericStructures();
        }
      else
        {
        this->FreeStructures();
        }
      }
    this->GenericTestTime.Modified();
    }

  if (this->UseGeneric)
    {
    this->RenderGeneric(ren,actor);
    }
  else
    {
    vtkProperty* prop = actor->GetProperty();

    // Push base-values on the state stack.
    this->BlockState.Visibility.push(true);
    this->BlockState.Opacity.push(prop->GetOpacity());
    this->BlockState.AmbientColor.push(vtkColor3d(prop->GetAmbientColor()));
    this->BlockState.DiffuseColor.push(vtkColor3d(prop->GetDiffuseColor()));
    this->BlockState.SpecularColor.push(vtkColor3d(prop->GetSpecularColor()));

    // set current input
    this->CurrentInput = 0;
    vtkSmartPointer<vtkDataObjectTreeIterator> iter =
      vtkSmartPointer<vtkDataObjectTreeIterator>::New();
    iter->SetDataSet(input);
    iter->SkipEmptyNodesOn();
    iter->VisitOnlyLeavesOn();
    for (iter->InitTraversal();
        !iter->IsDoneWithTraversal() && !this->CurrentInput;
        iter->GoToNextItem())
      {
      vtkDataObject *dso = iter->GetCurrentDataObject();
      vtkPolyData *pd = vtkPolyData::SafeDownCast(dso);
      if (pd && pd->GetPoints())
        {
        this->CurrentInput = pd;
        }
      }

     if (this->CurrentInput)
      {
      // render using the composite data attributes
      this->RenderPiece(ren, actor);
      }

    this->BlockState.Visibility.pop();
    this->BlockState.Opacity.pop();
    this->BlockState.AmbientColor.pop();
    this->BlockState.DiffuseColor.pop();
    this->BlockState.SpecularColor.pop();

    this->UpdateProgress(1.0);
    }
}

//-----------------------------------------------------------------------------
void vtkCompositePolyDataMapper2::RenderPiece(
  vtkRenderer* ren, vtkActor *actor)
{
  // Make sure that we have been properly initialized.
  if (ren->GetRenderWindow()->CheckAbortStatus())
    {
    return;
    }

  this->InvokeEvent(vtkCommand::StartEvent,NULL);
  if (!this->Static)
    {
    this->GetInputAlgorithm()->Update();
    }
  this->InvokeEvent(vtkCommand::EndEvent,NULL);

  this->RenderPieceStart(ren, actor);
  this->RenderPieceDraw(ren, actor);
  this->RenderEdges(ren, actor);
  this->RenderPieceFinish(ren, actor);
}



//-----------------------------------------------------------------------------
void vtkCompositePolyDataMapper2::BuildRenderValues(
  vtkRenderer *renderer,
  vtkActor *actor,
  vtkDataObject *dobj,
  unsigned int &flat_index,
  unsigned int &lastVertex,
  unsigned int &lastIndex,
  unsigned int &lastEdgeIndex)
{
  vtkHardwareSelector *selector = renderer->GetSelector();
  vtkCompositeDataDisplayAttributes* cda = this->GetCompositeDataDisplayAttributes();
  bool overrides_visibility = (cda && cda->HasBlockVisibility(flat_index));
  if (overrides_visibility)
    {
    this->BlockState.Visibility.push(cda->GetBlockVisibility(flat_index));
    }

  bool overrides_opacity = (cda && cda->HasBlockOpacity(flat_index));
  if (overrides_opacity)
    {
    this->BlockState.Opacity.push(cda->GetBlockOpacity(flat_index));
    }

  bool overrides_color = (cda && cda->HasBlockColor(flat_index));
  if (overrides_color)
    {
    vtkColor3d color = cda->GetBlockColor(flat_index);
    this->BlockState.AmbientColor.push(color);
    this->BlockState.DiffuseColor.push(color);
    this->BlockState.SpecularColor.push(color);
    }

  unsigned int my_flat_index = flat_index;
  // Advance flat-index. After this point, flat_index no longer points to this
  // block.
  flat_index++;

  vtkMultiBlockDataSet *mbds = vtkMultiBlockDataSet::SafeDownCast(dobj);
  vtkMultiPieceDataSet *mpds = vtkMultiPieceDataSet::SafeDownCast(dobj);
  if (mbds || mpds)
    {
    unsigned int numChildren = mbds? mbds->GetNumberOfBlocks() :
      mpds->GetNumberOfPieces();
    for (unsigned int cc=0 ; cc < numChildren; cc++)
      {
      vtkDataObject* child = mbds ? mbds->GetBlock(cc) : mpds->GetPiece(cc);
      if (child == NULL)
        {
        // speeds things up when dealing with NULL blocks (which is common with
        // AMRs).
        flat_index++;
        continue;
        }
      this->BuildRenderValues(renderer, actor, child,
        flat_index, lastVertex, lastIndex, lastEdgeIndex);
      }
    }
  else
    {
    double op = this->BlockState.Opacity.top();
    bool vis = this->BlockState.Visibility.top();
    vtkColor3d color = this->BlockState.AmbientColor.top();
    if (this->RenderValues.size() == 0)
      {
      vtkCompositePolyDataMapper2::RenderValue rv;
      rv.StartVertex = 0;
      rv.StartIndex = 0;
      rv.StartEdgeIndex = 0;
      rv.Opacity = op;
      rv.Visibility = vis;
      rv.Color = color;
      rv.PickId = my_flat_index;
      rv.OverridesColor = (this->BlockState.AmbientColor.size() > 1);
      this->RenderValues.push_back(rv);
      }

    // has something changed?
    if (this->RenderValues.back().Opacity != op ||
        this->RenderValues.back().Visibility != vis ||
        this->RenderValues.back().Color != color ||
        selector)
      {
      // close old group
      this->RenderValues.back().EndVertex = lastVertex - 1;
      this->RenderValues.back().EndIndex = lastIndex - 1;
      this->RenderValues.back().EndEdgeIndex = lastEdgeIndex - 1;
      // open a new group
      vtkCompositePolyDataMapper2::RenderValue rv;
      rv.StartVertex = lastVertex;
      rv.StartIndex = lastIndex;
      rv.StartEdgeIndex = lastEdgeIndex;
      rv.Opacity = op;
      rv.Visibility = vis;
      rv.Color = color;
      rv.PickId = my_flat_index;
      rv.OverridesColor = (this->BlockState.AmbientColor.size() > 1);
      this->RenderValues.push_back(rv);
      }
    lastVertex = this->VertexOffsets[my_flat_index];
    lastIndex = this->IndexOffsets[my_flat_index];
    lastEdgeIndex = this->EdgeIndexOffsets[my_flat_index];
    }

  if (overrides_color)
    {
    this->BlockState.AmbientColor.pop();
    this->BlockState.DiffuseColor.pop();
    this->BlockState.SpecularColor.pop();
    }
  if (overrides_opacity)
    {
    this->BlockState.Opacity.pop();
    }
  if (overrides_visibility)
    {
    this->BlockState.Visibility.pop();
    }
}

//-----------------------------------------------------------------------------
void vtkCompositePolyDataMapper2::RenderPieceDraw(
  vtkRenderer* ren, vtkActor *actor)
{
  int representation = actor->GetProperty()->GetRepresentation();

  // render points for point picking in a special way
  // all cell types should be rendered as points
  vtkHardwareSelector* selector = ren->GetSelector();
  if (selector && this->PopulateSelectionSettings &&
      selector->GetFieldAssociation() == vtkDataObject::FIELD_ASSOCIATION_POINTS &&
      selector->GetCurrentPass() >= vtkHardwareSelector::ID_LOW24)
    {
    representation = VTK_POINTS;
    }

  // rebuild the render values if needed
  if (this->RenderValuesBuildTime < this->GetMTime() ||
      this->RenderValuesBuildTime < this->VBOBuildTime ||
      this->RenderValuesBuildTime < this->SelectionStateChanged)
    {
    vtkCompositeDataSet *input = vtkCompositeDataSet::SafeDownCast(
      this->GetInputDataObject(0, 0));
    unsigned int lastVertex = 0;
    unsigned int lastIndex = 0;
    unsigned int lastEdgeIndex = 0;
    this->RenderValues.resize(0);
    unsigned int flat_index = 0;
    this->BuildRenderValues(ren, actor, input,
      flat_index, lastVertex, lastIndex, lastEdgeIndex);
    // close last group
    this->RenderValues.back().EndVertex = lastVertex - 1;
    this->RenderValues.back().EndIndex = lastIndex - 1;
    this->RenderValues.back().EndEdgeIndex = lastEdgeIndex - 1;
    this->RenderValuesBuildTime.Modified();
    }

  // draw polygons
  if (this->Tris.IBO->IndexCount)
    {
    // First we do the triangles, update the shader, set uniforms, etc.
    this->UpdateShaders(this->Tris, ren, actor);
    this->Tris.IBO->Bind();
    GLenum mode = (representation == VTK_POINTS) ? GL_POINTS :
      (representation == VTK_WIREFRAME) ? GL_LINES : GL_TRIANGLES;
    unsigned int modeDenom = (representation == VTK_POINTS) ? 1 :
      (representation == VTK_WIREFRAME) ? 2 : 3;

    vtkProperty *ppty = actor->GetProperty();
    double aIntensity = this->DrawingEdges ? 1.0 : ppty->GetAmbient();
    double dIntensity = this->DrawingEdges ? 0.0 : ppty->GetDiffuse();
    vtkShaderProgram *prog = this->Tris.Program;

    std::vector<
      vtkCompositePolyDataMapper2::RenderValue>::iterator it;
    // reset the offset so each composite starts at 0
    this->PrimitiveIDOffset = 0;
    for (it = this->RenderValues.begin(); it != this->RenderValues.end(); it++)
      {
      if (it->Visibility)
        {
        if (selector &&
            selector->GetCurrentPass() ==
              vtkHardwareSelector::COMPOSITE_INDEX_PASS)
          {
          selector->RenderCompositeIndex(it->PickId);
          prog->SetUniform3f("mapperIndex", selector->GetPropColorValue());
          }
        // override the opacity and color
        prog->SetUniformf("opacityUniform", it->Opacity);
        vtkColor3d &aColor = it->Color;
        float ambientColor[3] = {static_cast<float>(aColor[0] * aIntensity),
          static_cast<float>(aColor[1] * aIntensity),
          static_cast<float>(aColor[2] * aIntensity)};
        vtkColor3d &dColor = it->Color;
        float diffuseColor[3] = {static_cast<float>(dColor[0] * dIntensity),
          static_cast<float>(dColor[1] * dIntensity),
          static_cast<float>(dColor[2] * dIntensity)};
        prog->SetUniform3f("ambientColorUniform", ambientColor);
        prog->SetUniform3f("diffuseColorUniform", diffuseColor);
        prog->SetUniformi("PrimitiveIDOffset",
          this->PrimitiveIDOffset);
        prog->SetUniformi("OverridesColor", it->OverridesColor);
        glDrawRangeElements(mode,
          static_cast<GLuint>(it->StartVertex),
          static_cast<GLuint>(it->EndVertex),
          static_cast<GLsizei>(it->EndIndex - it->StartIndex + 1),
          GL_UNSIGNED_INT,
          reinterpret_cast<const GLvoid *>(it->StartIndex*sizeof(GLuint)));
        }
      this->PrimitiveIDOffset +=
        ((it->EndIndex - it->StartIndex + 1)/modeDenom);
      }

    this->Tris.IBO->Release();
    }

}

void vtkCompositePolyDataMapper2::RenderEdges(
  vtkRenderer* ren, vtkActor *actor)
{
  vtkProperty *prop = actor->GetProperty();
  bool draw_surface_with_edges =
    (prop->GetEdgeVisibility() && prop->GetRepresentation() == VTK_SURFACE);

  if (!draw_surface_with_edges)
    {
    return;
    }

  this->DrawingEdges = true;

  // draw polygons
  if (this->TrisEdges.IBO->IndexCount)
    {
    // First we do the triangles, update the shader, set uniforms, etc.
    this->UpdateShaders(this->TrisEdges, ren, actor);
    this->TrisEdges.IBO->Bind();
    std::vector<
      vtkCompositePolyDataMapper2::RenderValue>::iterator it;
    for (it = this->RenderValues.begin(); it != this->RenderValues.end(); it++)
      {
      if (it->Visibility)
        {
        glDrawRangeElements(GL_LINES,
          static_cast<GLuint>(it->StartVertex),
          static_cast<GLuint>(it->EndVertex),
          static_cast<GLsizei>(it->EndEdgeIndex - it->StartEdgeIndex + 1),
          GL_UNSIGNED_INT,
          reinterpret_cast<const GLvoid *>(it->StartEdgeIndex*sizeof(GLuint)));
        }
      }
    this->TrisEdges.IBO->Release();
    }

  this->DrawingEdges = false;

/*
    // Disable textures when rendering the surface edges.
    // This ensures that edges are always drawn solid.
    glDisable(GL_TEXTURE_2D);

    this->Information->Set(vtkPolyDataPainter::DISABLE_SCALAR_COLOR(), 1);
    this->Information->Remove(vtkPolyDataPainter::DISABLE_SCALAR_COLOR());
    */
}

//-----------------------------------------------------------------------------
// Returns if we can use texture maps for scalar coloring. Note this doesn't say
// we "will" use scalar coloring. It says, if we do use scalar coloring, we will
// use a texture.
// When rendering multiblock datasets, if any 2 blocks provide different
// lookup tables for the scalars, then also we cannot use textures. This case can
// be handled if required.
int vtkCompositePolyDataMapper2::CanUseTextureMapForColoring(vtkDataObject*)
{
  if (!this->InterpolateScalarsBeforeMapping)
    {
    return 0; // user doesn't want us to use texture maps at all.
    }

  if (this->CanUseTextureMapForColoringSet)
    {
    return this->CanUseTextureMapForColoringValue;
    }

  vtkCompositeDataSet *cdInput = vtkCompositeDataSet::SafeDownCast(
    this->GetInputDataObject(0, 0));

  vtkSmartPointer<vtkDataObjectTreeIterator> iter =
    vtkSmartPointer<vtkDataObjectTreeIterator>::New();
  iter->SetDataSet(cdInput);
  iter->SkipEmptyNodesOn();
  iter->VisitOnlyLeavesOn();
  int cellFlag=0;
  this->CanUseTextureMapForColoringValue = 1;
  vtkScalarsToColors *scalarsLookupTable = 0;
  for (iter->InitTraversal();
       !iter->IsDoneWithTraversal() &&
          this->CanUseTextureMapForColoringValue == 1;
       iter->GoToNextItem())
    {
    vtkDataObject *dso = iter->GetCurrentDataObject();
    vtkPolyData *pd = vtkPolyData::SafeDownCast(dso);
    vtkDataArray* scalars = vtkAbstractMapper::GetScalars(pd,
      this->ScalarMode, this->ArrayAccessMode, this->ArrayId,
      this->ArrayName, cellFlag);

    if (scalars)
      {
      if (cellFlag)
        {
        this->CanUseTextureMapForColoringValue = 0;
        }
      if ((this->ColorMode == VTK_COLOR_MODE_DEFAULT &&
           vtkUnsignedCharArray::SafeDownCast(scalars)) ||
          this->ColorMode == VTK_COLOR_MODE_DIRECT_SCALARS)
        {
        // Don't use texture is direct coloring using RGB unsigned chars is
        // requested.
        this->CanUseTextureMapForColoringValue = 0;
        }

      if (scalarsLookupTable && scalars->GetLookupTable() &&
          (scalarsLookupTable != scalars->GetLookupTable()))
        {
        // Two datasets are requesting different lookup tables to color with.
        // We don't handle this case right now for composite datasets.
        this->CanUseTextureMapForColoringValue = 0;
        }
      if (scalars->GetLookupTable())
        {
        scalarsLookupTable = scalars->GetLookupTable();
        }
      }
    }

  if ((scalarsLookupTable &&
       scalarsLookupTable->GetIndexedLookup()) ||
      (!scalarsLookupTable &&
       this->LookupTable &&
       this->LookupTable->GetIndexedLookup()))
      {
      this->CanUseTextureMapForColoringValue = 0;
      }

  this->CanUseTextureMapForColoringSet = true;
  return this->CanUseTextureMapForColoringValue;
}

//-------------------------------------------------------------------------
void vtkCompositePolyDataMapper2::BuildBufferObjects(
  vtkRenderer *ren,
  vtkActor *act)
{
  vtkCompositeDataSet *input = vtkCompositeDataSet::SafeDownCast(
    this->GetInputDataObject(0, 0));

  // render using the composite data attributes
  this->VBO->VertexCount = 0;

  // compute the MaximumFlatIndex
  this->MaximumFlatIndex = 0;
  vtkSmartPointer<vtkDataObjectTreeIterator> iter =
    vtkSmartPointer<vtkDataObjectTreeIterator>::New();
  iter->SetDataSet(input);
  iter->SkipEmptyNodesOn();
  iter->VisitOnlyLeavesOn();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    this->MaximumFlatIndex = iter->GetCurrentFlatIndex();
    }
  this->VertexOffsets.resize(this->MaximumFlatIndex+1);
  this->IndexOffsets.resize(this->MaximumFlatIndex+1);
  this->EdgeIndexOffsets.resize(this->MaximumFlatIndex+1);
  this->CanUseTextureMapForColoringSet = false;

  // create the cell scalar array adjusted for ogl Cells
  std::vector<unsigned char> newColors;
  std::vector<float> newNorms;

  unsigned int voffset = 0;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    unsigned int fidx = iter->GetCurrentFlatIndex();
    vtkDataObject *dso = iter->GetCurrentDataObject();
    vtkPolyData *pd = vtkPolyData::SafeDownCast(dso);
    this->AppendOneBufferObject(ren, act, pd, voffset, newColors, newNorms);
    this->VertexOffsets[fidx] =
      static_cast<unsigned int>(this->VBO->VertexCount);
    voffset = static_cast<unsigned int>(this->VBO->VertexCount);
    this->IndexOffsets[fidx] =
      static_cast<unsigned int>(this->IndexArray.size());
    this->EdgeIndexOffsets[fidx] =
      static_cast<unsigned int>(this->EdgeIndexArray.size());
    }

  this->VBO->Upload(this->VBO->PackedVBO, vtkOpenGLBufferObject::ArrayBuffer);
  this->VBO->PackedVBO.resize(0);
  this->Tris.IBO->Upload(this->IndexArray,
    vtkOpenGLBufferObject::ElementArrayBuffer);
  this->Tris.IBO->IndexCount = this->IndexArray.size();
  this->IndexArray.resize(0);
  this->TrisEdges.IBO->Upload(this->EdgeIndexArray,
    vtkOpenGLBufferObject::ElementArrayBuffer);
  this->TrisEdges.IBO->IndexCount = this->EdgeIndexArray.size();
  this->EdgeIndexArray.resize(0);

  // allocate as needed
  if (this->HaveCellScalars || this->HavePickScalars)
    {
    if (!this->CellScalarTexture)
      {
      this->CellScalarTexture = vtkTextureObject::New();
      this->CellScalarBuffer = vtkOpenGLBufferObject::New();
      }
    this->CellScalarTexture->SetContext(
      static_cast<vtkOpenGLRenderWindow*>(ren->GetVTKWindow()));
    this->CellScalarBuffer->Upload(newColors,
      vtkOpenGLBufferObject::TextureBuffer);
    this->CellScalarTexture->CreateTextureBuffer(
      static_cast<unsigned int>(newColors.size()/4),
      4,
      VTK_UNSIGNED_CHAR,
      this->CellScalarBuffer);
    }

  if (this->HaveCellNormals)
    {
    if (!this->CellNormalTexture)
      {
      this->CellNormalTexture = vtkTextureObject::New();
      this->CellNormalBuffer = vtkOpenGLBufferObject::New();
      }
    this->CellNormalTexture->SetContext(
      static_cast<vtkOpenGLRenderWindow*>(ren->GetVTKWindow()));
    this->CellNormalBuffer->Upload(newNorms,
      vtkOpenGLBufferObject::TextureBuffer);
    this->CellNormalTexture->CreateTextureBuffer(
      static_cast<unsigned int>(newNorms.size()/4),
      4, VTK_FLOAT,
      this->CellNormalBuffer);
    }
}

//-------------------------------------------------------------------------
void vtkCompositePolyDataMapper2::AppendOneBufferObject(
  vtkRenderer *ren,
  vtkActor *act,
  vtkPolyData *poly,
  unsigned int voffset,
  std::vector<unsigned char> &newColors,
  std::vector<float> &newNorms
  )
{
  // if there are no cells then skip this piece
  if (poly->GetPolys()->GetNumberOfCells() == 0)
    {
    return;
    }

  // Get rid of old texture color coordinates if any
  if ( this->ColorCoordinates )
    {
    this->ColorCoordinates->UnRegister(this);
    this->ColorCoordinates = 0;
    }
  // Get rid of old texture color coordinates if any
  if ( this->Colors )
    {
    this->Colors->UnRegister(this);
    this->Colors = 0;
    }

  // For vertex coloring, this sets this->Colors as side effect.
  // For texture map coloring, this sets ColorCoordinates
  // and ColorTextureMap as a side effect.
  // I moved this out of the conditional because it is fast.
  // Color arrays are cached. If nothing has changed,
  // then the scalars do not have to be regenerted.
  this->MapScalars(poly, 1.0);

  // If we are coloring by texture, then load the texture map.
  if (this->ColorTextureMap)
    {
    if (this->InternalColorTexture == 0)
      {
      this->InternalColorTexture = vtkOpenGLTexture::New();
      this->InternalColorTexture->RepeatOff();
      }
    this->InternalColorTexture->SetInputData(this->ColorTextureMap);
    }

  this->HaveCellScalars = false;
  vtkDataArray *c = this->Colors;
  if (this->ScalarVisibility)
    {
    // We must figure out how the scalars should be mapped to the polydata.
    if ( (this->ScalarMode == VTK_SCALAR_MODE_USE_CELL_DATA ||
          this->ScalarMode == VTK_SCALAR_MODE_USE_CELL_FIELD_DATA ||
          this->ScalarMode == VTK_SCALAR_MODE_USE_FIELD_DATA ||
          !poly->GetPointData()->GetScalars() )
         && this->ScalarMode != VTK_SCALAR_MODE_USE_POINT_FIELD_DATA
         && this->Colors)
      {
      this->HaveCellScalars = true;
      c = NULL;
      }
    }

  this->HaveCellNormals = false;
  // Do we have cell normals?
  vtkDataArray *n =
    (act->GetProperty()->GetInterpolation() != VTK_FLAT) ? poly->GetPointData()->GetNormals() : NULL;
  if (n == NULL && poly->GetCellData()->GetNormals())
    {
    this->HaveCellNormals = true;
    n = NULL;
    }

  // if we have cell scalars then we have to
  // explode the data
  vtkCellArray *prims[4];
  prims[0] =  poly->GetVerts();
  prims[1] =  poly->GetLines();
  prims[2] =  poly->GetPolys();
  prims[3] =  poly->GetStrips();

  int representation = act->GetProperty()->GetRepresentation();
  vtkHardwareSelector* selector = ren->GetSelector();
  if (selector && this->PopulateSelectionSettings &&
      selector->GetFieldAssociation() == vtkDataObject::FIELD_ASSOCIATION_POINTS &&
      selector->GetCurrentPass() >= vtkHardwareSelector::ID_LOW24)
    {
    representation = VTK_POINTS;
    }

  this->AppendCellTextures(ren, act, prims, representation,
    newColors, newNorms, poly);

  // do we have texture maps?
  bool haveTextures = (this->ColorTextureMap || act->GetTexture() || act->GetProperty()->GetNumberOfTextures());

  // Set the texture if we are going to use texture
  // for coloring with a point attribute.
  // fixme ... make the existence of the coordinate array the signal.
  vtkDataArray *tcoords = NULL;
  if (haveTextures)
    {
    if (this->InterpolateScalarsBeforeMapping && this->ColorCoordinates)
      {
      tcoords = this->ColorCoordinates;
      }
    else
      {
      tcoords = poly->GetPointData()->GetTCoords();
      }
    }

  // Build the VBO
  this->VBO->AppendVBO(poly->GetPoints(),
            poly->GetPoints()->GetNumberOfPoints(),
            n, tcoords,
            c ? (unsigned char *)c->GetVoidPointer(0) : NULL,
            c ? c->GetNumberOfComponents() : 0);

  // now create the IBOs
  if (representation == VTK_POINTS)
    {
    vtkOpenGLIndexBufferObject::AppendPointIndexBuffer(
      this->IndexArray, prims[2], voffset);
    }
  else // WIREFRAME OR SURFACE
    {
    if (representation == VTK_WIREFRAME)
      {
      vtkOpenGLIndexBufferObject::AppendTriangleLineIndexBuffer(
        this->IndexArray, prims[2], voffset);
      }
   else // SURFACE
      {
      vtkOpenGLIndexBufferObject::AppendTriangleIndexBuffer(this->IndexArray,
        prims[2],
        poly->GetPoints(),
        voffset);
      }
    }

  // when drawing edges also build the edge IBOs
  vtkProperty *prop = act->GetProperty();
  bool draw_surface_with_edges =
    (prop->GetEdgeVisibility() && prop->GetRepresentation() == VTK_SURFACE);
  if (draw_surface_with_edges)
    {
    vtkOpenGLIndexBufferObject::AppendTriangleLineIndexBuffer(
      this->EdgeIndexArray, prims[2], voffset);
    }

}

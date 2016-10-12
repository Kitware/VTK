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
#include "vtkCompositePolyDataMapper2.h"

#include "vtk_glew.h"

#include "vtkCellData.h"
#include "vtkCompositeDataDisplayAttributes.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkFloatArray.h"
#include "vtkHardwareSelector.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkLookupTable.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLIndexBufferObject.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLTexture.h"
#include "vtkOpenGLVertexBufferObject.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkScalarsToColors.h"
#include "vtkShaderProgram.h"
#include "vtkTextureObject.h"

#include <algorithm>
#include <sstream>

// this class encapsulates values tied to a
// polydata
class vtkCompositeMapperHelperData
{
public:
  vtkPolyData *Data;
  unsigned int FlatIndex;
  double Opacity;
  bool Visibility;
  bool OverridesColor;
  vtkColor3d AmbientColor;
  vtkColor3d DiffuseColor;

  bool Marked;

  unsigned int StartVertex;
  unsigned int NextVertex;

  // point line poly strip edge stripedge
  unsigned int StartIndex[6];
  unsigned int NextIndex[6];

  // Point Line Poly Strip end
  size_t PrimOffsets[5];

  bool Different(
    vtkCompositeMapperHelperData *next,
    vtkHardwareSelector *selector,
    int primType)
  {
    return
      (selector &&
        selector->GetCurrentPass() ==
            vtkHardwareSelector::COMPOSITE_INDEX_PASS) ||
      this->Opacity != next->Opacity ||
      this->Visibility != next->Visibility ||
      this->OverridesColor != next->OverridesColor ||
      this->AmbientColor != next->AmbientColor ||
      this->DiffuseColor != next->DiffuseColor ||
      (primType >= 0 && primType <= 3 &&
        this->PrimOffsets[primType+1] != next->PrimOffsets[primType]);
  }

};

//===================================================================
// We define a helper class that is a subclass of vtkOpenGLPolyDataMapper
class vtkCompositeMapperHelper2 : public vtkOpenGLPolyDataMapper
{
public:
  static vtkCompositeMapperHelper2* New();
  vtkTypeMacro(vtkCompositeMapperHelper2, vtkOpenGLPolyDataMapper);

  void SetParent(vtkCompositePolyDataMapper2 *p) {
    this->Parent = p; }

  vtkCompositeMapperHelperData *AddData(vtkPolyData *pd, unsigned int flatIndex);

  // Description:
  // Implemented by sub classes. Actual rendering is done here.
  virtual void RenderPiece(vtkRenderer *ren, vtkActor *act);

  // keep track of what data is being used as the multiblock
  // can change
  void ClearMark();
  void RemoveUnused();
  bool GetMarked() { return this->Marked; }
  void SetMarked(bool v) { this->Marked = v; }

protected:
  vtkCompositePolyDataMapper2 *Parent;
  std::map<vtkPolyData *, vtkCompositeMapperHelperData *> Data;

  bool Marked;

  vtkCompositeMapperHelper2()
  {
    this->Parent = 0;
  };
  ~vtkCompositeMapperHelper2();

  void DrawIBO(
    vtkRenderer* ren, vtkActor *actor,
    int primType,
    vtkOpenGLHelper &CellBO,
    GLenum mode,
    int pointSize);

  void SetShaderValues(
    vtkShaderProgram *prog,
    vtkCompositeMapperHelperData *hdata,
    size_t primOffset);

  // Description:
  // Perform string replacments on the shader templates, called from
  // ReplaceShaderValues
  void ReplaceShaderColor(
    std::map<vtkShader::Type, vtkShader *> shaders,
    vtkRenderer *ren, vtkActor *act) VTK_OVERRIDE;

  // Description:
  // Determine if the buffer objects need to be rebuilt
  virtual bool GetNeedToRebuildBufferObjects(vtkRenderer *ren, vtkActor *act);

  // Description:
  // Build the VBO/IBO, called by UpdateBufferObjects
  virtual void BuildBufferObjects(vtkRenderer *ren, vtkActor *act);
  virtual void AppendOneBufferObject(vtkRenderer *ren,
    vtkActor *act, vtkCompositeMapperHelperData *hdata,
    unsigned int flat_index,
    std::vector<unsigned char> &colors,
    std::vector<float> &norms);

  // Description:
  // Returns if we can use texture maps for scalar coloring. Note this doesn't
  // say we "will" use scalar coloring. It says, if we do use scalar coloring,
  // we will use a texture. Always off for this mapper.
  virtual int CanUseTextureMapForColoring(vtkDataObject*);

  std::vector<unsigned int> VertexOffsets;

  // vert line poly strip edge stripedge
  std::vector<unsigned int> IndexArray[6];

  virtual void RenderPieceDraw(vtkRenderer *ren, vtkActor *act);
  virtual void RenderEdges(vtkRenderer *ren, vtkActor *act);

  bool PrimIDUsed;
  bool OverideColorUsed;

  vtkHardwareSelector *CurrentSelector;
  double CurrentAmbientIntensity;
  double CurrentDiffuseIntensity;

private:
  vtkCompositeMapperHelper2(const vtkCompositeMapperHelper2&) VTK_DELETE_FUNCTION;
  void operator=(const vtkCompositeMapperHelper2&) VTK_DELETE_FUNCTION;
};

typedef std::map<vtkPolyData *, vtkCompositeMapperHelperData *>::iterator dataIter;
typedef std::map<const std::string, vtkCompositeMapperHelper2 *>::iterator helpIter;

vtkStandardNewMacro(vtkCompositeMapperHelper2);

vtkCompositeMapperHelper2::~vtkCompositeMapperHelper2()
{
  for (dataIter it = this->Data.begin(); it != this->Data.end(); it++)
  {
    delete it->second;
  }
  this->Data.clear();
}

void vtkCompositeMapperHelper2::SetShaderValues(
  vtkShaderProgram *prog,
  vtkCompositeMapperHelperData *hdata,
  size_t primOffset)
{
  if (this->PrimIDUsed)
  {
    prog->SetUniformi("PrimitiveIDOffset",
      static_cast<int>(primOffset));
  }

  if (this->CurrentSelector)
  {
    if (this->CurrentSelector->GetCurrentPass() ==
        vtkHardwareSelector::COMPOSITE_INDEX_PASS &&
        prog->IsUniformUsed("mapperIndex"))
    {
      this->CurrentSelector->RenderCompositeIndex(hdata->FlatIndex);
      prog->SetUniform3f("mapperIndex",
        this->CurrentSelector->GetPropColorValue());
    }
    return;
  }
  // override the opacity and color
  prog->SetUniformf("opacityUniform", hdata->Opacity);
  vtkColor3d &aColor = hdata->AmbientColor;
  float ambientColor[3] = {static_cast<float>(aColor[0] * this->CurrentAmbientIntensity),
    static_cast<float>(aColor[1] * this->CurrentAmbientIntensity),
    static_cast<float>(aColor[2] * this->CurrentAmbientIntensity)};
  vtkColor3d &dColor = hdata->DiffuseColor;
  float diffuseColor[3] = {static_cast<float>(dColor[0] * this->CurrentDiffuseIntensity),
    static_cast<float>(dColor[1] * this->CurrentDiffuseIntensity),
    static_cast<float>(dColor[2] * this->CurrentDiffuseIntensity)};
  prog->SetUniform3f("ambientColorUniform", ambientColor);
  prog->SetUniform3f("diffuseColorUniform", diffuseColor);
  if (this->OverideColorUsed)
  {
    prog->SetUniformi("OverridesColor", hdata->OverridesColor);
  }
}

void vtkCompositeMapperHelper2::ReplaceShaderColor(
  std::map<vtkShader::Type, vtkShader *> shaders,
  vtkRenderer *ren, vtkActor *actor)
{
  if (!this->CurrentSelector)
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
  }

  this->Superclass::ReplaceShaderColor(shaders,ren,actor);
}

void vtkCompositeMapperHelper2::ClearMark()
{
  for (dataIter it = this->Data.begin(); it != this->Data.end(); it++)
  {
    it->second->Marked = false;
  }
  this->Marked = false;
}

void vtkCompositeMapperHelper2::RemoveUnused()
{
  for (dataIter it = this->Data.begin(); it != this->Data.end(); )
  {
    if (!it->second->Marked)
    {
      delete it->second;
      this->Data.erase(it++);
      this->Modified();
    }
    else
    {
      ++it;
    }
  }
}

//-----------------------------------------------------------------------------
// Returns if we can use texture maps for scalar coloring. Note this doesn't say
// we "will" use scalar coloring. It says, if we do use scalar coloring, we will
// use a texture.
// When rendering multiblock datasets, if any 2 blocks provide different
// lookup tables for the scalars, then also we cannot use textures. This case can
// be handled if required.
int vtkCompositeMapperHelper2::CanUseTextureMapForColoring(vtkDataObject*)
{
  if (!this->InterpolateScalarsBeforeMapping)
  {
    return 0; // user doesn't want us to use texture maps at all.
  }

  int cellFlag=0;
  vtkScalarsToColors *scalarsLookupTable = 0;
  for (dataIter it = this->Data.begin(); it != this->Data.end(); it++)
  {
    vtkPolyData *pd = it->second->Data;
    vtkDataArray* scalars = vtkAbstractMapper::GetScalars(pd,
      this->ScalarMode, this->ArrayAccessMode, this->ArrayId,
      this->ArrayName, cellFlag);

    if (scalars)
    {
      if (cellFlag)
      {
        return 0;
      }
      if ((this->ColorMode == VTK_COLOR_MODE_DEFAULT &&
           vtkArrayDownCast<vtkUnsignedCharArray>(scalars)) ||
          this->ColorMode == VTK_COLOR_MODE_DIRECT_SCALARS)
      {
        // Don't use texture if direct coloring using RGB unsigned chars is
        // requested.
        return 0;
      }

      if (scalarsLookupTable && scalars->GetLookupTable() &&
          (scalarsLookupTable != scalars->GetLookupTable()))
      {
        // Two datasets are requesting different lookup tables to color with.
        // We don't handle this case right now for composite datasets.
        return 0;
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
      return 0;
  }

  return 1;
}

//-----------------------------------------------------------------------------
void vtkCompositeMapperHelper2::RenderPiece(vtkRenderer* ren, vtkActor *actor)
{
  // Make sure that we have been properly initialized.
  if (ren->GetRenderWindow()->CheckAbortStatus())
  {
    return;
  }

  this->CurrentInput = this->Data.begin()->first;

  this->RenderPieceStart(ren, actor);
  this->RenderPieceDraw(ren, actor);
  this->RenderEdges(ren,actor);
  this->RenderPieceFinish(ren, actor);
}

void vtkCompositeMapperHelper2::DrawIBO(
  vtkRenderer* ren, vtkActor *actor,
  int primType,
  vtkOpenGLHelper &CellBO,
  GLenum mode,
  int pointSize)
{
  if (CellBO.IBO->IndexCount)
  {
    bool haveCellTexture =
      (this->HaveCellScalars || this->HaveCellNormals || this->HavePickScalars);

    if (pointSize > 0)
    {
#if GL_ES_VERSION_2_0 != 1
      glPointSize(pointSize);
#endif
    }
    // First we do the triangles, update the shader, set uniforms, etc.
    this->UpdateShaders(CellBO, ren, actor);
    vtkShaderProgram *prog = CellBO.Program;
    this->PrimIDUsed = prog->IsUniformUsed("PrimitiveIDOffset");
    this->OverideColorUsed = prog->IsUniformUsed("OverridesColor");
    CellBO.IBO->Bind();

    if (!this->HaveWideLines(ren,actor) && mode == GL_LINES)
    {
      glLineWidth(actor->GetProperty()->GetLineWidth());
    }

    if (this->DrawingEdges && !this->DrawingTubes(CellBO, actor))
    {
      vtkProperty *ppty = actor->GetProperty();
      float diffuseColor[3] = {0.0, 0.0, 0.0};
      float ambientColor[3];
      double *acol = ppty->GetEdgeColor();
      ambientColor[0] = acol[0];
      ambientColor[1] = acol[1];
      ambientColor[2] = acol[2];
      prog->SetUniform3f("diffuseColorUniform", diffuseColor);
      prog->SetUniform3f("ambientColorUniform", ambientColor);
    }

    for (dataIter it = this->Data.begin(); it != this->Data.end(); )
    {
      vtkCompositeMapperHelperData *starthdata = it->second;
      vtkCompositeMapperHelperData *endhdata = starthdata;
      do
      {
        endhdata = it->second;
        it++;
      }
      while (it != this->Data.end() &&
        !(this->HaveAppleBug && haveCellTexture) &&
        !endhdata->Different(it->second, this->CurrentSelector,
          haveCellTexture ? primType : -1));
      if (endhdata->Visibility &&
          endhdata->NextIndex[primType] > starthdata->StartIndex[primType])
      {
        //compilers think this can exceed the bounds so we also
        // test against primType even though we should not need to
        if (!this->DrawingEdges && primType < 4)
        {
          this->SetShaderValues(prog, starthdata,
            starthdata->PrimOffsets[primType]);
        }
        glDrawRangeElements(mode,
          static_cast<GLuint>(starthdata->StartVertex),
          static_cast<GLuint>(endhdata->NextVertex > 0 ? endhdata->NextVertex - 1 : 0),
          static_cast<GLsizei>(endhdata->NextIndex[primType] - starthdata->StartIndex[primType]),
          GL_UNSIGNED_INT,
          reinterpret_cast<const GLvoid *>(starthdata->StartIndex[primType]*sizeof(GLuint)));
      }
    }
    CellBO.IBO->Release();
  }
}

//-----------------------------------------------------------------------------
void vtkCompositeMapperHelper2::RenderPieceDraw(
  vtkRenderer* ren, vtkActor *actor)
{
  int representation = actor->GetProperty()->GetRepresentation();

  // render points for point picking in a special way
  // all cell types should be rendered as points
  this->CurrentSelector = ren->GetSelector();
  bool pointPicking = false;
  if (this->CurrentSelector && this->PopulateSelectionSettings &&
      this->CurrentSelector->GetFieldAssociation() == vtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    representation = VTK_POINTS;
    pointPicking = true;
  }

  vtkProperty *ppty = actor->GetProperty();
  this->CurrentAmbientIntensity = ppty->GetAmbient();
  this->CurrentDiffuseIntensity = ppty->GetDiffuse();

  this->PrimitiveIDOffset = 0;

  GLenum mode = (representation == VTK_POINTS) ? GL_POINTS :
    (representation == VTK_WIREFRAME) ? GL_LINES : GL_TRIANGLES;

  // draw IBOs
  this->DrawIBO(ren, actor, 0, this->Points, GL_POINTS, pointPicking ? 2 : 0);
  this->DrawIBO(ren, actor, 1, this->Lines,
   representation == VTK_POINTS ? GL_POINTS : GL_LINES, pointPicking ? 4 : 0);
  this->DrawIBO(ren, actor, 2, this->Tris, mode, pointPicking ? 6 : 0);
  this->DrawIBO(ren, actor, 3, this->TriStrips, mode, pointPicking ? 6 : 0);

  if (this->CurrentSelector && (
        this->CurrentSelector->GetCurrentPass() == vtkHardwareSelector::ID_LOW24 ||
        this->CurrentSelector->GetCurrentPass() == vtkHardwareSelector::ID_MID24 ||
        this->CurrentSelector->GetCurrentPass() == vtkHardwareSelector::ID_HIGH16))
  {
    this->CurrentSelector->RenderAttributeId(this->PrimitiveIDOffset);
  }

}

//-----------------------------------------------------------------------------
void vtkCompositeMapperHelper2::RenderEdges(
  vtkRenderer* ren, vtkActor *actor)
{
  vtkProperty *prop = actor->GetProperty();
  bool draw_surface_with_edges =
    (prop->GetEdgeVisibility() && prop->GetRepresentation() == VTK_SURFACE);

  if (!draw_surface_with_edges || this->CurrentSelector)
  {
    return;
  }

  this->DrawingEdges = true;

  this->DrawIBO(ren, actor, 4, this->TrisEdges, GL_LINES, 0);
  this->DrawIBO(ren, actor, 5, this->TriStripsEdges, GL_LINES, 0);

  this->DrawingEdges = false;
}

vtkCompositeMapperHelperData *vtkCompositeMapperHelper2::AddData(
  vtkPolyData *pd, unsigned int flatIndex)
{
  dataIter found = this->Data.find(pd);
  if (found == this->Data.end())
  {
    vtkCompositeMapperHelperData *hdata =
      new vtkCompositeMapperHelperData();
    hdata->FlatIndex = flatIndex;
    hdata->Data = pd;
    hdata->Marked = true;
    this->Data.insert(std::make_pair(pd, hdata));
    this->Modified();
    return hdata;
  }
  found->second->Marked = true;
  return found->second;
}

//-------------------------------------------------------------------------
bool vtkCompositeMapperHelper2::GetNeedToRebuildBufferObjects(
  vtkRenderer *ren, vtkActor *act)
{
  // might need to loop of Data and check each one?  Or is that handled
  // in parent Render
  if (vtkCompositeMapperHelper2::Superclass::GetNeedToRebuildBufferObjects(ren, act) ||
      (this->CurrentInput && this->VBOBuildTime < this->CurrentInput->GetMTime()))
  {
    return true;
  }
  return false;
}

//-------------------------------------------------------------------------
void vtkCompositeMapperHelper2::BuildBufferObjects(
  vtkRenderer *ren,
  vtkActor *act)
{
  // render using the composite data attributes
  this->VBO->VertexCount = 0;

  // create the cell scalar array adjusted for ogl Cells
  std::vector<unsigned char> newColors;
  std::vector<float> newNorms;

  // check if this system is subject to the apple/amd primID bug
  this->HaveAppleBug =
    static_cast<vtkOpenGLRenderer *>(ren)->HaveApplePrimitiveIdBug();
  if (this->HaveAppleBugForce == 1)
  {
    this->HaveAppleBug = false;
  }
  if (this->HaveAppleBugForce == 2)
  {
    this->HaveAppleBug = true;
  }
  this->AppleBugPrimIDs.resize(0);

  dataIter iter;
  unsigned int voffset = 0;
  for (iter = this->Data.begin(); iter != this->Data.end(); iter++)
  {
    vtkCompositeMapperHelperData *hdata = iter->second;
    hdata->StartVertex =
      static_cast<unsigned int>(this->VBO->VertexCount);
    for (int i = 0; i < 6; i++)
    {
      hdata->StartIndex[i] =
        static_cast<unsigned int>(this->IndexArray[i].size());
    }
    this->AppendOneBufferObject(ren, act, hdata,
      voffset, newColors, newNorms);
    hdata->NextVertex =
      static_cast<unsigned int>(this->VBO->VertexCount);
    voffset = static_cast<unsigned int>(this->VBO->VertexCount);
    for (int i = 0; i < 6; i++)
    {
      hdata->NextIndex[i] =
        static_cast<unsigned int>(this->IndexArray[i].size());
    }
  }

  this->VBO->Upload(this->VBO->PackedVBO, vtkOpenGLBufferObject::ArrayBuffer);
  this->VBO->PackedVBO.resize(0);

  this->Points.IBO->IndexCount = this->IndexArray[0].size();
  if (this->Points.IBO->IndexCount)
  {
    this->Points.IBO->Upload(this->IndexArray[0],
      vtkOpenGLBufferObject::ElementArrayBuffer);
    this->IndexArray[0].resize(0);
  }
  this->Lines.IBO->IndexCount = this->IndexArray[1].size();
  if (this->Lines.IBO->IndexCount)
  {
    this->Lines.IBO->Upload(this->IndexArray[1],
      vtkOpenGLBufferObject::ElementArrayBuffer);
    this->IndexArray[1].resize(0);
  }
  this->Tris.IBO->IndexCount = this->IndexArray[2].size();
  if (this->Tris.IBO->IndexCount)
  {
    this->Tris.IBO->Upload(this->IndexArray[2],
      vtkOpenGLBufferObject::ElementArrayBuffer);
    this->IndexArray[2].resize(0);
  }
  this->TriStrips.IBO->IndexCount = this->IndexArray[3].size();
  if (this->TriStrips.IBO->IndexCount)
  {
    this->TriStrips.IBO->Upload(this->IndexArray[3],
      vtkOpenGLBufferObject::ElementArrayBuffer);
    this->IndexArray[3].resize(0);
  }
  this->TrisEdges.IBO->IndexCount = this->IndexArray[4].size();
  if (this->TrisEdges.IBO->IndexCount)
  {
    this->TrisEdges.IBO->Upload(this->IndexArray[4],
      vtkOpenGLBufferObject::ElementArrayBuffer);
    this->IndexArray[4].resize(0);
  }
  this->TriStripsEdges.IBO->IndexCount = this->IndexArray[5].size();
  if (this->TriStripsEdges.IBO->IndexCount)
  {
    this->TriStripsEdges.IBO->Upload(this->IndexArray[5],
      vtkOpenGLBufferObject::ElementArrayBuffer);
    this->IndexArray[5].resize(0);
  }

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
      this->CellNormalBuffer->SetType(vtkOpenGLBufferObject::TextureBuffer);
    }
    this->CellNormalTexture->SetContext(
      static_cast<vtkOpenGLRenderWindow*>(ren->GetVTKWindow()));

    // do we have float texture support ?
    int ftex =
      static_cast<vtkOpenGLRenderWindow *>(ren->GetRenderWindow())->
        GetDefaultTextureInternalFormat(VTK_FLOAT, 4, false, true);

    if (ftex)
    {
      this->CellNormalBuffer->Upload(newNorms,
        vtkOpenGLBufferObject::TextureBuffer);
      this->CellNormalTexture->CreateTextureBuffer(
        static_cast<unsigned int>(newNorms.size()/4),
        4, VTK_FLOAT,
        this->CellNormalBuffer);
    }
    else
    {
      // have to convert to unsigned char if no float support
      std::vector<unsigned char> ucNewNorms;
      ucNewNorms.resize(newNorms.size());
      for (size_t i = 0; i < newNorms.size(); i++)
      {
        ucNewNorms[i] = 127.0*(newNorms[i] + 1.0);
      }
      this->CellNormalBuffer->Upload(ucNewNorms,
        vtkOpenGLBufferObject::TextureBuffer);
      this->CellNormalTexture->CreateTextureBuffer(
        static_cast<unsigned int>(newNorms.size()/4),
        4, VTK_UNSIGNED_CHAR,
        this->CellNormalBuffer);
    }
  }

  vtkHardwareSelector* selector = ren->GetSelector();
  bool pointPicking = false;
  if (selector && this->PopulateSelectionSettings &&
      selector->GetFieldAssociation() == vtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    pointPicking = true;
  }
  if (this->HaveAppleBug &&
      !pointPicking &&
      (this->HaveCellNormals || this->HaveCellScalars || this->HavePickScalars))
  {
    if (!this->AppleBugPrimIDBuffer)
    {
      this->AppleBugPrimIDBuffer = vtkOpenGLBufferObject::New();
    }
    this->AppleBugPrimIDBuffer->Bind();
    this->AppleBugPrimIDBuffer->Upload(
     this->AppleBugPrimIDs, vtkOpenGLBufferObject::ArrayBuffer);
    this->AppleBugPrimIDBuffer->Release();
  }

  this->VBOBuildTime.Modified();
}

//-------------------------------------------------------------------------
void vtkCompositeMapperHelper2::AppendOneBufferObject(
  vtkRenderer *ren,
  vtkActor *act,
  vtkCompositeMapperHelperData *hdata,
  unsigned int voffset,
  std::vector<unsigned char> &newColors,
  std::vector<float> &newNorms
  )
{
  vtkPolyData *poly = hdata->Data;

  // if there are no points then skip this piece
  if (!poly->GetPoints() || poly->GetPoints()->GetNumberOfPoints() == 0)
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

  int representation = act->GetProperty()->GetRepresentation();
  vtkHardwareSelector* selector = ren->GetSelector();

  bool pointPicking = false;
  if (selector && this->PopulateSelectionSettings &&
      selector->GetFieldAssociation() == vtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    representation = VTK_POINTS;
    pointPicking = true;
  }

  // if we have cell scalars then we have to
  // explode the data
  vtkCellArray *prims[4];
  prims[0] =  poly->GetVerts();
  prims[1] =  poly->GetLines();
  prims[2] =  poly->GetPolys();
  prims[3] =  poly->GetStrips();

  // vert cell offset starts at the end of the last block
  hdata->PrimOffsets[0] = (newColors.size() ? newColors.size()/4 : newNorms.size()/4);
  hdata->PrimOffsets[1] = hdata->PrimOffsets[0] +
    prims[0]->GetNumberOfConnectivityEntries() -
    prims[0]->GetNumberOfCells();
  hdata->PrimOffsets[2] = hdata->PrimOffsets[1] +
    prims[1]->GetNumberOfConnectivityEntries() -
    2*prims[1]->GetNumberOfCells();

  this->AppendCellTextures(ren, act, prims, representation,
    newColors, newNorms, poly);

  hdata->PrimOffsets[4] = (newColors.size() ? newColors.size()/4 : newNorms.size()/4);

  // we back compute the strip number
  size_t triCount = prims[3]->GetNumberOfConnectivityEntries()
    - 3*prims[3]->GetNumberOfCells();
  hdata->PrimOffsets[3] = hdata->PrimOffsets[4] - triCount;

  // on apple with the AMD PrimID bug we use a slow
  // painful approach to work around it
  if (this->HaveAppleBug &&
      !pointPicking &&
      (this->HaveCellNormals || this->HaveCellScalars || this->HavePickScalars))
  {
    poly = this->HandleAppleBug(poly, this->AppleBugPrimIDs);
    prims[0] =  poly->GetVerts();
    prims[1] =  poly->GetLines();
    prims[2] =  poly->GetPolys();
    prims[3] =  poly->GetStrips();

#ifndef NDEBUG
    static bool warnedAboutBrokenAppleDriver = false;
    if (!warnedAboutBrokenAppleDriver)
    {
      vtkWarningMacro("VTK is working around a bug in Apple-AMD hardware related to gl_PrimitiveID.  This may cause significant memory and performance impacts. Your hardware has been identified as vendor "
        << (const char *)glGetString(GL_VENDOR) << " with renderer of "
        << (const char *)glGetString(GL_RENDERER) << " and version "
        << (const char *)glGetString(GL_VERSION));
      warnedAboutBrokenAppleDriver = true;
    }
#endif
    if (n)
    {
      n = (act->GetProperty()->GetInterpolation() != VTK_FLAT) ?
            poly->GetPointData()->GetNormals() : NULL;
    }
    if (c)
    {
      this->Colors->Delete();
      this->Colors = 0;
      this->MapScalars(poly,1.0);
      c = this->Colors;
    }
  }


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
  vtkOpenGLIndexBufferObject::AppendPointIndexBuffer(
    this->IndexArray[0], prims[0], voffset);

  vtkDataArray *ef = poly->GetPointData()->GetAttribute(
                    vtkDataSetAttributes::EDGEFLAG);

  if (representation == VTK_POINTS)
  {
    vtkOpenGLIndexBufferObject::AppendPointIndexBuffer(
      this->IndexArray[1], prims[1], voffset);

    vtkOpenGLIndexBufferObject::AppendPointIndexBuffer(
      this->IndexArray[2], prims[2], voffset);

    vtkOpenGLIndexBufferObject::AppendPointIndexBuffer(
      this->IndexArray[3], prims[3], voffset);
  }
  else // WIREFRAME OR SURFACE
  {
    vtkOpenGLIndexBufferObject::AppendLineIndexBuffer(
      this->IndexArray[1], prims[1], voffset);

    if (representation == VTK_WIREFRAME)
    {
      if (ef)
      {
        if (ef->GetNumberOfComponents() != 1)
        {
          vtkDebugMacro(<< "Currently only 1d edge flags are supported.");
          ef = NULL;
        }
        if (!ef->IsA("vtkUnsignedCharArray"))
        {
          vtkDebugMacro(<< "Currently only unsigned char edge flags are suported.");
          ef = NULL;
        }
      }
      if (ef)
      {
        vtkOpenGLIndexBufferObject::AppendEdgeFlagIndexBuffer(
          this->IndexArray[2], prims[2], voffset, ef);
      }
      else
      {
        vtkOpenGLIndexBufferObject::AppendTriangleLineIndexBuffer(
          this->IndexArray[2], prims[2], voffset);
      }
      vtkOpenGLIndexBufferObject::AppendStripIndexBuffer(
        this->IndexArray[3], prims[3], voffset, true);
    }
   else // SURFACE
   {
      vtkOpenGLIndexBufferObject::AppendTriangleIndexBuffer(
        this->IndexArray[2],
        prims[2],
        poly->GetPoints(),
        voffset);
      vtkOpenGLIndexBufferObject::AppendStripIndexBuffer(
        this->IndexArray[3], prims[3], voffset, false);
   }
  }

  // when drawing edges also build the edge IBOs
  vtkProperty *prop = act->GetProperty();
  bool draw_surface_with_edges =
    (prop->GetEdgeVisibility() && prop->GetRepresentation() == VTK_SURFACE);
  if (draw_surface_with_edges)
  {
    if (ef)
    {
      if (ef->GetNumberOfComponents() != 1)
      {
        vtkDebugMacro(<< "Currently only 1d edge flags are supported.");
        ef = NULL;
      }
      if (!ef->IsA("vtkUnsignedCharArray"))
      {
        vtkDebugMacro(<< "Currently only unsigned char edge flags are suported.");
        ef = NULL;
      }
    }
    if (ef)
    {
      vtkOpenGLIndexBufferObject::AppendEdgeFlagIndexBuffer(
        this->IndexArray[4], prims[2], voffset, ef);
    }
    else
    {
      vtkOpenGLIndexBufferObject::AppendTriangleLineIndexBuffer(
        this->IndexArray[4], prims[2], voffset);
    }
    vtkOpenGLIndexBufferObject::AppendStripIndexBuffer(
      this->IndexArray[5], prims[3], voffset, false);
  }

  // free up polydata if allocated due to apple bug
  if (poly != hdata->Data)
  {
    poly->Delete();
  }
}


//===================================================================
// Now the main class methods

vtkStandardNewMacro(vtkCompositePolyDataMapper2);
//----------------------------------------------------------------------------
vtkCompositePolyDataMapper2::vtkCompositePolyDataMapper2()
{
  this->LastOpaqueCheckTime = 0;
  this->CurrentFlatIndex = 0;
  this->LastOpaqueCheckValue = true;
}

//----------------------------------------------------------------------------
vtkCompositePolyDataMapper2::~vtkCompositePolyDataMapper2()
{
  helpIter miter = this->Helpers.begin();
  for (;miter != this->Helpers.end(); miter++)
  {
    miter->second->Delete();
  }
  this->Helpers.clear();
}

//----------------------------------------------------------------------------
int vtkCompositePolyDataMapper2::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  return 1;
}

//----------------------------------------------------------------------------
vtkExecutive* vtkCompositePolyDataMapper2::CreateDefaultExecutive()
{
  return vtkCompositeDataPipeline::New();
}

//-----------------------------------------------------------------------------
//Looks at each DataSet and finds the union of all the bounds
void vtkCompositePolyDataMapper2::ComputeBounds()
{
  vtkCompositeDataSet *input = vtkCompositeDataSet::SafeDownCast(
    this->GetInputDataObject(0, 0));

  // If we don't have hierarchical data, test to see if we have
  // plain old polydata. In this case, the bounds are simply
  // the bounds of the input polydata.
  if (!input)
  {
    this->Superclass::ComputeBounds();
    return;
  }

  if (input->GetMTime() < this->BoundsMTime &&
      this->GetMTime() < this->BoundsMTime)
  {
    return;
  }

  // computing bounds with only visible blocks
  vtkCompositeDataDisplayAttributes::ComputeVisibleBounds(
    this->CompositeAttributes, input, this->Bounds);
  this->BoundsMTime.Modified();
}

//-----------------------------------------------------------------------------
bool vtkCompositePolyDataMapper2::GetIsOpaque()
{
  vtkCompositeDataSet *input = vtkCompositeDataSet::SafeDownCast(
    this->GetInputDataObject(0, 0));
  vtkMTimeType lastMTime = std::max(input ? input->GetMTime() : 0, this->GetMTime());
  if (lastMTime <= this->LastOpaqueCheckTime)
  {
    return this->LastOpaqueCheckValue;
  }
  this->LastOpaqueCheckTime = lastMTime;
  if (this->ScalarVisibility && input &&
      (this->ColorMode == VTK_COLOR_MODE_DEFAULT ||
       this->ColorMode == VTK_COLOR_MODE_DIRECT_SCALARS))
  {
    vtkSmartPointer<vtkCompositeDataIterator> iter;
    iter.TakeReference(input->NewIterator());
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      vtkPolyData *pd = vtkPolyData::SafeDownCast(iter->GetCurrentDataObject());
      if (pd)
      {
        int cellFlag;
        vtkDataArray* scalars = this->GetScalars(pd,
          this->ScalarMode, this->ArrayAccessMode, this->ArrayId,
          this->ArrayName, cellFlag);
        if (scalars &&
            (scalars->IsA("vtkUnsignedCharArray")  ||
             this->ColorMode == VTK_COLOR_MODE_DIRECT_SCALARS) &&
            (scalars->GetNumberOfComponents() ==  4 /*(RGBA)*/ ||
             scalars->GetNumberOfComponents() == 2 /*(LuminanceAlpha)*/))
        {
          int opacityIndex = scalars->GetNumberOfComponents() - 1;
          unsigned char opacity = 0;
          switch (scalars->GetDataType())
          {
            vtkTemplateMacro(
              vtkScalarsToColors::ColorToUChar(
                static_cast<VTK_TT>(scalars->GetRange(opacityIndex)[0]),
                &opacity));
          }
          if (opacity < 255)
          {
            // If the opacity is 255, despite the fact that the user specified
            // RGBA, we know that the Alpha is 100% opaque. So treat as opaque.
            this->LastOpaqueCheckValue = false;
            return false;
          }
        }
      }
    }
  }
  else if(this->CompositeAttributes &&
    this->CompositeAttributes->HasBlockOpacities())
  {
    this->LastOpaqueCheckValue = false;
    return false;
  }

  this->LastOpaqueCheckValue = this->Superclass::GetIsOpaque();
  return this->LastOpaqueCheckValue;
}

//----------------------------------------------------------------------------
void vtkCompositePolyDataMapper2::SetBlockVisibility(unsigned int index, bool visible)
{
  if(this->CompositeAttributes)
  {
    this->CompositeAttributes->SetBlockVisibility(index, visible);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
bool vtkCompositePolyDataMapper2::GetBlockVisibility(unsigned int index) const
{
  if(this->CompositeAttributes)
  {
    return this->CompositeAttributes->GetBlockVisibility(index);
  }
  else
  {
    return true;
  }
}

//----------------------------------------------------------------------------
void vtkCompositePolyDataMapper2::RemoveBlockVisibility(unsigned int index)
{
  if(this->CompositeAttributes)
  {
    this->CompositeAttributes->RemoveBlockVisibility(index);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkCompositePolyDataMapper2::RemoveBlockVisibilites()
{
  if(this->CompositeAttributes)
  {
    this->CompositeAttributes->RemoveBlockVisibilites();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkCompositePolyDataMapper2::SetBlockColor(unsigned int index, double color[3])
{
  if(this->CompositeAttributes)
  {
    this->CompositeAttributes->SetBlockColor(index, color);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
double* vtkCompositePolyDataMapper2::GetBlockColor(unsigned int index)
{
  static double white[3] = {1.0,1.0,1.0};

  if(this->CompositeAttributes)
  {
    this->CompositeAttributes->GetBlockColor(index, this->ColorResult);
    return this->ColorResult;
  }
  else
  {
    return white;
  }
}

//----------------------------------------------------------------------------
void vtkCompositePolyDataMapper2::RemoveBlockColor(unsigned int index)
{
  if(this->CompositeAttributes)
  {
    this->CompositeAttributes->RemoveBlockColor(index);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkCompositePolyDataMapper2::RemoveBlockColors()
{
  if(this->CompositeAttributes)
  {
    this->CompositeAttributes->RemoveBlockColors();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkCompositePolyDataMapper2::SetBlockOpacity(unsigned int index, double opacity)
{
  if(this->CompositeAttributes)
  {
    this->CompositeAttributes->SetBlockOpacity(index, opacity);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
double vtkCompositePolyDataMapper2::GetBlockOpacity(unsigned int index)
{
  if(this->CompositeAttributes)
  {
    return this->CompositeAttributes->GetBlockOpacity(index);
  }
  return 1.;
}

//----------------------------------------------------------------------------
void vtkCompositePolyDataMapper2::RemoveBlockOpacity(unsigned int index)
{
  if(this->CompositeAttributes)
  {
    this->CompositeAttributes->RemoveBlockOpacity(index);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkCompositePolyDataMapper2::RemoveBlockOpacities()
{
  if(this->CompositeAttributes)
  {
    this->CompositeAttributes->RemoveBlockOpacities();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkCompositePolyDataMapper2::SetCompositeDataDisplayAttributes(
  vtkCompositeDataDisplayAttributes *attributes)
{
  if(this->CompositeAttributes != attributes)
  {
    this->CompositeAttributes = attributes;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
vtkCompositeDataDisplayAttributes*
vtkCompositePolyDataMapper2::GetCompositeDataDisplayAttributes()
{
  return this->CompositeAttributes;
}

//----------------------------------------------------------------------------
void vtkCompositePolyDataMapper2::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

void vtkCompositePolyDataMapper2::CopyMapperValuesToHelper(vtkCompositeMapperHelper2 *helper)
{
  // We avoid PolyDataMapper::ShallowCopy because it copies the input
  helper->vtkMapper::ShallowCopy(this);
  helper->SetPointIdArrayName(this->GetPointIdArrayName());
  helper->SetCompositeIdArrayName(this->GetCompositeIdArrayName());
  helper->SetProcessIdArrayName(this->GetProcessIdArrayName());
  helper->SetCellIdArrayName(this->GetCellIdArrayName());
  helper->SetVertexShaderCode(this->GetVertexShaderCode());
  helper->SetGeometryShaderCode(this->GetGeometryShaderCode());
  helper->SetFragmentShaderCode(this->GetFragmentShaderCode());
  helper->SetStatic(1);
}

//-----------------------------------------------------------------------------
void vtkCompositePolyDataMapper2::ReleaseGraphicsResources(vtkWindow* win)
{
  helpIter miter = this->Helpers.begin();
  for (;miter != this->Helpers.end(); miter++)
  {
    miter->second->ReleaseGraphicsResources(win);
  }
  miter = this->Helpers.begin();
  for (;miter != this->Helpers.end(); miter++)
  {
    miter->second->Delete();
  }
  this->Helpers.clear();
  this->Modified();
  this->Superclass::ReleaseGraphicsResources(win);
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

  // the first step is to gather up the polydata based on their
  // signatures (aka have normals, have scalars etc)
  if (this->HelperMTime < this->GetInputDataObject(0, 0)->GetMTime() ||
      this->HelperMTime < this->GetMTime())
  {
    // clear old helpers
    for (helpIter hiter = this->Helpers.begin(); hiter != this->Helpers.end(); hiter++)
    {
      hiter->second->ClearMark();
    }
    this->HelperDataMap.clear();

    vtkSmartPointer<vtkDataObjectTreeIterator> iter =
      vtkSmartPointer<vtkDataObjectTreeIterator>::New();
    iter->SetDataSet(input);
    iter->SkipEmptyNodesOn();
    iter->VisitOnlyLeavesOn();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal();
        iter->GoToNextItem())
    {
      unsigned int flatIndex = iter->GetCurrentFlatIndex();
      vtkDataObject *dso = iter->GetCurrentDataObject();
      vtkPolyData *pd = vtkPolyData::SafeDownCast(dso);

      if (!pd || !pd->GetPoints())
      {
        continue;
      }
      int cellFlag = 0;
      bool hasScalars = this->ScalarVisibility &&
        (vtkAbstractMapper::GetAbstractScalars(
          pd, this->ScalarMode, this->ArrayAccessMode,
          this->ArrayId, this->ArrayName,
          cellFlag) != NULL);

      bool hasNormals =
        (pd->GetPointData()->GetNormals() || pd->GetCellData()->GetNormals());

      bool hasTCoords = (pd->GetPointData()->GetTCoords() != NULL);

      std::ostringstream toString;
      toString.str("");
      toString.clear();
      toString <<
        'A' << (hasScalars ? 1 : 0) <<
        'B' << (hasNormals ? 1 : 0) <<
        'C' << (hasTCoords ? 1 : 0);

      vtkCompositeMapperHelper2 *helper = NULL;
      helpIter found = this->Helpers.find(toString.str());
      if (found == this->Helpers.end())
      {
        helper = vtkCompositeMapperHelper2::New();
        helper->SetParent(this);
        this->Helpers.insert(std::make_pair(toString.str(), helper));
      }
      else
      {
        helper = found->second;
      }
      this->CopyMapperValuesToHelper(helper);
      helper->SetMarked(true);
      this->HelperDataMap[pd] =
        helper->AddData(pd, flatIndex);
    }
    // delete unused old helpers/data
    for (helpIter hiter = this->Helpers.begin(); hiter != this->Helpers.end(); )
    {
      hiter->second->RemoveUnused();
      if (!hiter->second->GetMarked())
      {
        hiter->second->ReleaseGraphicsResources(ren->GetVTKWindow());
        hiter->second->Delete();
        this->Helpers.erase(hiter++);
      }
      else
      {
        ++hiter;
      }
    }
    this->HelperMTime.Modified();
  }


  // rebuild the render values if needed
  if (this->RenderValuesBuildTime < this->GetMTime() ||
      this->RenderValuesBuildTime < actor->GetProperty()->GetMTime() ||
      this->RenderValuesBuildTime < this->VBOBuildTime ||
      this->RenderValuesBuildTime < this->HelperMTime ||
      this->RenderValuesBuildTime < this->SelectionStateChanged)
  {
    vtkProperty* prop = actor->GetProperty();

    // Push base-values on the state stack.
    this->BlockState.Visibility.push(true);
    this->BlockState.Opacity.push(prop->GetOpacity());
    this->BlockState.AmbientColor.push(vtkColor3d(prop->GetAmbientColor()));
    this->BlockState.DiffuseColor.push(vtkColor3d(prop->GetDiffuseColor()));
    this->BlockState.SpecularColor.push(vtkColor3d(prop->GetSpecularColor()));

    vtkCompositeDataSet *cds = vtkCompositeDataSet::SafeDownCast(
      this->GetInputDataObject(0, 0));
    unsigned int flat_index = 0;
    this->BuildRenderValues(ren, actor, cds, flat_index);

    this->BlockState.Visibility.pop();
    this->BlockState.Opacity.pop();
    this->BlockState.AmbientColor.pop();
    this->BlockState.DiffuseColor.pop();
    this->BlockState.SpecularColor.pop();

    this->RenderValuesBuildTime.Modified();
  }


  for (helpIter hiter = this->Helpers.begin(); hiter != this->Helpers.end(); hiter++)
  {
    vtkCompositeMapperHelper2 *helper = hiter->second;
    helper->RenderPiece(ren,actor);
  }
}

//-----------------------------------------------------------------------------
void vtkCompositePolyDataMapper2::BuildRenderValues(
  vtkRenderer *renderer,
  vtkActor *actor,
  vtkDataObject *dobj,
  unsigned int &flat_index)
{
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
      this->BuildRenderValues(renderer, actor, child, flat_index);
    }
  }
  else
  {
    vtkPolyData *pd = vtkPolyData::SafeDownCast(dobj);
    dataIter dit = this->HelperDataMap.find(pd);
    if (dit != this->HelperDataMap.end())
    {
      vtkCompositeMapperHelperData *helperData = dit->second;
      helperData->Opacity = this->BlockState.Opacity.top();
      helperData->Visibility = this->BlockState.Visibility.top();
      helperData->AmbientColor = this->BlockState.AmbientColor.top();
      helperData->DiffuseColor = this->BlockState.DiffuseColor.top();
      helperData->OverridesColor = (this->BlockState.AmbientColor.size() > 1);
    }
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

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGL2Glyph3DMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenGL2Glyph3DMapper.h"

#include "vtkActor.h"
#include "vtkBitArray.h"
#include "vtkBoundingBox.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkHardwareSelector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkVBOPolyDataMapper.h"
#include "vtkPointData.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTimerLog.h"
#include "vtkTransform.h"
#include "vtkNew.h"
#include "vtkOpenGLError.h"

#include <cassert>
#include <vector>
#include <map>


template <class T>
static T vtkClamp(T val, T min, T max)
{
  val = val < min? min : val;
  val = val > max? max : val;
  return val;
}

class vtkOpenGL2Glyph3DMapper::vtkColorMapper : public vtkMapper
{
public:
  vtkTypeMacro(vtkColorMapper, vtkMapper);
  static vtkColorMapper* New() { return new vtkColorMapper; }
  void Render(vtkRenderer *, vtkActor *) {}
  vtkUnsignedCharArray* GetColors() { return this->Colors; }
};

class vtkOpenGL2Glyph3DMapper::vtkOpenGL2Glyph3DMapperEntry
{
public:
  std::vector<unsigned char> Colors;
  std::vector<vtkMatrix4x4 * > Matrices;
};

class vtkOpenGL2Glyph3DMapper::vtkOpenGL2Glyph3DMapperArray
{
public:
  std::map<const vtkDataSet *, vtkOpenGL2Glyph3DMapper::vtkOpenGL2Glyph3DMapperEntry *>  Entries;
};

vtkStandardNewMacro(vtkOpenGL2Glyph3DMapper)

// ---------------------------------------------------------------------------
// Construct object with scaling on, scaling mode is by scalar value,
// scale factor = 1.0, the range is (0,1), orient geometry is on, and
// orientation is by vector. Clamping and indexing are turned off. No
// initial sources are defined.
vtkOpenGL2Glyph3DMapper::vtkOpenGL2Glyph3DMapper()
{
  this->GlyphValues = new vtkOpenGL2Glyph3DMapper::vtkOpenGL2Glyph3DMapperArray();
  this->Mapper = vtkVBOPolyDataMapper::New();
  this->LastWindow = 0;
}

// ---------------------------------------------------------------------------
vtkOpenGL2Glyph3DMapper::~vtkOpenGL2Glyph3DMapper()
{
  if (this->GlyphValues)
    {
    delete this->GlyphValues;
    this->GlyphValues = NULL;
    }

  if (this->LastWindow)
    {
    this->ReleaseGraphicsResources(this->LastWindow);
    this->LastWindow = 0;
    }
  this->Mapper->UnRegister(this);
}

// ---------------------------------------------------------------------------
// Description:
// Send mapper ivars to sub-mapper.
// \pre mapper_exists: mapper!=0
void vtkOpenGL2Glyph3DMapper::CopyInformationToSubMapper(
    vtkVBOPolyDataMapper *mapper)
{
  assert("pre: mapper_exists" && mapper!=0);
  mapper->SetStatic(this->Static);
  mapper->ScalarVisibilityOff();
  // not used
  mapper->SetClippingPlanes(this->ClippingPlanes);

  mapper->SetResolveCoincidentTopology(this->GetResolveCoincidentTopology());
  mapper->SetResolveCoincidentTopologyZShift(
    this->GetResolveCoincidentTopologyZShift());

  // ResolveCoincidentTopologyPolygonOffsetParameters is static
  mapper->SetResolveCoincidentTopologyPolygonOffsetFaces(
    this->GetResolveCoincidentTopologyPolygonOffsetFaces());
  mapper->SetImmediateModeRendering(this->ImmediateModeRendering);
}

void vtkOpenGL2Glyph3DMapper::SetupColorMapper()
{
  this->ColorMapper->ShallowCopy(this);
}

// ---------------------------------------------------------------------------
// Description:
// Method initiates the mapping process. Generally sent by the actor
// as each frame is rendered.
void vtkOpenGL2Glyph3DMapper::Render(vtkRenderer *ren, vtkActor *actor)
{
  vtkOpenGLClearErrorMacro();

  this->SetupColorMapper();

  vtkHardwareSelector* selector = NULL;// ren->GetSelector();
  bool selecting_points = false; /* selector && (selector->GetFieldAssociation() ==
    vtkDataObject::FIELD_ASSOCIATION_POINTS); */

  if (selector)
    {
    selector->BeginRenderProp();
    }

  if (selector && !selecting_points)
    {
    // Selecting some other attribute. Not supported.
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    }

  vtkDataObject* inputDO = this->GetInputDataObject(0, 0);

  if (true) // Only supporting this path now, maybe with MTime
    {
    // Check input for consistency
    //
    // Create a default source, if no source is specified.
    if (this->GetSource(0) == 0)
      {
      vtkPolyData *defaultSource = vtkPolyData::New();
      defaultSource->Allocate();
      vtkPoints *defaultPoints = vtkPoints::New();
      defaultPoints->Allocate(6);
      defaultPoints->InsertNextPoint(0., 0., 0.);
      defaultPoints->InsertNextPoint(1., 0., 0.);
      vtkIdType defaultPointIds[2];
      defaultPointIds[0] = 0;
      defaultPointIds[1] = 1;
      defaultSource->SetPoints(defaultPoints);
      defaultSource->InsertNextCell(VTK_LINE, 2, defaultPointIds);
      this->SetSourceData(defaultSource);
      defaultSource->Delete();
      defaultSource = NULL;
      defaultPoints->Delete();
      defaultPoints = NULL;
      }
    this->Mapper->SetInputData(this->GetSource());
    }

  // Copy mapper ivar to sub-mapper
  this->CopyInformationToSubMapper(this->Mapper);

  // Render the input dataset or every dataset in the input composite dataset.
  vtkDataSet* ds = vtkDataSet::SafeDownCast(inputDO);
  vtkCompositeDataSet* cd = vtkCompositeDataSet::SafeDownCast(inputDO);
  if (ds)
    {
    this->Render(ren, actor, ds);
    }
  else if (cd)
    {
    vtkCompositeDataIterator* iter = cd->NewIterator();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal();
      iter->GoToNextItem())
      {
      ds = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
      if (ds)
        {
        if (selector)
          {
          selector->RenderCompositeIndex(iter->GetCurrentFlatIndex());
          }
        this->Render(ren, actor, ds);
        }
      }
    iter->Delete();
    }

  if (selector && !selecting_points)
    {
    // Selecting some other attribute. Not supported.
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    }
  if (selector)
    {
    selector->EndRenderProp();
    }

  vtkOpenGLCheckErrorMacro("Failed after Render");

  this->UpdateProgress(1.0);
}

// ---------------------------------------------------------------------------
void vtkOpenGL2Glyph3DMapper::Render(
  vtkRenderer* ren, vtkActor* actor, vtkDataSet* dataset)
{
  vtkIdType numPts = dataset->GetNumberOfPoints();
  if (numPts < 1)
    {
    vtkDebugMacro(<<"No points to glyph!");
    return;
    }

  // lookup the values for this dataset
  vtkOpenGL2Glyph3DMapper::vtkOpenGL2Glyph3DMapperEntry *entry;
  bool building = true;
  typedef std::map<const vtkDataSet *,vtkOpenGL2Glyph3DMapper::vtkOpenGL2Glyph3DMapperEntry *>::iterator GVIter;
  GVIter found = this->GlyphValues->Entries.find(dataset);
  if (found == this->GlyphValues->Entries.end())
    {
    entry = new vtkOpenGL2Glyph3DMapper::vtkOpenGL2Glyph3DMapperEntry();
    this->GlyphValues->Entries.insert(std::make_pair(dataset, entry));
    }
  else
    {
    building = false;
    entry = found->second;
    }

  vtkOpenGLClearErrorMacro();

  vtkHardwareSelector* selector = ren->GetSelector();
  bool selecting_points = selector && (selector->GetFieldAssociation() ==
    vtkDataObject::FIELD_ASSOCIATION_POINTS);

  double den = this->Range[1] - this->Range[0];
  if (den == 0.0)
    {
    den = 1.0;
    }

  vtkBitArray *maskArray = 0;
  if (this->Masking)
    {
    maskArray = vtkBitArray::SafeDownCast(this->GetMaskArray(dataset));
    if (maskArray == 0)
      {
      vtkDebugMacro(<<"masking is enabled but there is no mask array. Ignore masking.");
      }
    else
      {
      if (maskArray->GetNumberOfComponents() != 1)
        {
        vtkErrorMacro(" expecting a mask array with one component, getting "
          << maskArray->GetNumberOfComponents() << " components.");
        return;
        }
      }
    }

  if (building)
    {
    entry->Colors.resize(numPts*4);
    entry->Matrices.resize(numPts);
    vtkTransform *trans = vtkTransform::New();
    vtkDataArray* scaleArray = this->GetScaleArray(dataset);
    vtkDataArray* orientArray = this->GetOrientationArray(dataset);
    vtkDataArray* selectionArray = this->GetSelectionIdArray(dataset);
    if (orientArray !=0 && orientArray->GetNumberOfComponents() != 3)
      {
      vtkErrorMacro(" expecting an orientation array with 3 component, getting "
        << orientArray->GetNumberOfComponents() << " components.");
      return;
      }

    /// FIXME: Didn't handle the premultiplycolorswithalpha aspect...
    vtkUnsignedCharArray* colors = NULL;
    this->ColorMapper->SetInputDataObject(dataset);
    this->ColorMapper->MapScalars(actor->GetProperty()->GetOpacity());
    colors = this->ColorMapper->GetColors();
  //  bool multiplyWithAlpha =
  //    (this->ScalarsToColorsPainter->GetPremultiplyColorsWithAlpha(actor) == 1);
    // Traverse all Input points, transforming Source points
    for (vtkIdType inPtId = 0; inPtId < numPts; inPtId++)
      {
      entry->Colors[inPtId*4] = 255;
      entry->Colors[inPtId*4+1] = 255;
      entry->Colors[inPtId*4+2] = 255;
      entry->Colors[inPtId*4+3] = 255;

      if (!(inPtId % 10000))
        {
        this->UpdateProgress (static_cast<double>(inPtId)/
          static_cast<double>(numPts));
        if (this->GetAbortExecute())
          {
          break;
          }
        }

      if (maskArray && maskArray->GetValue(inPtId) == 0)
        {
        continue;
        }

      double scalex = 1.0;
      double scaley = 1.0;
      double scalez = 1.0;
      // Get the scalar and vector data
      if (scaleArray)
        {
        double* tuple = scaleArray->GetTuple(inPtId);
        switch (this->ScaleMode)
          {
        case SCALE_BY_MAGNITUDE:
          scalex = scaley = scalez = vtkMath::Norm(tuple,
            scaleArray->GetNumberOfComponents());
          break;
        case SCALE_BY_COMPONENTS:
          if (scaleArray->GetNumberOfComponents() != 3)
            {
            vtkErrorMacro("Cannot scale by components since " <<
              scaleArray->GetName() << " does not have 3 components.");
            }
          else
            {
            scalex = tuple[0];
            scaley = tuple[1];
            scalez = tuple[2];
            }
          break;
        case NO_DATA_SCALING:
        default:
          break;
          }

        // Clamp data scale if enabled
        if (this->Clamping && this->ScaleMode != NO_DATA_SCALING)
          {
          scalex = (scalex < this->Range[0] ? this->Range[0] :
            (scalex > this->Range[1] ? this->Range[1] : scalex));
          scalex = (scalex - this->Range[0]) / den;
          scaley = (scaley < this->Range[0] ? this->Range[0] :
            (scaley > this->Range[1] ? this->Range[1] : scaley));
          scaley = (scaley - this->Range[0]) / den;
          scalez = (scalez < this->Range[0] ? this->Range[0] :
            (scalez > this->Range[1] ? this->Range[1] : scalez));
          scalez = (scalez - this->Range[0]) / den;
          }
        }
      scalex *= this->ScaleFactor;
      scaley *= this->ScaleFactor;
      scalez *= this->ScaleFactor;

      // Now begin copying/transforming glyph
      trans->Identity();

      // translate Source to Input point
      double x[3];
      dataset->GetPoint(inPtId, x);
      trans->Translate(x[0], x[1], x[2]);

      if (orientArray)
        {
        double orientation[3];
        orientArray->GetTuple(inPtId, orientation);
        switch (this->OrientationMode)
          {
        case ROTATION:
          trans->RotateZ(orientation[2]);
          trans->RotateX(orientation[0]);
          trans->RotateY(orientation[1]);
          break;

        case DIRECTION:
          if (orientation[1] == 0.0 && orientation[2] == 0.0)
            {
            if (orientation[0] < 0) //just flip x if we need to
              {
              trans->RotateWXYZ(180.0, 0, 1, 0);
              }
            }
          else
            {
            double vMag = vtkMath::Norm(orientation);
            double vNew[3];
            vNew[0] = (orientation[0] + vMag) / 2.0;
            vNew[1] = orientation[1] / 2.0;
            vNew[2] = orientation[2] / 2.0;
            trans->RotateWXYZ(180.0, vNew[0], vNew[1], vNew[2]);
            }
          break;
          }
        }

      // Set color
      if (selecting_points)
        {
        // Use selectionArray value or glyph point ID.
        vtkIdType selectionId = inPtId;
        if (this->UseSelectionIds)
          {
          if (selectionArray == NULL ||
              selectionArray->GetNumberOfTuples() == 0)
            {
            vtkWarningMacro(<<"UseSelectionIds is true, but selection array"
                            " is invalid. Ignoring selection array.");
            }
          else
            {
          selectionId = static_cast<vtkIdType>(
                *selectionArray->GetTuple(inPtId));
            }
          }
        selector->RenderAttributeId(selectionId);
        }
      else if (colors)
        {
        unsigned char rgba[4];
        colors->GetTupleValue(inPtId, rgba);
        entry->Colors[inPtId*4] = rgba[0];
        entry->Colors[inPtId*4+1] = rgba[1];
        entry->Colors[inPtId*4+2] = rgba[2];
        entry->Colors[inPtId*4+3] = rgba[3];
        }

      // scale data if appropriate
      if (this->Scaling)
        {
        if (scalex == 0.0)
          {
          scalex = 1.0e-10;
          }
        if (scaley == 0.0)
          {
          scaley = 1.0e-10;
          }
        if (scalez == 0.0)
          {
          scalez = 1.0e-10;
          }
        trans->Scale(scalex, scaley, scalez);
        }

      entry->Matrices[inPtId] = vtkMatrix4x4::New();
      entry->Matrices[inPtId]->DeepCopy(trans->GetMatrix());
      }
    trans->Delete();
    }

  // now draw
  bool primed = false;
  for (vtkIdType inPtId = 0; inPtId < numPts; inPtId++)
    {
    if (maskArray && maskArray->GetValue(inPtId) == 0)
      {
      continue;
      }
    unsigned char rgba[4];
    rgba[0] = entry->Colors[inPtId*4];
    rgba[1] = entry->Colors[inPtId*4+1];
    rgba[2] = entry->Colors[inPtId*4+2];
    rgba[3] = entry->Colors[inPtId*4+3];
    this->Mapper->SetModelColor(rgba);
    this->Mapper->SetModelTransform(entry->Matrices[inPtId]);
    if (!primed)
      {
      this->Mapper->RenderPieceStart(ren, actor);
      primed = true;
      }






    this->Mapper->RenderPieceDraw(ren, actor);
    }
  if (primed)
    {
    this->Mapper->RenderPieceFinish(ren, actor);
    }

  vtkOpenGLCheckErrorMacro("failed after Render");
}

/*
{
  vtkgl::ShaderProgram &program = cellBO.CachedProgram->Program;

  vtkCamera *cam = ren->GetActiveCamera();

  vtkNew<vtkMatrix4x4> tmpMat;
  tmpMat->DeepCopy(cam->GetModelViewTransformMatrix());
  if (this->ModelTransformMatrix)
    {
    // Apply this extra transform from things like the glyph mapper.
    vtkMatrix4x4::Multiply4x4(tmpMat.Get(), this->ModelTransformMatrix,
                              tmpMat.Get());
    }

  // compute the combined ModelView matrix and send it down to save time in the shader
  vtkMatrix4x4::Multiply4x4(tmpMat.Get(), actor->GetMatrix(), tmpMat.Get());
  tmpMat->Transpose();
  program.SetUniformValue("MCVCMatrix", tmpMat.Get());

  // for lit shaders set normal matrix
  if (this->Internal->LastLightComplexity > 0)
    {
    tmpMat->Transpose();

    // set the normal matrix and send it down
    // (make this a function in camera at some point returning a 3x3)
    // Reuse the matrix we already got (and possibly multiplied with model mat.
    if (!actor->GetIsIdentity() || this->ModelTransformMatrix)
      {
      vtkNew<vtkTransform> aTF;
      aTF->SetMatrix(tmpMat.Get());
      double *scale = aTF->GetScale();
      aTF->Scale(1.0 / scale[0], 1.0 / scale[1], 1.0 / scale[2]);
      tmpMat->DeepCopy(aTF->GetMatrix());
      }
    vtkNew<vtkMatrix3x3> tmpMat3d;
    for(int i = 0; i < 3; ++i)
      {
      for (int j = 0; j < 3; ++j)
        {
        tmpMat3d->SetElement(i, j, tmpMat->GetElement(i, j));
        }
      }
    tmpMat3d->Invert();
    program.SetUniformValue("normalMatrix", tmpMat3d.Get());
    }

  // Query the actor for some of the properties that can be applied.
  float opacity = static_cast<float>(actor->GetProperty()->GetOpacity());
  vtkgl::Vector3ub diffuseColor(static_cast<unsigned char>(dColor[0] * dIntensity * 255.0),
                         static_cast<unsigned char>(dColor[1] * dIntensity * 255.0),
                         static_cast<unsigned char>(dColor[2] * dIntensity * 255.0));

  // Override the model color when the value was set directly on the mapper.
  if (this->ModelColor)
    {
    for (int i = 0; i < 3; ++i)
      {
      diffuseColor[i] = this->ModelColor[i];
      }
    opacity = this->ModelColor[3]/255.0;
    }

  program.SetUniformValue("opacityUniform", opacity);
  program.SetUniformValue("diffuseColorUniform", diffuseColor);

  if (first)
    {
    this->Internal->triStrips.ibo.Bind();
    }

  if (actor->GetProperty()->GetRepresentation() == VTK_POINTS)
    {
    glDrawRangeElements(GL_POINTS, 0,
                        static_cast<GLuint>(layout.VertexCount - 1),
                        static_cast<GLsizei>(this->Internal->triStrips.indexCount),
                        GL_UNSIGNED_INT,
                        reinterpret_cast<const GLvoid *>(NULL));
    }
  // TODO fix wireframe
  if (actor->GetProperty()->GetRepresentation() == VTK_WIREFRAME)
    {
    for (size_t eCount = 0;
         eCount < this->Internal->triStrips.offsetArray.size(); ++eCount)
      {
      glDrawElements(GL_LINE_STRIP,
        this->Internal->triStrips.elementsArray[eCount],
        GL_UNSIGNED_INT,
        (GLvoid *)(this->Internal->triStrips.offsetArray[eCount]));
      }
    }
  if (actor->GetProperty()->GetRepresentation() == VTK_SURFACE)
    {
    for (size_t eCount = 0;
         eCount < this->Internal->triStrips.offsetArray.size(); ++eCount)
      {
      glDrawElements(GL_TRIANGLE_STRIP,
        this->Internal->triStrips.elementsArray[eCount],
        GL_UNSIGNED_INT,
        (GLvoid *)(this->Internal->triStrips.offsetArray[eCount]));
      }
    }

  if (last)
    {
    this->Internal->triStrips.ibo.Release();
    }
}
*/



// ---------------------------------------------------------------------------
// Description:
// Release any graphics resources that are being consumed by this mapper.
void vtkOpenGL2Glyph3DMapper::ReleaseGraphicsResources(vtkWindow *window)
{
  if (this->Mapper)
    {
    this->Mapper->ReleaseGraphicsResources(window);
    }
}

// ----------------------------------------------------------------------------
void vtkOpenGL2Glyph3DMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

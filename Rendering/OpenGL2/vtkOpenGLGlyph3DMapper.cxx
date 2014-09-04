/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLGlyph3DMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenGLGlyph3DMapper.h"

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
#include "vtkOpenGLGlyph3DHelper.h"
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

class vtkOpenGLGlyph3DMapper::vtkColorMapper : public vtkMapper
{
public:
  vtkTypeMacro(vtkColorMapper, vtkMapper);
  static vtkColorMapper* New() { return new vtkColorMapper; }
  void Render(vtkRenderer *, vtkActor *) {}
  vtkUnsignedCharArray* GetColors() { return this->Colors; }
};

class vtkOpenGLGlyph3DMapper::vtkOpenGLGlyph3DMapperEntry
{
public:
  std::vector<unsigned char> Colors;
  std::vector<vtkMatrix4x4 * > Matrices;
  vtkTimeStamp BuildTime;
  bool LastSelectingState;

  vtkOpenGLGlyph3DMapperEntry()  { this->LastSelectingState = false; };
  ~vtkOpenGLGlyph3DMapperEntry()
  {
    std::vector<vtkMatrix4x4 * >::iterator miter = this->Matrices.begin();
    for (;miter != this->Matrices.end(); miter++)
      {
      (*miter)->Delete();
      }
  };
};

class vtkOpenGLGlyph3DMapper::vtkOpenGLGlyph3DMapperArray
{
public:
  std::map<const vtkDataSet *, vtkOpenGLGlyph3DMapper::vtkOpenGLGlyph3DMapperEntry *>  Entries;
  ~vtkOpenGLGlyph3DMapperArray()
  {
    std::map<const vtkDataSet *, vtkOpenGLGlyph3DMapper::vtkOpenGLGlyph3DMapperEntry *>::iterator miter = this->Entries.begin();
    for (;miter != this->Entries.end(); miter++)
      {
      delete miter->second;
      }
  };
};

vtkStandardNewMacro(vtkOpenGLGlyph3DMapper)

// ---------------------------------------------------------------------------
// Construct object with scaling on, scaling mode is by scalar value,
// scale factor = 1.0, the range is (0,1), orient geometry is on, and
// orientation is by vector. Clamping and indexing are turned off. No
// initial sources are defined.
vtkOpenGLGlyph3DMapper::vtkOpenGLGlyph3DMapper()
{
  this->GlyphValues = new vtkOpenGLGlyph3DMapper::vtkOpenGLGlyph3DMapperArray();
  this->Mapper = vtkOpenGLGlyph3DHelper::New();
  this->Mapper->SetPopulateSelectionSettings(0);
  this->LastWindow = 0;
}

// ---------------------------------------------------------------------------
vtkOpenGLGlyph3DMapper::~vtkOpenGLGlyph3DMapper()
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
void vtkOpenGLGlyph3DMapper::CopyInformationToSubMapper(
    vtkOpenGLGlyph3DHelper *mapper)
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

void vtkOpenGLGlyph3DMapper::SetupColorMapper()
{
  this->ColorMapper->ShallowCopy(this);
}

// ---------------------------------------------------------------------------
// Description:
// Method initiates the mapping process. Generally sent by the actor
// as each frame is rendered.
void vtkOpenGLGlyph3DMapper::Render(vtkRenderer *ren, vtkActor *actor)
{
  vtkOpenGLClearErrorMacro();

  this->SetupColorMapper();

  vtkHardwareSelector* selector = ren->GetSelector();
  bool selecting_points = selector && (selector->GetFieldAssociation() ==
    vtkDataObject::FIELD_ASSOCIATION_POINTS);

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
void vtkOpenGLGlyph3DMapper::Render(
  vtkRenderer* ren, vtkActor* actor, vtkDataSet* dataset)
{
  vtkIdType numPts = dataset->GetNumberOfPoints();
  if (numPts < 1)
    {
    vtkDebugMacro(<<"No points to glyph!");
    return;
    }

  // lookup the values for this dataset
  vtkOpenGLGlyph3DMapper::vtkOpenGLGlyph3DMapperEntry *entry;
  bool building = true;
  typedef std::map<const vtkDataSet *,vtkOpenGLGlyph3DMapper::vtkOpenGLGlyph3DMapperEntry *>::iterator GVIter;
  GVIter found = this->GlyphValues->Entries.find(dataset);
  if (found == this->GlyphValues->Entries.end())
    {
    entry = new vtkOpenGLGlyph3DMapper::vtkOpenGLGlyph3DMapperEntry();
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

  if (building || entry->BuildTime < dataset->GetMTime() ||
      entry->LastSelectingState != selecting_points)
    {
    entry->Colors.resize(numPts*4);
    // delete any prior matrices
    std::vector<vtkMatrix4x4 * >::iterator miter = entry->Matrices.begin();
    for (;miter != entry->Matrices.end(); miter++)
      {
      (*miter)->Delete();
      (*miter) = NULL;;
      }
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
        entry->Colors[inPtId*4] = selectionId & 0xff;
        entry->Colors[inPtId*4+1] = (selectionId & 0xff00) >> 8;
        entry->Colors[inPtId*4+2] = (selectionId & 0xff0000) >> 16;
        entry->Colors[inPtId*4+3] = 255;
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
    entry->LastSelectingState = selecting_points;
    entry->BuildTime.Modified();
    }

  // now draw, there is a fast path for a special case of
  // only triangles
  bool fastPath = false;
  vtkPolyData* pd = this->Mapper->GetInput();
  if (pd && pd->GetNumberOfVerts() == 0 && pd->GetNumberOfLines() == 0 && pd->GetNumberOfStrips() == 0)
    {
    fastPath = true;
    }

  bool primed = false;
  unsigned char rgba[4];
  for (vtkIdType inPtId = 0; inPtId < numPts; inPtId++)
    {
    if (maskArray && maskArray->GetValue(inPtId) == 0)
      {
      continue;
      }
    rgba[0] = entry->Colors[inPtId*4];
    rgba[1] = entry->Colors[inPtId*4+1];
    rgba[2] = entry->Colors[inPtId*4+2];
    rgba[3] = entry->Colors[inPtId*4+3];

    if (selecting_points)
      {
      selector->RenderAttributeId(rgba[0] + (rgba[1] << 8) + (rgba[2] << 16));
      }
    if (!primed)
      {
      if (fastPath)
        {
        this->Mapper->GlyphRender(ren, actor, rgba, entry->Matrices[inPtId], 1);
        }
      else
        {
        this->Mapper->RenderPieceStart(ren, actor);
        }
      primed = true;
      }
    if (fastPath)
      {
      this->Mapper->GlyphRender(ren, actor, rgba, entry->Matrices[inPtId], 2);
      }
    else
      {
      this->Mapper->SetModelColor(rgba);
      this->Mapper->SetModelTransform(entry->Matrices[inPtId]);
      this->Mapper->RenderPieceDraw(ren, actor);
      }
    }
  if (primed)
    {
    if (fastPath)
      {
      this->Mapper->GlyphRender(ren, actor, rgba, NULL, 3);
      }
    else
      {
      this->Mapper->RenderPieceFinish(ren, actor);
      }
    }

  vtkOpenGLCheckErrorMacro("failed after Render");
}

// ---------------------------------------------------------------------------
// Description:
// Release any graphics resources that are being consumed by this mapper.
void vtkOpenGLGlyph3DMapper::ReleaseGraphicsResources(vtkWindow *window)
{
  if (this->Mapper)
    {
    this->Mapper->ReleaseGraphicsResources(window);
    }
}

// ----------------------------------------------------------------------------
void vtkOpenGLGlyph3DMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

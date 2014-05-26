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

template <class T>
static T vtkClamp(T val, T min, T max)
{
  val = val < min? min : val;
  val = val > max? max : val;
  return val;
}

class vtkColorMapper : public vtkMapper
{
public:
  vtkTypeMacro(vtkColorMapper, vtkMapper);
  static vtkColorMapper* New();
  void Render(vtkRenderer *, vtkActor *) {}
  vtkUnsignedCharArray* GetColors() { return this->Colors; }
};

vtkStandardNewMacro(vtkColorMapper);

class vtkOpenGL2Glyph3DMapperArray
{
public:
  std::vector<vtkSmartPointer<vtkVBOPolyDataMapper > > Mappers;
};

vtkStandardNewMacro(vtkOpenGL2Glyph3DMapper)

// ---------------------------------------------------------------------------
// Construct object with scaling on, scaling mode is by scalar value,
// scale factor = 1.0, the range is (0,1), orient geometry is on, and
// orientation is by vector. Clamping and indexing are turned off. No
// initial sources are defined.
vtkOpenGL2Glyph3DMapper::vtkOpenGL2Glyph3DMapper()
{
  this->SourceMappers = 0;
  this->LastWindow = 0;
}

// ---------------------------------------------------------------------------
vtkOpenGL2Glyph3DMapper::~vtkOpenGL2Glyph3DMapper()
{
  delete this->SourceMappers;
  this->SourceMappers = 0;

  if (this->LastWindow)
    {
    this->ReleaseGraphicsResources(this->LastWindow);
    this->LastWindow = 0;
    }
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

// ---------------------------------------------------------------------------
// Description:
// Method initiates the mapping process. Generally sent by the actor
// as each frame is rendered.
void vtkOpenGL2Glyph3DMapper::Render(vtkRenderer *ren, vtkActor *actor)
{
  vtkOpenGLClearErrorMacro();

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
    int numberOfSources = this->GetNumberOfInputConnections(1);
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

    if (!this->SourceMappers)
      {
      this->SourceMappers = new vtkOpenGL2Glyph3DMapperArray();
      }
    if (/*indexArray*/ true)
      {
      this->SourceMappers->Mappers.resize(static_cast<size_t>(numberOfSources));
      }
    else
      {
      this->SourceMappers->Mappers.resize(1);
      }
    for (size_t cc = 0; cc < this->SourceMappers->Mappers.size(); cc++)
      {
      vtkPolyData *s = this->GetSource(static_cast<int>(cc));
      // s can be null.
      if (!this->SourceMappers->Mappers[cc])
        {
        this->SourceMappers->Mappers[cc] = vtkVBOPolyDataMapper::New();
        this->SourceMappers->Mappers[cc]->Delete();
        }
      // Copy mapper ivar to sub-mapper
      this->CopyInformationToSubMapper(this->SourceMappers->Mappers[cc]);

      vtkPolyData *ss = this->SourceMappers->Mappers[cc]->GetInput();
      if (!ss)
        {
        ss = vtkPolyData::New();
        this->SourceMappers->Mappers[cc]->SetInputData(ss);
        ss->Delete();
        ss->ShallowCopy(s);
        }

      if (s->GetMTime() > ss->GetMTime())
        {
        ss->ShallowCopy(s);
        }

      /// Create the VBOs, etc, FIXME: Add function to do so without calling render?
      //this->SourceMappers->Mappers[cc]->Render(ren, actor);
      }

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
    } // if(immediateMode||createDisplayList)

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

  vtkOpenGLClearErrorMacro();

  vtkHardwareSelector* selector = ren->GetSelector();
  bool selecting_points = selector && (selector->GetFieldAssociation() ==
    vtkDataObject::FIELD_ASSOCIATION_POINTS);

  double den = this->Range[1] - this->Range[0];
  if (den == 0.0)
    {
    den = 1.0;
    }

  int numberOfSources = this->GetNumberOfInputConnections(1);
  vtkTransform *trans = vtkTransform::New();
  vtkDataArray* scaleArray = this->GetScaleArray(dataset);
  vtkDataArray* orientArray = this->GetOrientationArray(dataset);
  vtkDataArray* indexArray = this->GetSourceIndexArray(dataset);
  vtkDataArray* selectionArray = this->GetSelectionIdArray(dataset);
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

  if (orientArray !=0 && orientArray->GetNumberOfComponents() != 3)
    {
    vtkErrorMacro(" expecting an orientation array with 3 component, getting "
      << orientArray->GetNumberOfComponents() << " components.");
    return;
    }

  /// FIXME: Didn't handle the premultiplycolorswithalpha aspect...
  vtkUnsignedCharArray* colors = NULL;
  vtkNew<vtkColorMapper> colorMapper;
  colorMapper->SetInputDataObject(dataset);
  colorMapper->MapScalars(actor->GetProperty()->GetOpacity());
  colors = colorMapper->GetColors();
//  bool multiplyWithAlpha =
//    (this->ScalarsToColorsPainter->GetPremultiplyColorsWithAlpha(actor) == 1);
  // Traverse all Input points, transforming Source points
  for (vtkIdType inPtId = 0; inPtId < numPts; inPtId++)
    {
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

    int index = 0;
    // Compute index into table of glyphs
    if (indexArray)
      {
      double value = vtkMath::Norm(indexArray->GetTuple(inPtId),
        indexArray->GetNumberOfComponents());
      index = static_cast<int>((value - this->Range[0]) * numberOfSources / den);
      index = ::vtkClamp(index, 0, numberOfSources - 1);
      }

    // source can be null.
    vtkPolyData *source = this->GetSource(index);

    // Make sure we're not indexing into empty glyph
    if (source)
      {
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

      vtkVBOPolyDataMapper *mapper =
          this->SourceMappers->Mappers[static_cast<size_t>(index)];

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
        mapper->SetModelColor(rgba);
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

      // multiply points and normals by resulting matrix
      vtkMatrix4x4* mat = trans->GetMatrix();
      mapper->SetModelTransform(mat);
      mapper->Render(ren, actor);
      mapper->SetModelTransform(0);
      }
    }
  trans->Delete();

  vtkOpenGLCheckErrorMacro("failed after Render");
}

// ---------------------------------------------------------------------------
// Description:
// Release any graphics resources that are being consumed by this mapper.
void vtkOpenGL2Glyph3DMapper::ReleaseGraphicsResources(vtkWindow *window)
{
  if (this->SourceMappers)
    {
    for (size_t i = 0; i < this->SourceMappers->Mappers.size(); ++i)
      {
      this->SourceMappers->Mappers[i]->ReleaseGraphicsResources(window);
      }
    }
}

// ----------------------------------------------------------------------------
void vtkOpenGL2Glyph3DMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

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
#include "vtkDefaultPainter.h"
#include "vtkGarbageCollector.h"
#include "vtkHardwareSelector.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLookupTable.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPainterPolyDataMapper.h"
#include "vtkPointData.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkScalarsToColorsPainter.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTimerLog.h"
#include "vtkTransform.h"
#include "vtkHardwareSelectionPolyDataPainter.h"
#include "vtkOpenGLError.h"

#include <cassert>
#include <vector>
#include "vtkgl.h"

vtkStandardNewMacro(vtkOpenGLGlyph3DMapper);

template <class T>
static T vtkClamp(T val, T min, T max)
{
  val = val < min? min : val;
  val = val > max? max : val;
  return val;
}

class vtkOpenGLGlyph3DMapperArray
{
public:
  std::vector<vtkSmartPointer<vtkPainterPolyDataMapper > > Mappers;
};

// ---------------------------------------------------------------------------
// Construct object with scaling on, scaling mode is by scalar value,
// scale factor = 1.0, the range is (0,1), orient geometry is on, and
// orientation is by vector. Clamping and indexing are turned off. No
// initial sources are defined.
vtkOpenGLGlyph3DMapper::vtkOpenGLGlyph3DMapper()
{
  this->SourceMappers = 0;

  this->DisplayListId = 0; // for the matrices and color per glyph
  this->LastWindow = 0;

  this->ScalarsToColorsPainter = vtkScalarsToColorsPainter::New();
  this->PainterInformation = vtkInformation::New();
  this->ScalarsToColorsPainter->SetInformation(this->PainterInformation);
}

// ---------------------------------------------------------------------------
vtkOpenGLGlyph3DMapper::~vtkOpenGLGlyph3DMapper()
{
  delete this->SourceMappers;
  this->SourceMappers = 0;

  if (this->LastWindow)
    {
    this->ReleaseGraphicsResources(this->LastWindow);
    this->LastWindow = 0;
    }
  if (this->ScalarsToColorsPainter)
    {
    this->ScalarsToColorsPainter->Delete();
    this->ScalarsToColorsPainter = 0;
    }
  if (this->PainterInformation)
    {
    this->PainterInformation->Delete();
    this->PainterInformation = 0;
    }
}

// ---------------------------------------------------------------------------
void vtkOpenGLGlyph3DMapper::UpdatePainterInformation()
{
  if (this->GetMTime() < this->PainterUpdateTime)
    {
    return;
    }

  vtkInformation* info = this->PainterInformation;

  info->Set(vtkPainter::STATIC_DATA(), this->Static);
  info->Set(vtkScalarsToColorsPainter::USE_LOOKUP_TABLE_SCALAR_RANGE(),
    this->GetUseLookupTableScalarRange());
  info->Set(vtkScalarsToColorsPainter::SCALAR_RANGE(),
    this->GetScalarRange(), 2);
  info->Set(vtkScalarsToColorsPainter::SCALAR_MODE(), this->GetScalarMode());
  info->Set(vtkScalarsToColorsPainter::COLOR_MODE(), this->GetColorMode());
  info->Set(vtkScalarsToColorsPainter::INTERPOLATE_SCALARS_BEFORE_MAPPING(),
    this->GetInterpolateScalarsBeforeMapping());
  info->Set(vtkScalarsToColorsPainter::LOOKUP_TABLE(), this->LookupTable);
  info->Set(vtkScalarsToColorsPainter::SCALAR_VISIBILITY(),
    this->GetScalarVisibility());
  info->Set(vtkScalarsToColorsPainter::ARRAY_ACCESS_MODE(),
    this->ArrayAccessMode);
  info->Set(vtkScalarsToColorsPainter::ARRAY_ID(), this->ArrayId);
  info->Set(vtkScalarsToColorsPainter::ARRAY_NAME(), this->ArrayName);
  info->Set(vtkScalarsToColorsPainter::ARRAY_COMPONENT(), this->ArrayComponent);
  info->Set(vtkScalarsToColorsPainter::SCALAR_MATERIAL_MODE(),
    this->GetScalarMaterialMode());
  this->PainterUpdateTime.Modified();
}


// ---------------------------------------------------------------------------
// Description:
// Send mapper ivars to sub-mapper.
// \pre mapper_exists: mapper!=0
void vtkOpenGLGlyph3DMapper::CopyInformationToSubMapper(
  vtkPainterPolyDataMapper *mapper)
{
  assert("pre: mapper_exists" && mapper!=0);

  // see void vtkPainterPolyDataMapper::UpdatePainterInformation()

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
void vtkOpenGLGlyph3DMapper::Render(vtkRenderer *ren, vtkActor *actor)
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
  bool immediateMode = this->ImmediateModeRendering ||
    vtkMapper::GetGlobalImmediateModeRendering() ||
    !this->NestedDisplayLists || (selecting_points);

  vtkDataObject* inputDO = this->GetInputDataObject(0, 0);

  vtkProperty *prop = actor->GetProperty();

  bool createDisplayList = false;

  if (immediateMode)
    {
    this->ReleaseList();
    }
  else
    {
    // if something has changed, regenerate display lists.
    createDisplayList = this->DisplayListId == 0 ||
      this->GetMTime() > this->BuildTime ||
      inputDO->GetMTime() > this->BuildTime ||
      prop->GetMTime() > this->BuildTime ||
      ren->GetRenderWindow() != this->LastWindow.GetPointer();
    }

  if (immediateMode || createDisplayList)
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
      this->SourceMappers = new vtkOpenGLGlyph3DMapperArray();
      }
    if (/*indexArray*/ true)
      {
      this->SourceMappers->Mappers.resize(
        static_cast<size_t>(numberOfSources));
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
        this->SourceMappers->Mappers[cc] = vtkPainterPolyDataMapper::New();
        this->SourceMappers->Mappers[cc]->Delete();
        vtkDefaultPainter *p =
          static_cast<vtkDefaultPainter *>(this->SourceMappers->Mappers[cc]->GetPainter());
        p->SetScalarsToColorsPainter(0); // bypass default mapping.
        p->SetClipPlanesPainter(0); // bypass default mapping.
        vtkHardwareSelectionPolyDataPainter::SafeDownCast(
          this->SourceMappers->Mappers[cc]->GetSelectionPainter())->EnableSelectionOff();
          // use the same painter for selection pass as well.
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

      if (s->GetMTime()>ss->GetMTime())
        {
        ss->ShallowCopy(s);
        }

      if (createDisplayList)
        {
        this->SourceMappers->Mappers[cc]->SetForceCompileOnly(1);
        this->SourceMappers->Mappers[cc]->Render(ren, actor); // compile display list.
        this->SourceMappers->Mappers[cc]->SetForceCompileOnly(0);
        }
      }

    if (createDisplayList)
      {
      this->ReleaseList();
      this->DisplayListId = glGenLists(1);
      glNewList(this->DisplayListId, GL_COMPILE);
      }
    this->UpdatePainterInformation();

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

    if (createDisplayList)
      {
      glEndList();
      this->BuildTime.Modified();
      this->LastWindow = ren->GetRenderWindow();
      }

    } // if(immediateMode||createDisplayList)

  if (!immediateMode)
    {
    this->TimeToDraw = 0.0;
    this->Timer->StartTimer();
    glCallList(this->DisplayListId);
    this->Timer->StopTimer();
    this->TimeToDraw += this->Timer->GetElapsedTime();
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

  this->ScalarsToColorsPainter->SetInput(dataset);
  this->ScalarsToColorsPainter->Render(ren, actor, 0xff, false);
  vtkUnsignedCharArray* colors = this->GetColors(
    vtkDataSet::SafeDownCast(this->ScalarsToColorsPainter->GetOutput()));
  bool multiplyWithAlpha =
    (this->ScalarsToColorsPainter->GetPremultiplyColorsWithAlpha(actor) == 1);
  if (multiplyWithAlpha)
    {
    // We colors were premultiplied by alpha then we change the blending
    // function to one that will compute correct blended destination alpha
    // value, otherwise we stick with the default.
    // save the blend function.
    glPushAttrib(GL_COLOR_BUFFER_BIT);
    // the following function is not correct with textures because there
    // are not premultiplied by alpha.
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    }

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
      index = static_cast<int>((value-this->Range[0])*numberOfSources/den);
      index = ::vtkClamp(index, 0, numberOfSources-1);
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
              trans->RotateWXYZ(180.0,0,1,0);
              }
            }
          else
            {
            double vMag = vtkMath::Norm(orientation);
            double vNew[3];
            vNew[0] = (orientation[0]+vMag) / 2.0;
            vNew[1] = orientation[1] / 2.0;
            vNew[2] = orientation[2] / 2.0;
            trans->RotateWXYZ(180.0,vNew[0],vNew[1],vNew[2]);
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
        glColor4ub(rgba[0], rgba[1], rgba[2], rgba[3]);
        }
      //glFinish(); // for debug

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
      // glFinish(); // for debug
      glMatrixMode(GL_MODELVIEW);
      glPushMatrix();
      double mat[16];
      vtkMatrix4x4::Transpose(*trans->GetMatrix()->Element, mat);
      glMultMatrixd(mat);
      this->SourceMappers->Mappers[static_cast<size_t>(index)]->
        Render(ren, actor);
      // assume glMatrix(GL_MODELVIEW);
      glMatrixMode(GL_MODELVIEW);
      glPopMatrix();
      //glFinish(); // for debug
      }
    }
  trans->Delete();

  // from vtkOpenGLScalarsToColorsPainter::RenderInternal
  if (multiplyWithAlpha)
    {
    // restore the blend function
    glPopAttrib();
    }

  vtkOpenGLCheckErrorMacro("failed after Render");
}

// ---------------------------------------------------------------------------
// Description:
// Release any graphics resources that are being consumed by this mapper.
// The parameter window could be used to determine which graphic
// resources to release.
void vtkOpenGLGlyph3DMapper::ReleaseGraphicsResources(vtkWindow *window)
{
  if(this->SourceMappers!=0)
    {
    size_t c=this->SourceMappers->Mappers.size();
    size_t i=0;
    while(i<c)
      {
      this->SourceMappers->Mappers[i]->ReleaseGraphicsResources(window);
      ++i;
      }
    }
  this->ReleaseList();
}

// ---------------------------------------------------------------------------
// Description:
// Release display list used for matrices and color.
void vtkOpenGLGlyph3DMapper::ReleaseList()
{
  if(this->DisplayListId>0)
    {
    glDeleteLists(this->DisplayListId,1);
    this->DisplayListId = 0;
    vtkOpenGLCheckErrorMacro("failed after ReleaseList");
    }
}

//-----------------------------------------------------------------------------
void vtkOpenGLGlyph3DMapper::ReportReferences(vtkGarbageCollector *collector)
{
  this->Superclass::ReportReferences(collector);
  vtkGarbageCollectorReport(collector, this->ScalarsToColorsPainter,
    "ScalarsToColorsPainter");
}

// ----------------------------------------------------------------------------
void vtkOpenGLGlyph3DMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

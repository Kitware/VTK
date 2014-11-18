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
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkHardwareSelector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLGlyph3DHelper.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkTransform.h"
#include "vtkOpenGLError.h"
#include "vtkSmartPointer.h"

#include <map>

template <class T>
static T vtkClamp(T val, T min, T max)
{
  val = val < min? min : val;
  val = val > max? max : val;
  return val;
}

class vtkOpenGLGlyph3DMappervtkColorMapper : public vtkMapper
{
public:
  vtkTypeMacro(vtkOpenGLGlyph3DMappervtkColorMapper, vtkMapper);
  static vtkOpenGLGlyph3DMappervtkColorMapper* New();
  void Render(vtkRenderer *, vtkActor *) {}
  vtkUnsignedCharArray* GetColors() { return this->Colors; }
};

vtkStandardNewMacro(vtkOpenGLGlyph3DMappervtkColorMapper)

class vtkOpenGLGlyph3DMapper::vtkOpenGLGlyph3DMapperEntry
{
public:
  std::vector<vtkIdType> PickIds;
  std::vector<unsigned char> Colors;
  std::vector<float> Matrices;  // transposed
  std::vector<float> NormalMatrices; // transposed
  vtkTimeStamp BuildTime;
  vtkOpenGLGlyph3DHelper *Mapper;
  int NumberOfPoints;

  vtkOpenGLGlyph3DMapperEntry()
  {
    this->Mapper = vtkOpenGLGlyph3DHelper::New();
    vtkPolyData *ss = vtkPolyData::New();
    this->Mapper->SetInputData(ss);
    ss->Delete();
    this->Mapper->SetPopulateSelectionSettings(0);
  };
  ~vtkOpenGLGlyph3DMapperEntry()
  {
    this->Mapper->Delete();
  };
};

class vtkOpenGLGlyph3DMapper::vtkOpenGLGlyph3DMapperSubArray
{
public:
  std::map<size_t, vtkOpenGLGlyph3DMapper::vtkOpenGLGlyph3DMapperEntry *>  Entries;
  vtkTimeStamp BuildTime;
  bool LastSelectingState;
  vtkOpenGLGlyph3DMapperSubArray()
  {
    this->LastSelectingState = false;
  };
  ~vtkOpenGLGlyph3DMapperSubArray()
  {
    std::map<size_t, vtkOpenGLGlyph3DMapper::vtkOpenGLGlyph3DMapperEntry *>::iterator miter = this->Entries.begin();
    for (;miter != this->Entries.end(); miter++)
      {
      delete miter->second;
      }
  };
};

class vtkOpenGLGlyph3DMapper::vtkOpenGLGlyph3DMapperArray
{
public:
  std::map<const vtkDataSet *, vtkOpenGLGlyph3DMapper::vtkOpenGLGlyph3DMapperSubArray *> Entries;
  ~vtkOpenGLGlyph3DMapperArray()
  {
    std::map<const vtkDataSet *, vtkOpenGLGlyph3DMapper::vtkOpenGLGlyph3DMapperSubArray *>::iterator miter = this->Entries.begin();
    for (;miter != this->Entries.end(); miter++)
      {
      delete miter->second;
      }
    this->Entries.clear();
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
  this->LastWindow = 0;
  this->ColorMapper = vtkOpenGLGlyph3DMappervtkColorMapper::New();
}

// ---------------------------------------------------------------------------
vtkOpenGLGlyph3DMapper::~vtkOpenGLGlyph3DMapper()
{
  this->ColorMapper->Delete();
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

  // make sure we have an entry for this dataset
  vtkOpenGLGlyph3DMapper::vtkOpenGLGlyph3DMapperSubArray *subarray;
  bool rebuild = false;
  typedef std::map<const vtkDataSet *,vtkOpenGLGlyph3DMapper::vtkOpenGLGlyph3DMapperSubArray *>::iterator GVIter;
  GVIter found = this->GlyphValues->Entries.find(dataset);
  if (found == this->GlyphValues->Entries.end())
    {
    subarray = new vtkOpenGLGlyph3DMapper::vtkOpenGLGlyph3DMapperSubArray();
    this->GlyphValues->Entries.insert(std::make_pair(dataset, subarray));
    rebuild = true;
    }
  else
    {
    subarray = found->second;
    }

  // make sure we have a subentry for each source
  unsigned int numberOfSources = this->GetNumberOfInputConnections(1);
  bool numberOfSourcesChanged = false;
  if (numberOfSources != subarray->Entries.size())
    {
    subarray->Entries.clear();
    for (size_t cc = 0; cc < numberOfSources; cc++)
      {
      subarray->Entries.insert(std::make_pair(cc,
        new vtkOpenGLGlyph3DMapper::vtkOpenGLGlyph3DMapperEntry()));
      }
    numberOfSourcesChanged = true;
    }

  // make sure sources are up to date
  for (size_t cc = 0; cc < subarray->Entries.size(); cc++)
    {
    vtkPolyData *s = this->GetSource(static_cast<int>(cc));
    vtkOpenGLGlyph3DHelper *gh = subarray->Entries[cc]->Mapper;
    vtkPolyData *ss = gh->GetInput();
    if (s->GetMTime() > ss->GetMTime() || numberOfSourcesChanged)
      {
      ss->ShallowCopy(s);
      // Copy mapper ivar to sub-mapper
      this->CopyInformationToSubMapper(gh);
      }
    }

  vtkHardwareSelector* selector = ren->GetSelector();
  bool selecting_points = selector && (selector->GetFieldAssociation() ==
    vtkDataObject::FIELD_ASSOCIATION_POINTS);

  // rebuild all entries for this DataSet if it
  // has been modified
  if (subarray->BuildTime < dataset->GetMTime() ||
      subarray->LastSelectingState != selecting_points )
    {
    rebuild = true;
    }

  // get the mask array
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

  // rebuild all sources for this dataset
  if (rebuild)
    {
    this->RebuildStructures(subarray, numPts, actor, dataset, maskArray,
            selecting_points);
    }

  // for each subarray
  for (size_t cc = 0; cc < subarray->Entries.size(); cc++)
    {
    vtkOpenGLGlyph3DMapper::vtkOpenGLGlyph3DMapperEntry *entry =
      subarray->Entries[cc];
    vtkOpenGLGlyph3DHelper *gh = entry->Mapper;

    // now draw, there is a fast path for a special case of
    // only triangles
    bool fastPath = false;
    vtkPolyData* pd = gh->GetInput();
    gh->CurrentInput = pd;
    if (pd && pd->GetNumberOfVerts() == 0 && pd->GetNumberOfLines() == 0 && pd->GetNumberOfStrips() == 0)
      {
      fastPath = true;
      }

    // use fast path
    gh->SetUseFastPath(fastPath);
    if (fastPath)
      {
      gh->GlyphRender(ren, actor, entry->NumberOfPoints,
        entry->Colors, entry->Matrices, entry->NormalMatrices,
        entry->PickIds, subarray->BuildTime);
      }
    else
      {
      bool primed = false;
      for (vtkIdType inPtId = 0; inPtId < entry->NumberOfPoints; inPtId++)
        {
        if (selecting_points)
          {
          selector->RenderAttributeId(entry->PickIds[inPtId]);
          }
        if (!primed)
          {
          gh->RenderPieceStart(ren, actor);
          primed = true;
          }
        gh->SetModelColor(&(entry->Colors[inPtId*4]));
        gh->SetModelTransform(&(entry->Matrices[inPtId*16]));
        gh->SetModelNormalTransform(&(entry->NormalMatrices[inPtId*9]));
        gh->RenderPieceDraw(ren, actor);
        }
      if (primed)
        {
        gh->RenderPieceFinish(ren, actor);
        }
      }
    }

  vtkOpenGLCheckErrorMacro("failed after Render");
}



// ---------------------------------------------------------------------------
void vtkOpenGLGlyph3DMapper::RebuildStructures(
  vtkOpenGLGlyph3DMapper::vtkOpenGLGlyph3DMapperSubArray *subarray,
  vtkIdType numPts,
  vtkActor* actor,
  vtkDataSet* dataset,
  vtkBitArray *maskArray,
  bool selecting_points)
{
  double den = this->Range[1] - this->Range[0];
  if (den == 0.0)
    {
    den = 1.0;
    }

  vtkDataArray* orientArray = this->GetOrientationArray(dataset);
  if (orientArray !=0 && orientArray->GetNumberOfComponents() != 3)
    {
    vtkErrorMacro(" expecting an orientation array with 3 component, getting "
      << orientArray->GetNumberOfComponents() << " components.");
    return;
    }

  double arrayVals[16];
  vtkTransform *trans = vtkTransform::New();
  vtkTransform *normalTrans = vtkTransform::New();
  vtkDataArray* indexArray = this->GetSourceIndexArray(dataset);
  vtkDataArray* scaleArray = this->GetScaleArray(dataset);
  vtkDataArray* selectionArray = this->GetSelectionIdArray(dataset);

  /// FIXME: Didn't handle the premultiplycolorswithalpha aspect...
  vtkUnsignedCharArray* colors = NULL;
  this->ColorMapper->SetInputDataObject(dataset);
  this->ColorMapper->MapScalars(actor->GetProperty()->GetOpacity());
  colors = ((vtkOpenGLGlyph3DMappervtkColorMapper *)this->ColorMapper)->GetColors();
  //  bool multiplyWithAlpha =
  //    (this->ScalarsToColorsPainter->GetPremultiplyColorsWithAlpha(actor) == 1);
  // Traverse all Input points, transforming Source points

  int numEntries = (int)subarray->Entries.size();

  // how many points for each source
  int *numPointsPerSource = new int [numEntries];
  if (numEntries > 1 && indexArray)
    {
    for (int i = 0; i < numEntries; i++)
      {
      numPointsPerSource[i] = 0;
      }
    // loop over every point
    int index = 0;
    for (vtkIdType inPtId = 0; inPtId < numPts; inPtId++)
      {
      if (maskArray && maskArray->GetValue(inPtId) == 0)
        {
        continue;
        }

      // Compute index into table of glyphs
      if (indexArray)
        {
        double value = vtkMath::Norm(indexArray->GetTuple(inPtId),
          indexArray->GetNumberOfComponents());
        index = static_cast<int>((value-this->Range[0])*numEntries/den);
        index = ::vtkClamp(index, 0, numEntries-1);
        }
      numPointsPerSource[index]++;
      }
    }
  else
    {
    numPointsPerSource[0] = numPts;
    }

  // for each entry start with a reasonable allocation
  for (size_t cc = 0; cc < subarray->Entries.size(); cc++)
    {
    vtkOpenGLGlyph3DMapper::vtkOpenGLGlyph3DMapperEntry *entry =
      subarray->Entries[cc];
    entry->PickIds.resize(numPointsPerSource[cc]);
    entry->Colors.resize(numPointsPerSource[cc]*4);
    entry->Matrices.resize(numPointsPerSource[cc]*16);
    entry->NormalMatrices.resize(numPointsPerSource[cc]*9);
    entry->NumberOfPoints = 0;
    entry->BuildTime.Modified();
    }
  delete [] numPointsPerSource;

  // loop over every point and fill structures
  int index = 0;
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

    // Compute index into table of glyphs
    if (indexArray)
      {
      double value = vtkMath::Norm(indexArray->GetTuple(inPtId),
        indexArray->GetNumberOfComponents());
      index = static_cast<int>((value-this->Range[0])*numEntries/den);
      index = ::vtkClamp(index, 0, numEntries-1);
      }

    // source can be null.
    vtkPolyData *source = this->GetSource(index);

    // Make sure we're not indexing into empty glyph
    if (source)
      {
      vtkOpenGLGlyph3DMapper::vtkOpenGLGlyph3DMapperEntry *entry =
        subarray->Entries[index];

      entry->Colors[entry->NumberOfPoints*4] = 255;
      entry->Colors[entry->NumberOfPoints*4+1] = 255;
      entry->Colors[entry->NumberOfPoints*4+2] = 255;
      entry->Colors[entry->NumberOfPoints*4+3] = 255;

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
      normalTrans->Identity();

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
          normalTrans->RotateZ(orientation[2]);
          normalTrans->RotateX(orientation[0]);
          normalTrans->RotateY(orientation[1]);
          break;

        case DIRECTION:
          if (orientation[1] == 0.0 && orientation[2] == 0.0)
            {
            if (orientation[0] < 0) //just flip x if we need to
              {
              trans->RotateWXYZ(180.0, 0, 1, 0);
              normalTrans->RotateWXYZ(180.0, 0, 1, 0);
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
            normalTrans->RotateWXYZ(180.0, vNew[0], vNew[1], vNew[2]);
            }
          break;
          }
        }

      // Set pickid
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
      entry->PickIds[entry->NumberOfPoints] = selectionId;

      if (colors)
        {
        colors->GetTupleValue(inPtId, &(entry->Colors[entry->NumberOfPoints*4]));
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
        normalTrans->Scale(scalex, scaley, scalez);
        }

      vtkMatrix4x4::DeepCopy(arrayVals, trans->GetMatrix());
      for (int i = 0; i < 4; i++)
        {
        for (int j = 0; j < 4; j++)
          {
          entry->Matrices[entry->NumberOfPoints*16+i*4+j] = arrayVals[j*4+i];
          }
        }
      normalTrans->Inverse();
      vtkMatrix4x4::DeepCopy(arrayVals, normalTrans->GetMatrix());
      for (int i = 0; i < 3; i++)
        {
        for (int j = 0; j < 3; j++)
          {
          entry->NormalMatrices[entry->NumberOfPoints*9+i*3+j] = arrayVals[i*4+j];
          }
        }
      entry->NumberOfPoints++;
      }
    }

  subarray->LastSelectingState = selecting_points;
  subarray->BuildTime.Modified();
  trans->Delete();
  normalTrans->Delete();
}


// ---------------------------------------------------------------------------
// Description:
// Release any graphics resources that are being consumed by this mapper.
void vtkOpenGLGlyph3DMapper::ReleaseGraphicsResources(vtkWindow *window)
{
  std::map<const vtkDataSet *, vtkOpenGLGlyph3DMapper::vtkOpenGLGlyph3DMapperSubArray *>::iterator miter = this->GlyphValues->Entries.begin();
  for (;miter != this->GlyphValues->Entries.end(); miter++)
    {
    std::map<size_t, vtkOpenGLGlyph3DMapper::vtkOpenGLGlyph3DMapperEntry *>::iterator miter2 = miter->second->Entries.begin();
    for (;miter2 != miter->second->Entries.end(); miter2++)
      {
      miter2->second->Mapper->ReleaseGraphicsResources(window);
      }
    }
}

// ----------------------------------------------------------------------------
void vtkOpenGLGlyph3DMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

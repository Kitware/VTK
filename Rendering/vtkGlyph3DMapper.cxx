/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGlyph3DMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGlyph3DMapper.h"

#include "vtkActor.h"
#include "vtkBitArray.h"
#include "vtkBoundingBox.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkGraphicsFactory.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLookupTable.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTimerLog.h"
#include "vtkTransform.h"

#include <assert.h>
#include <vector>

vtkInstantiatorNewMacro(vtkGlyph3DMapper);

template <class T>
static T vtkClamp(T val, T min, T max)
{
  val = val < min? min : val;
  val = val > max? max : val;
  return val;
}
//----------------------------------------------------------------------------
// return the correct type of vtkGlyph3DMapper 
vtkGlyph3DMapper *vtkGlyph3DMapper::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkGraphicsFactory::CreateInstance("vtkGlyph3DMapper");
  return static_cast<vtkGlyph3DMapper *>(ret);
}

// ---------------------------------------------------------------------------
// Construct object with scaling on, scaling mode is by scalar value,
// scale factor = 1.0, the range is (0,1), orient geometry is on, and
// orientation is by vector. Clamping and indexing are turned off. No
// initial sources are defined.
vtkGlyph3DMapper::vtkGlyph3DMapper()
{
  this->SetNumberOfInputPorts(2);

  this->Scaling = true;
  this->ScaleMode = SCALE_BY_MAGNITUDE;
  this->ScaleFactor = 1.0;
  this->Range[0] = 0.0;
  this->Range[1] = 1.0;
  this->Orient = true;
  this->Clamping = false;
  this->SourceIndexing = false;
  this->UseSelectionIds = false;
  this->OrientationMode = vtkGlyph3DMapper::DIRECTION;

  // Set default arrays.
  this->SetScaleArray(vtkDataSetAttributes::SCALARS);
  this->SetMaskArray(vtkDataSetAttributes::SCALARS);
  this->SetOrientationArray(vtkDataSetAttributes::VECTORS);
  this->SetSelectionIdArray(vtkDataSetAttributes::SCALARS);

  this->NestedDisplayLists = true;

  this->Masking = false;
  this->SelectMode=1;
  this->SelectionColorId=1;
}

// ---------------------------------------------------------------------------
vtkGlyph3DMapper::~vtkGlyph3DMapper()
{
}

// ---------------------------------------------------------------------------
void vtkGlyph3DMapper::SetMaskArray(int fieldAttributeType)
{
  this->SetInputArrayToProcess(vtkGlyph3DMapper::MASK, 0, 0,
    vtkDataObject::FIELD_ASSOCIATION_POINTS, fieldAttributeType);
}

// ---------------------------------------------------------------------------
void vtkGlyph3DMapper::SetMaskArray(const char* maskarrayname)
{
  this->SetInputArrayToProcess(vtkGlyph3DMapper::MASK, 0, 0,
    vtkDataObject::FIELD_ASSOCIATION_POINTS, maskarrayname);
}

// ---------------------------------------------------------------------------
vtkDataArray* vtkGlyph3DMapper::GetMaskArray(vtkDataSet* input)
{
  if (this->Masking)
    {
    int association = vtkDataObject::FIELD_ASSOCIATION_POINTS;
    return this->GetInputArrayToProcess(vtkGlyph3DMapper::MASK,
      input, association);
    }
  return 0;
}

// ---------------------------------------------------------------------------
void vtkGlyph3DMapper::SetOrientationArray(const char* orientationarrayname)
{
  this->SetInputArrayToProcess(vtkGlyph3DMapper::ORIENTATION, 0, 0,
    vtkDataObject::FIELD_ASSOCIATION_POINTS, orientationarrayname);
}

// ---------------------------------------------------------------------------
void vtkGlyph3DMapper::SetOrientationArray(int fieldAttributeType)
{
  this->SetInputArrayToProcess(vtkGlyph3DMapper::ORIENTATION, 0, 0,
    vtkDataObject::FIELD_ASSOCIATION_POINTS, fieldAttributeType);
}

// ---------------------------------------------------------------------------
vtkDataArray* vtkGlyph3DMapper::GetOrientationArray(vtkDataSet* input)
{
  if (this->Orient)
    {
    int association = vtkDataObject::FIELD_ASSOCIATION_POINTS;
    return this->GetInputArrayToProcess(vtkGlyph3DMapper::ORIENTATION,
      input, association);
    }
  return NULL;
}

// ---------------------------------------------------------------------------
void vtkGlyph3DMapper::SetScaleArray(const char* scalarsarrayname)
{
  this->SetInputArrayToProcess(vtkGlyph3DMapper::SCALE, 0, 0,
    vtkDataObject::FIELD_ASSOCIATION_POINTS, scalarsarrayname);
}

// ---------------------------------------------------------------------------
void vtkGlyph3DMapper::SetScaleArray(int fieldAttributeType)
{
  this->SetInputArrayToProcess(vtkGlyph3DMapper::SCALE, 0, 0,
    vtkDataObject::FIELD_ASSOCIATION_POINTS, fieldAttributeType);
}

// ---------------------------------------------------------------------------
vtkDataArray* vtkGlyph3DMapper::GetScaleArray(vtkDataSet* input)
{
  if (this->Scaling && this->ScaleMode != vtkGlyph3DMapper::NO_DATA_SCALING)
    {
    int association = vtkDataObject::FIELD_ASSOCIATION_POINTS;
    vtkDataArray* arr = this->GetInputArrayToProcess(vtkGlyph3DMapper::SCALE,
      input, association);
    return arr;
    }
  return 0;
}

// ---------------------------------------------------------------------------
void vtkGlyph3DMapper::SetSourceIndexArray(const char* arrayname)
{
  this->SetInputArrayToProcess(vtkGlyph3DMapper::SOURCE_INDEX, 0, 0,
    vtkDataObject::FIELD_ASSOCIATION_POINTS, arrayname);
}

// ---------------------------------------------------------------------------
void vtkGlyph3DMapper::SetSourceIndexArray(int fieldAttributeType)
{
  this->SetInputArrayToProcess(vtkGlyph3DMapper::SOURCE_INDEX, 0, 0,
    vtkDataObject::FIELD_ASSOCIATION_POINTS, fieldAttributeType);
}

// ---------------------------------------------------------------------------
vtkDataArray* vtkGlyph3DMapper::GetSourceIndexArray(vtkDataSet* input)
{
  if (this->SourceIndexing)
    {
    int association = vtkDataObject::FIELD_ASSOCIATION_POINTS;
    return this->GetInputArrayToProcess(
      vtkGlyph3DMapper::SOURCE_INDEX, input, association);
    }
  return 0;
}

// ---------------------------------------------------------------------------
void vtkGlyph3DMapper::SetSelectionIdArray(const char* selectionIdArrayName)
{
  this->SetInputArrayToProcess(vtkGlyph3DMapper::SELECTIONID, 0, 0,
    vtkDataObject::FIELD_ASSOCIATION_POINTS, selectionIdArrayName);
}

// ---------------------------------------------------------------------------
void vtkGlyph3DMapper::SetSelectionIdArray(int fieldAttributeType)
{
  this->SetInputArrayToProcess(vtkGlyph3DMapper::SELECTIONID, 0, 0,
    vtkDataObject::FIELD_ASSOCIATION_POINTS, fieldAttributeType);
}

// ---------------------------------------------------------------------------
vtkDataArray* vtkGlyph3DMapper::GetSelectionIdArray(vtkDataSet* input)
{
  if (this->UseSelectionIds)
    {
    int association = vtkDataObject::FIELD_ASSOCIATION_POINTS;
    vtkDataArray* arr = this->GetInputArrayToProcess(
          vtkGlyph3DMapper::SELECTIONID, input, association);
    return arr;
    }
  return NULL;
}

// ---------------------------------------------------------------------------
vtkUnsignedCharArray* vtkGlyph3DMapper::GetColors(vtkDataSet* input)
{
  return vtkUnsignedCharArray::SafeDownCast(
    input->GetPointData()->GetScalars());
}

// ---------------------------------------------------------------------------
// Specify a source object at a specified table location.
void vtkGlyph3DMapper::SetSourceConnection(int idx,
  vtkAlgorithmOutput *algOutput)
{
  if (idx < 0)
    {
    vtkErrorMacro("Bad index " << idx << " for source.");
    return;
    }

  int numConnections = this->GetNumberOfInputConnections(1);
  if (idx < numConnections)
    {
    this->SetNthInputConnection(1, idx, algOutput);
    }
  else if (idx == numConnections && algOutput)
    {
    this->AddInputConnection(1, algOutput);
    }
  else if (algOutput)
    {
    vtkWarningMacro("The source id provided is larger than the maximum "
      "source id, using " << numConnections << " instead.");
    this->AddInputConnection(1, algOutput);
    }
}

// ---------------------------------------------------------------------------
// Specify a source object at a specified table location.
void vtkGlyph3DMapper::SetSource(int idx, vtkPolyData *pd)
{
  if (idx < 0)
    {
    vtkErrorMacro("Bad index " << idx << " for source.");
    return;
    }

  int numConnections = this->GetNumberOfInputConnections(1);
  vtkAlgorithmOutput *algOutput = 0;
  if (pd)
    {
    algOutput = pd->GetProducerPort();
    }
  else
    {
    vtkErrorMacro("Cannot set NULL source.");
    return;
    }

  if (idx < numConnections)
    {
    if (algOutput)
      {
      this->SetNthInputConnection(1, idx, algOutput);
      }
    }
  else if (idx == numConnections && algOutput)
    {
    this->AddInputConnection(1, algOutput);
    }
}

// ---------------------------------------------------------------------------
// Description:
// Set the source to use for he glyph. Old style. See SetSourceConnection.
void vtkGlyph3DMapper::SetSource(vtkPolyData *pd)
{
  this->SetSource(0,pd);
}

// ---------------------------------------------------------------------------
// Get a pointer to a source object at a specified table location.
vtkPolyData *vtkGlyph3DMapper::GetSource(int idx)
{
  if ( idx < 0 || idx >= this->GetNumberOfInputConnections(1) )
    {
    return NULL;
    }

  return vtkPolyData::SafeDownCast(
    this->GetExecutive()->GetInputData(1, idx));
}

// ---------------------------------------------------------------------------
vtkPolyData *vtkGlyph3DMapper::GetSource(int idx,
  vtkInformationVector *sourceInfo)
{
  vtkInformation *info = sourceInfo->GetInformationObject(idx);
  if (!info)
    {
    return NULL;
    }
  return vtkPolyData::SafeDownCast(info->Get(vtkDataObject::DATA_OBJECT()));
}

// ---------------------------------------------------------------------------
const char* vtkGlyph3DMapper::GetOrientationModeAsString()
{
  switch (this->OrientationMode)
    {
  case vtkGlyph3DMapper::DIRECTION:
    return "Direction";
  case vtkGlyph3DMapper::ORIENTATION:
    return "Orientation";
    }
  return "Invalid";
}

// ---------------------------------------------------------------------------
void vtkGlyph3DMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if ( this->GetNumberOfInputConnections(1) < 2 )
    {
    if ( this->GetSource(0) != NULL )
      {
      os << indent << "Source: (" << this->GetSource(0) << ")\n";
      }
    else
      {
      os << indent << "Source: (none)\n";
      }
    }
  else
    {
    os << indent << "A table of " << this->GetNumberOfInputConnections(1)
      << " glyphs has been defined\n";
    }

  os << indent << "Scaling: " << (this->Scaling ? "On\n" : "Off\n");

  os << indent << "Scale Mode: " << this->GetScaleModeAsString() << endl;
  os << indent << "Scale Factor: " << this->ScaleFactor << "\n";
  os << indent << "Clamping: " << (this->Clamping ? "On\n" : "Off\n");
  os << indent << "Range: (" << this->Range[0] << ", " << this->Range[1] << ")\n";
  os << indent << "Orient: " << (this->Orient ? "On\n" : "Off\n");
  os << indent << "OrientationMode: "
    << this->GetOrientationModeAsString() << "\n";
  os << indent << "SourceIndexing: "
    << (this->SourceIndexing? "On" : "Off") << endl;
  os << indent << "UseSelectionIds: "
     << (this->UseSelectionIds? "On" : "Off") << endl;
  os << indent << "SelectMode: " << this->SelectMode << endl;
  os << indent << "SelectionColorId: " << this->SelectionColorId << endl;
  os << "Masking: " << (this->Masking? "On" : "Off") << endl;
  os << "NestedDisplayLists: " << (this->NestedDisplayLists? "On" : "Off") << endl;
}

// ---------------------------------------------------------------------------
  int vtkGlyph3DMapper::RequestUpdateExtent(
    vtkInformation *vtkNotUsed(request),
    vtkInformationVector **inputVector,
    vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *sourceInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  if (sourceInfo)
    {
    sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
      0);
    sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
      1);
    sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
      0);
    }
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()));
  inInfo->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);

  return 1;
}

// ---------------------------------------------------------------------------
int vtkGlyph3DMapper::FillInputPortInformation(int port,
  vtkInformation *info)
{
  if (port == 0)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
    return 1;
    }
  else if (port == 1)
    {
    info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
    return 1;
    }
  return 0;
}

// ---------------------------------------------------------------------------
// Description:
// Return the method of scaling as a descriptive character string.
const char *vtkGlyph3DMapper::GetScaleModeAsString(void)
{
  if ( this->ScaleMode == SCALE_BY_MAGNITUDE)
    {
    return "ScaleByMagnitude";
    }
  else if ( this->ScaleMode == SCALE_BY_COMPONENTS)
    {
    return "ScaleByVectorComponents";
    }

  return "NoDataScaling";
}

//-------------------------------------------------------------------------
bool vtkGlyph3DMapper::GetBoundsInternal(vtkDataSet* ds, double ds_bounds[6])
{
  if (ds == NULL)
    {
    return false;
    }

  ds->GetBounds(ds_bounds);

  // if the input is not conform to what the mapper expects (use vector
  // but no vector data), nothing will be mapped.
  // It make sense to return uninitialized bounds.

  vtkDataArray *scaleArray = this->GetScaleArray(ds);
  vtkDataArray *orientArray = this->GetOrientationArray(ds);
  // TODO:
  // 1. cumulative bbox of all the glyph
  // 2. scale it by scale factor and maximum scalar value (or vector mag)
  // 3. enlarge the input bbox half-way in each direction with the
  // glyphs bbox.


  double den=this->Range[1]-this->Range[0];
  if(den==0.0)
    {
    den=1.0;
    }

  if(this->GetSource(0)==0)
    {
    vtkPolyData *defaultSource = vtkPolyData::New();
    defaultSource->Allocate();
    vtkPoints *defaultPoints = vtkPoints::New();
    defaultPoints->Allocate(6);
    defaultPoints->InsertNextPoint(0, 0, 0);
    defaultPoints->InsertNextPoint(1, 0, 0);
    vtkIdType defaultPointIds[2];
    defaultPointIds[0] = 0;
    defaultPointIds[1] = 1;
    defaultSource->SetPoints(defaultPoints);
    defaultSource->InsertNextCell(VTK_LINE, 2, defaultPointIds);
    defaultSource->SetUpdateExtent(0, 1, 0);
    this->SetSource(defaultSource);
    defaultSource->Delete();
    defaultSource = NULL;
    defaultPoints->Delete();
    defaultPoints = NULL;
    }

  // FB

  // Compute indexRange.
  int indexRange[2] = {0, 0};
  int numberOfSources=this->GetNumberOfInputConnections(1);
  vtkDataArray *indexArray = this->GetSourceIndexArray(ds);
  if (indexArray)
    {
    double range[2];
    indexArray->GetRange(range, -1);
    for (int i=0; i<2; i++)
      {
      indexRange[i]=static_cast<int>((range[i]-this->Range[0])*numberOfSources/den);
      indexRange[i] = ::vtkClamp(indexRange[i], 0, numberOfSources-1);
      }
    }

  vtkBoundingBox bbox; // empty

  double xScaleRange[2] = {1.0, 1.0};
  double yScaleRange[2] = {1.0, 1.0};
  double zScaleRange[2] = {1.0, 1.0};

  if (scaleArray)
    {
    switch(this->ScaleMode)
      {
    case SCALE_BY_MAGNITUDE:
      scaleArray->GetRange(xScaleRange,-1);
      yScaleRange[0]=xScaleRange[0];
      yScaleRange[1]=xScaleRange[1];
      zScaleRange[0]=xScaleRange[0];
      zScaleRange[1]=xScaleRange[1];
      break;

    case SCALE_BY_COMPONENTS:
      scaleArray->GetRange(xScaleRange,0);
      scaleArray->GetRange(yScaleRange,1);
      scaleArray->GetRange(zScaleRange,2);
      break;

    default:
      // NO_DATA_SCALING: do nothing, set variables to avoid warnings.
      break;
      }

    if (this->Clamping && this->ScaleMode != NO_DATA_SCALING)
      {
      xScaleRange[0]=vtkMath::ClampAndNormalizeValue(xScaleRange[0],
        this->Range);
      xScaleRange[1]=vtkMath::ClampAndNormalizeValue(xScaleRange[1],
        this->Range);
      yScaleRange[0]=vtkMath::ClampAndNormalizeValue(yScaleRange[0],
        this->Range);
      yScaleRange[1]=vtkMath::ClampAndNormalizeValue(yScaleRange[1],
        this->Range);
      zScaleRange[0]=vtkMath::ClampAndNormalizeValue(zScaleRange[0],
        this->Range);
      zScaleRange[1]=vtkMath::ClampAndNormalizeValue(zScaleRange[1],
        this->Range);
      }
    }
  int index=indexRange[0];
  while(index<=indexRange[1])
    {
    vtkPolyData *source=this->GetSource(index);
    // Make sure we're not indexing into empty glyph
    if(source!=0)
      {
      double bounds[6];
      source->GetBounds(bounds);// can be invalid/uninitialized
      if(vtkMath::AreBoundsInitialized(bounds))
        {
        bbox.AddBounds(bounds);
        }
      }
    ++index;
    }

  if(this->Scaling)
    {
    vtkBoundingBox bbox2(bbox);
    bbox.Scale(xScaleRange[0],yScaleRange[0],zScaleRange[0]);
    bbox2.Scale(xScaleRange[1],yScaleRange[1],zScaleRange[1]);
    bbox.AddBox(bbox2);
    bbox.Scale(this->ScaleFactor,this->ScaleFactor,this->ScaleFactor);
    }

  if(bbox.IsValid())
    {
    double bounds[6];
    if (orientArray)
      {
      // bounding sphere.
      double c[3];
      bbox.GetCenter(c);
      double l=bbox.GetDiagonalLength()/2.0;
      bounds[0]=c[0]-l;
      bounds[1]=c[0]+l;
      bounds[2]=c[1]-l;
      bounds[3]=c[1]+l;
      bounds[4]=c[2]-l;
      bounds[5]=c[2]+l;
      }
    else
      {
      bbox.GetBounds(bounds);
      }
    int j=0;
    while(j<6)
      {
      ds_bounds[j]+=bounds[j];
      ++j;
      }
    }
  else
    {
    return false;
    }

  return true;
}

//-------------------------------------------------------------------------
double* vtkGlyph3DMapper::GetBounds()
{
  //  static double bounds[] = {-1.0,1.0, -1.0,1.0, -1.0,1.0};
  vtkMath::UninitializeBounds(this->Bounds);

  // do we have an input
  if ( ! this->GetNumberOfInputConnections(0) )
    {
    return this->Bounds;
    }
  if (!this->Static)
    {
    // For proper clipping, this would be this->Piece,
    // this->NumberOfPieces.
    // But that removes all benefites of streaming.
    // Update everything as a hack for paraview streaming.
    // This should not affect anything else, because no one uses this.
    // It should also render just the same.
    // Just remove this lie if we no longer need streaming in paraview :)
    //this->GetInput()->SetUpdateExtent(0, 1, 0);
    //this->GetInput()->Update();

    // first get the bounds from the input
    this->Update();
    }

  vtkDataObject* dobj = this->GetInputDataObject(0, 0);
  vtkDataSet* ds = vtkDataSet::SafeDownCast(dobj);
  if (ds)
    {
    this->GetBoundsInternal(ds, this->Bounds);
    return this->Bounds;
    }

  vtkCompositeDataSet* cd = vtkCompositeDataSet::SafeDownCast(dobj);
  if (!cd)
    {
    return this->Bounds;
    }

  vtkBoundingBox bbox;
  vtkCompositeDataIterator* iter = cd->NewIterator();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal();
    iter->GoToNextItem())
    {
    ds = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
    if (ds)
      {
      bbox.AddBounds(ds->GetBounds());
      }
    }
  bbox.GetBounds(this->Bounds);
  iter->Delete();
  return this->Bounds;

}

//-------------------------------------------------------------------------
void vtkGlyph3DMapper::GetBounds(double bounds[6])
{
  this->Superclass::GetBounds(bounds);
}

// ---------------------------------------------------------------------------
void vtkGlyph3DMapper::Render(vtkRenderer *, vtkActor *)
{
  cerr << "Calling wrong render method!!\n";
}

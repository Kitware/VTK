/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGlyph3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGlyph3D.h"

#include "vtkCellData.h"
#include "vtkCell.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTransform.h"
#include "vtkTrivialProducer.h"
#include "vtkUniformGrid.h"
#include "vtkUnsignedCharArray.h"

vtkStandardNewMacro(vtkGlyph3D);
vtkCxxSetObjectMacro(vtkGlyph3D, SourceTransform, vtkTransform);

//----------------------------------------------------------------------------
// Construct object with scaling on, scaling mode is by scalar value,
// scale factor = 1.0, the range is (0,1), orient geometry is on, and
// orientation is by vector. Clamping and indexing are turned off. No
// initial sources are defined.
vtkGlyph3D::vtkGlyph3D()
{
  this->Scaling = 1;
  this->ColorMode = VTK_COLOR_BY_SCALE;
  this->ScaleMode = VTK_SCALE_BY_SCALAR;
  this->ScaleFactor = 1.0;
  this->Range[0] = 0.0;
  this->Range[1] = 1.0;
  this->Orient = 1;
  this->VectorMode = VTK_USE_VECTOR;
  this->Clamping = 0;
  this->IndexMode = VTK_INDEXING_OFF;
  this->GeneratePointIds = 0;
  this->PointIdsName = NULL;
  this->SetPointIdsName("InputPointIds");
  this->SetNumberOfInputPorts(2);
  this->FillCellData = 0;
  this->SourceTransform = 0;

  // by default process active point scalars
  this->SetInputArrayToProcess(0,0,0,vtkDataObject::FIELD_ASSOCIATION_POINTS,
                               vtkDataSetAttributes::SCALARS);
  // by default process active point vectors
  this->SetInputArrayToProcess(1,0,0,vtkDataObject::FIELD_ASSOCIATION_POINTS,
                               vtkDataSetAttributes::VECTORS);
  // by default process active point normals
  this->SetInputArrayToProcess(2,0,0,vtkDataObject::FIELD_ASSOCIATION_POINTS,
                               vtkDataSetAttributes::NORMALS);
  // by default process active point scalars
  this->SetInputArrayToProcess(3,0,0,vtkDataObject::FIELD_ASSOCIATION_POINTS,
                               vtkDataSetAttributes::SCALARS);
}

//----------------------------------------------------------------------------
vtkGlyph3D::~vtkGlyph3D()
{
  delete [] PointIdsName;
  this->SetSourceTransform(NULL);
}

//----------------------------------------------------------------------------
vtkMTimeType vtkGlyph3D::GetMTime()
{
  vtkMTimeType mTime=this->Superclass::GetMTime();
  vtkMTimeType time;
  if ( this->SourceTransform != NULL )
  {
    time = this->SourceTransform ->GetMTime();
    mTime = ( time > mTime ? time : mTime );
  }
  return mTime;
}

//----------------------------------------------------------------------------
int vtkGlyph3D::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkDataSet* input = vtkDataSet::GetData(inputVector[0], 0);
  vtkPolyData* output = vtkPolyData::GetData(outputVector, 0);

  return this->Execute(input, inputVector[1], output)? 1 : 0;
}

//----------------------------------------------------------------------------
bool vtkGlyph3D::Execute(
  vtkDataSet* input,
  vtkInformationVector* sourceVector,
  vtkPolyData* output)
{
  vtkDataArray *inSScalars = this->GetInputArrayToProcess(0, input);
  vtkDataArray *inVectors = this->GetInputArrayToProcess(1, input);
  return this->Execute(input, sourceVector, output, inSScalars, inVectors);
}

//----------------------------------------------------------------------------
bool vtkGlyph3D::Execute(
  vtkDataSet* input,
  vtkInformationVector* sourceVector,
  vtkPolyData* output,
  vtkDataArray *inSScalars,
  vtkDataArray *inVectors)
{
  assert(input && output);
  if (input == NULL || output == NULL)
  {
    // nothing to do.
    return true;
  }

  // this is used to respect blanking specified on uniform grids.
  vtkUniformGrid* inputUG = vtkUniformGrid::SafeDownCast(input);

  vtkPointData *pd;
  vtkDataArray *inCScalars; // Scalars for Coloring
  unsigned char* inGhostLevels=0;
  vtkDataArray *inNormals, *sourceNormals = NULL;
  vtkDataArray *sourceTCoords = NULL;
  vtkIdType numPts, numSourcePts, numSourceCells, inPtId, i;
  vtkPoints *sourcePts = NULL;
  vtkSmartPointer<vtkPoints> transformedSourcePts = vtkSmartPointer<vtkPoints>::New();
  vtkPoints *newPts;
  vtkDataArray *newScalars=NULL;
  vtkDataArray *newVectors=NULL;
  vtkDataArray *newNormals=NULL;
  vtkDataArray *newTCoords = NULL;
  double x[3], v[3], vNew[3], s = 0.0, vMag = 0.0, value, tc[3];
  vtkTransform *trans = vtkTransform::New();
  vtkNew<vtkIdList> pointIdList;
  vtkIdList *cellPts;
  int npts;
  vtkIdList *pts;
  vtkIdType ptIncr, cellIncr, cellId;
  int haveVectors, haveNormals, haveTCoords = 0;
  double scalex,scaley,scalez, den;
  vtkPointData* outputPD = output->GetPointData();
  vtkCellData* outputCD = output->GetCellData();
  int numberOfSources = this->GetNumberOfInputConnections(1);
  vtkPolyData *defaultSource = NULL;
  vtkIdTypeArray *pointIds=0;
  vtkPolyData *source = this->GetSource(0, sourceVector);
  vtkNew<vtkIdList> srcPointIdList;
  vtkNew<vtkIdList> dstPointIdList;
  vtkNew<vtkIdList> srcCellIdList;
  vtkNew<vtkIdList> dstCellIdList;

  vtkDebugMacro(<<"Generating glyphs");

  pts = vtkIdList::New();
  pts->Allocate(VTK_CELL_SIZE);

  pd = input->GetPointData();
  inNormals = this->GetInputArrayToProcess(2, input);
  inCScalars = this->GetInputArrayToProcess(3, input);
  if (inCScalars == NULL)
  {
    inCScalars = inSScalars;
  }

  vtkDataArray* temp = 0;
  if (pd)
  {
    temp = pd->GetArray(vtkDataSetAttributes::GhostArrayName());
  }
  if ( (!temp) || (temp->GetDataType() != VTK_UNSIGNED_CHAR)
    || (temp->GetNumberOfComponents() != 1))
  {
    vtkDebugMacro("No appropriate ghost levels field available.");
  }
  else
  {
    inGhostLevels =static_cast<vtkUnsignedCharArray *>(temp)->GetPointer(0);
  }


  numPts = input->GetNumberOfPoints();
  if (numPts < 1)
  {
    vtkDebugMacro(<<"No points to glyph!");
    pts->Delete();
    trans->Delete();
    return 1;
  }

  // Check input for consistency
  //
  if ( (den = this->Range[1] - this->Range[0]) == 0.0 )
  {
    den = 1.0;
  }
  if ( this->VectorMode != VTK_VECTOR_ROTATION_OFF &&
       ((this->VectorMode == VTK_USE_VECTOR && inVectors != NULL) ||
        (this->VectorMode == VTK_USE_NORMAL && inNormals != NULL)) )
  {
    haveVectors = 1;
  }
  else
  {
    haveVectors = 0;
  }

  if ( (this->IndexMode == VTK_INDEXING_BY_SCALAR && !inSScalars) ||
       (this->IndexMode == VTK_INDEXING_BY_VECTOR &&
       ((!inVectors && this->VectorMode == VTK_USE_VECTOR) ||
        (!inNormals && this->VectorMode == VTK_USE_NORMAL))) )
  {
    if ( !source )
    {
      vtkErrorMacro(<<"Indexing on but don't have data to index with");
      pts->Delete();
      trans->Delete();
      return true;
    }
    else
    {
      vtkWarningMacro(<<"Turning indexing off: no data to index with");
      this->IndexMode = VTK_INDEXING_OFF;
    }
  }

  // Allocate storage for output PolyData
  //
  outputPD->CopyVectorsOff();
  outputPD->CopyNormalsOff();
  outputPD->CopyTCoordsOff();

  if (!source)
  {
    defaultSource = vtkPolyData::New();
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
    defaultSource->Delete();
    defaultSource = NULL;
    defaultPoints->Delete();
    defaultPoints = NULL;
    source = defaultSource;
  }

  if ( this->IndexMode != VTK_INDEXING_OFF )
  {
    pd = NULL;
    haveNormals = 1;
    for (numSourcePts=numSourceCells=i=0; i < numberOfSources; i++)
    {
      source = this->GetSource(i, sourceVector);
      if ( source != NULL )
      {
        if (source->GetNumberOfPoints() > numSourcePts)
        {
          numSourcePts = source->GetNumberOfPoints();
        }
        if (source->GetNumberOfCells() > numSourceCells)
        {
          numSourceCells = source->GetNumberOfCells();
        }
        if ( !(sourceNormals = source->GetPointData()->GetNormals()) )
        {
          haveNormals = 0;
        }
      }
    }
  }
  else
  {
    sourcePts = source->GetPoints();
    numSourcePts = sourcePts->GetNumberOfPoints();
    numSourceCells = source->GetNumberOfCells();

    sourceNormals = source->GetPointData()->GetNormals();
    if ( sourceNormals )
    {
      haveNormals = 1;
    }
    else
    {
      haveNormals = 0;
    }

    sourceTCoords = source->GetPointData()->GetTCoords();
    if (sourceTCoords)
    {
      haveTCoords = 1;
    }
    else
    {
      haveTCoords = 0;
    }

    // Prepare to copy output.
    pd = input->GetPointData();
    outputPD->CopyAllocate(pd,numPts*numSourcePts);
    if (this->FillCellData)
    {
      outputCD->CopyAllocate(pd,numPts*numSourceCells);
    }
  }

  srcPointIdList->SetNumberOfIds(numSourcePts);
  dstPointIdList->SetNumberOfIds(numSourcePts);
  srcCellIdList->SetNumberOfIds(numSourceCells);
  dstCellIdList->SetNumberOfIds(numSourceCells);

  newPts = vtkPoints::New();
  newPts->Allocate(numPts*numSourcePts);
  if ( this->GeneratePointIds )
  {
    pointIds = vtkIdTypeArray::New();
    pointIds->SetName(this->PointIdsName);
    pointIds->Allocate(numPts*numSourcePts);
    outputPD->AddArray(pointIds);
    pointIds->Delete();
  }
  if ( this->ColorMode == VTK_COLOR_BY_SCALAR && inCScalars )
  {
    newScalars = inCScalars->NewInstance();
    newScalars->SetNumberOfComponents(inCScalars->GetNumberOfComponents());
    newScalars->Allocate(inCScalars->GetNumberOfComponents()*numPts*numSourcePts);
    newScalars->SetName(inCScalars->GetName());
  }
  else if ( (this->ColorMode == VTK_COLOR_BY_SCALE) && inSScalars)
  {
    newScalars = vtkFloatArray::New();
    newScalars->Allocate(numPts*numSourcePts);
    newScalars->SetName("GlyphScale");
    if (this->ScaleMode == VTK_SCALE_BY_SCALAR)
    {
      newScalars->SetName(inSScalars->GetName());
    }
  }
  else if ( (this->ColorMode == VTK_COLOR_BY_VECTOR) && haveVectors)
  {
    newScalars = vtkFloatArray::New();
    newScalars->Allocate(numPts*numSourcePts);
    newScalars->SetName("VectorMagnitude");
  }
  if ( haveVectors )
  {
    newVectors = vtkFloatArray::New();
    newVectors->SetNumberOfComponents(3);
    newVectors->Allocate(3*numPts*numSourcePts);
    newVectors->SetName("GlyphVector");
  }
  if ( haveNormals )
  {
    newNormals = vtkFloatArray::New();
    newNormals->SetNumberOfComponents(3);
    newNormals->Allocate(3*numPts*numSourcePts);
    newNormals->SetName("Normals");
  }
  if (haveTCoords)
  {
    newTCoords = vtkFloatArray::New();
    int numComps = sourceTCoords->GetNumberOfComponents();
    newTCoords->SetNumberOfComponents(numComps);
    newTCoords->Allocate(numComps*numPts*numSourcePts);
    newTCoords->SetName("TCoords");
  }

  // Setting up for calls to PolyData::InsertNextCell()
  if (this->IndexMode != VTK_INDEXING_OFF )
  {
    output->Allocate(3*numPts*numSourceCells,numPts*numSourceCells);
  }
  else
  {
    output->Allocate(source,
                     3*numPts*numSourceCells, numPts*numSourceCells);
  }

  transformedSourcePts->SetDataTypeToDouble();
  transformedSourcePts->Allocate(numSourcePts);

  // Traverse all Input points, transforming Source points and copying
  // point attributes.
  //
  ptIncr=0;
  cellIncr=0;
  for (inPtId=0; inPtId < numPts; inPtId++)
  {
    scalex = scaley = scalez = 1.0;
    if ( ! (inPtId % 10000) )
    {
      this->UpdateProgress(static_cast<double>(inPtId)/numPts);
      if (this->GetAbortExecute())
      {
        break;
      }
    }

    // Get the scalar and vector data
    if ( inSScalars )
    {
      s = inSScalars->GetComponent(inPtId, 0);
      if ( this->ScaleMode == VTK_SCALE_BY_SCALAR ||
           this->ScaleMode == VTK_DATA_SCALING_OFF )
      {
        scalex = scaley = scalez = s;
      }
    }

    if ( haveVectors )
    {
      vtkDataArray *array3D = this->VectorMode == VTK_USE_NORMAL? inNormals : inVectors;
      if(array3D->GetNumberOfComponents()>3)
      {
        vtkErrorMacro(<<"vtkDataArray "<<array3D->GetName()<<" has more than 3 components.\n");
        pts->Delete();
        trans->Delete();
        if(newPts)
        {
          newPts->Delete();
        }
        if(newVectors)
        {
          newVectors->Delete();
        }
        return false;
      }

      v[0] = 0;
      v[1] = 0;
      v[2] = 0;
      array3D->GetTuple(inPtId, v);
      vMag = vtkMath::Norm(v);
      if ( this->ScaleMode == VTK_SCALE_BY_VECTORCOMPONENTS )
      {
        scalex = v[0];
        scaley = v[1];
        scalez = v[2];
      }
      else if ( this->ScaleMode == VTK_SCALE_BY_VECTOR )
      {
        scalex = scaley = scalez = vMag;
      }
    }

    // Clamp data scale if enabled
    if ( this->Clamping )
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

    // Compute index into table of glyphs
    if ( this->IndexMode != VTK_INDEXING_OFF )
    {
      if ( this->IndexMode == VTK_INDEXING_BY_SCALAR )
      {
        value = s;
      }
      else
      {
        value = vMag;
      }

      int index = static_cast<int>((value - this->Range[0])*numberOfSources / den);
      index = (index < 0 ? 0 :
              (index >= numberOfSources ? (numberOfSources-1) : index));

      source = this->GetSource(index, sourceVector);
      if ( source != NULL )
      {
        sourcePts = source->GetPoints();
        sourceNormals = source->GetPointData()->GetNormals();
        numSourcePts = sourcePts->GetNumberOfPoints();
        numSourceCells = source->GetNumberOfCells();
      }
    }

    // Make sure we're not indexing into empty glyph
    if ( !source )
    {
      continue;
    }

    // Check ghost points.
    // If we are processing a piece, we do not want to duplicate
    // glyphs on the borders.
    if (inGhostLevels &&
        inGhostLevels[inPtId] & vtkDataSetAttributes::DUPLICATEPOINT)
    {
      continue;
    }

    if (inputUG && !inputUG->IsPointVisible(inPtId))
    {
      // input is a vtkUniformGrid and the current point is blanked. Don't glyph
      // it.
      continue;
    }

    if (!this->IsPointVisible(input, inPtId))
    {
      continue;
    }

    // Now begin copying/transforming glyph
    trans->Identity();

    // Copy all topology (transformation independent)
    for (cellId=0; cellId < numSourceCells; cellId++)
    {
      source->GetCellPoints(cellId, pointIdList.GetPointer());
      cellPts = pointIdList.GetPointer();
      npts = cellPts->GetNumberOfIds();
      for (pts->Reset(), i=0; i < npts; i++)
      {
        pts->InsertId(i, cellPts->GetId(i) + ptIncr);
      }
      output->InsertNextCell(source->GetCellType(cellId), pts);
    }

    // translate Source to Input point
    input->GetPoint(inPtId, x);
    trans->Translate(x[0], x[1], x[2]);

    if ( haveVectors )
    {
      // Copy Input vector
      for (i=0; i < numSourcePts; i++)
      {
        newVectors->InsertTuple(i+ptIncr, v);
      }
      if (this->Orient && (vMag > 0.0))
      {
        // if there is no y or z component
        if ( v[1] == 0.0 && v[2] == 0.0 )
        {
          if (v[0] < 0) //just flip x if we need to
          {
            trans->RotateWXYZ(180.0,0,1,0);
          }
        }
        else
        {
          vNew[0] = (v[0]+vMag) / 2.0;
          vNew[1] = v[1] / 2.0;
          vNew[2] = v[2] / 2.0;
          trans->RotateWXYZ(180.0,vNew[0],vNew[1],vNew[2]);
        }
      }
    }

    if (haveTCoords)
    {
      for (i = 0; i < numSourcePts; i++)
      {
        sourceTCoords->GetTuple(i, tc);
        newTCoords->InsertTuple(i+ptIncr, tc);
      }
    }

    // determine scale factor from scalars if appropriate
    // Copy scalar value
    if (inSScalars && (this->ColorMode == VTK_COLOR_BY_SCALE))
    {
      for (i=0; i < numSourcePts; i++)
      {
        newScalars->InsertTuple(i+ptIncr, &scalex); // = scaley = scalez
      }
    }
    else if (inCScalars && (this->ColorMode == VTK_COLOR_BY_SCALAR))
    {
      for (i=0; i < numSourcePts; i++)
      {
        outputPD->CopyTuple(inCScalars, newScalars, inPtId, ptIncr+i);
      }
    }
    if (haveVectors && this->ColorMode == VTK_COLOR_BY_VECTOR)
    {
      for (i=0; i < numSourcePts; i++)
      {
        newScalars->InsertTuple(i+ptIncr, &vMag);
      }
    }

    // scale data if appropriate
    if ( this->Scaling )
    {
      if ( this->ScaleMode == VTK_DATA_SCALING_OFF )
      {
        scalex = scaley = scalez = this->ScaleFactor;
      }
      else
      {
        scalex *= this->ScaleFactor;
        scaley *= this->ScaleFactor;
        scalez *= this->ScaleFactor;
      }

      if ( scalex == 0.0 )
      {
        scalex = 1.0e-10;
      }
      if ( scaley == 0.0 )
      {
        scaley = 1.0e-10;
      }
      if ( scalez == 0.0 )
      {
        scalez = 1.0e-10;
      }
      trans->Scale(scalex,scaley,scalez);
    }

    // multiply points and normals by resulting matrix
    if (this->SourceTransform)
    {
      transformedSourcePts->Reset();
      this->SourceTransform->TransformPoints(sourcePts, transformedSourcePts);
      trans->TransformPoints(transformedSourcePts, newPts);
    }
    else
    {
      trans->TransformPoints(sourcePts,newPts);
    }

    if ( haveNormals )
    {
      trans->TransformNormals(sourceNormals,newNormals);
    }

    // Copy point data from source (if possible)
    if ( pd )
    {
      for (i = 0; i < numSourcePts; ++i)
      {
        srcPointIdList->SetId(i, inPtId);
        dstPointIdList->SetId(i, ptIncr + i);
      }
      outputPD->CopyData(pd, srcPointIdList.GetPointer(),
                         dstPointIdList.GetPointer());
      if (this->FillCellData)
      {
        for (i = 0; i < numSourceCells; ++i)
        {
          srcCellIdList->SetId(i, inPtId);
          dstCellIdList->SetId(i, cellIncr + i);
        }
        outputCD->CopyData(pd, srcCellIdList.GetPointer(),
                           dstCellIdList.GetPointer());
      }
    }

    // If point ids are to be generated, do it here
    if ( this->GeneratePointIds )
    {
      for (i=0; i < numSourcePts; i++)
      {
        pointIds->InsertNextValue(inPtId);
      }
    }

    ptIncr += numSourcePts;
    cellIncr += numSourceCells;
  }

  // Update ourselves and release memory
  //
  output->SetPoints(newPts);
  newPts->Delete();

  if (newScalars)
  {
    int idx = outputPD->AddArray(newScalars);
    outputPD->SetActiveAttribute(idx, vtkDataSetAttributes::SCALARS);
    newScalars->Delete();
  }

  if (newVectors)
  {
    outputPD->SetVectors(newVectors);
    newVectors->Delete();
  }

  if (newNormals)
  {
    outputPD->SetNormals(newNormals);
    newNormals->Delete();
  }

  if (newTCoords)
  {
    outputPD->SetTCoords(newTCoords);
    newTCoords->Delete();
  }

  output->Squeeze();
  trans->Delete();
  pts->Delete();

  return true;
}

//----------------------------------------------------------------------------
// Specify a source object at a specified table location.
void vtkGlyph3D::SetSourceConnection(int id, vtkAlgorithmOutput* algOutput)
{
  if (id < 0)
  {
    vtkErrorMacro("Bad index " << id << " for source.");
    return;
  }

  int numConnections = this->GetNumberOfInputConnections(1);
  if (id < numConnections)
  {
    this->SetNthInputConnection(1, id, algOutput);
  }
  else if (id == numConnections && algOutput)
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

//----------------------------------------------------------------------------
// Specify a source object at a specified table location.
void vtkGlyph3D::SetSourceData(int id, vtkPolyData *pd)
{
  int numConnections = this->GetNumberOfInputConnections(1);

  if (id < 0 || id > numConnections)
  {
    vtkErrorMacro("Bad index " << id << " for source.");
    return;
  }

  vtkTrivialProducer* tp = 0;
  if (pd)
  {
    tp = vtkTrivialProducer::New();
    tp->SetOutput(pd);
  }

  if (id < numConnections)
  {
    if (tp)
    {
      this->SetNthInputConnection(1, id, tp->GetOutputPort());
    }
    else
    {
      this->SetNthInputConnection(1, id, 0);
    }
  }
  else if (id == numConnections && tp)
  {
    this->AddInputConnection(1, tp->GetOutputPort());
  }

  if (tp)
  {
    tp->Delete();
  }
}

//----------------------------------------------------------------------------
// Get a pointer to a source object at a specified table location.
vtkPolyData *vtkGlyph3D::GetSource(int id)
{
  if ( id < 0 || id >= this->GetNumberOfInputConnections(1) )
  {
    return NULL;
  }

  return vtkPolyData::SafeDownCast(
    this->GetExecutive()->GetInputData(1, id));
}

//----------------------------------------------------------------------------
void vtkGlyph3D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Generate Point Ids "
     << (this->GeneratePointIds ? "On\n" : "Off\n");

  os << indent << "PointIdsName: " << (this->PointIdsName ? this->PointIdsName
       : "(none)") << "\n";

  os << indent << "Color Mode: " << this->GetColorModeAsString() << endl;

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
    os << indent << "A table of " << this->GetNumberOfInputConnections(1) << " glyphs has been defined\n";
  }

  os << indent << "Scaling: " << (this->Scaling ? "On\n" : "Off\n");

  os << indent << "Scale Mode: ";
  if ( this->ScaleMode == VTK_SCALE_BY_SCALAR )
  {
    os << "Scale by scalar\n";
  }
  else if ( this->ScaleMode == VTK_SCALE_BY_VECTOR )
  {
    os << "Scale by vector\n";
  }
  else
  {
    os << "Data scaling is turned off\n";
  }

  os << indent << "Scale Factor: " << this->ScaleFactor << "\n";
  os << indent << "Clamping: " << (this->Clamping ? "On\n" : "Off\n");
  os << indent << "Range: (" << this->Range[0] << ", " << this->Range[1] << ")\n";
  os << indent << "Orient: " << (this->Orient ? "On\n" : "Off\n");
  os << indent << "Orient Mode: " << (this->VectorMode == VTK_USE_VECTOR ?
                                       "Orient by vector\n" : "Orient by normal\n");
  os << indent << "Index Mode: ";
  if ( this->IndexMode == VTK_INDEXING_BY_SCALAR )
  {
    os << "Index by scalar value\n";
  }
  else if ( this->IndexMode == VTK_INDEXING_BY_VECTOR )
  {
    os << "Index by vector value\n";
  }
  else
  {
    os << "Indexing off\n";
  }

  os << indent << "Fill Cell Data: " << (this->FillCellData ? "On\n" : "Off\n");

  os << indent << "SourceTransform: ";
  if (this->SourceTransform)
  {
    os << endl;
    this->SourceTransform->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)" << endl;
  }
}

int vtkGlyph3D::RequestUpdateExtent(
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

//----------------------------------------------------------------------------
vtkPolyData* vtkGlyph3D::GetSource(int idx, vtkInformationVector *sourceInfo)
{
  vtkInformation *info = sourceInfo->GetInformationObject(idx);
  if (!info)
  {
    return NULL;
  }
  return vtkPolyData::SafeDownCast(info->Get(vtkDataObject::DATA_OBJECT()));
}

//----------------------------------------------------------------------------
int vtkGlyph3D::FillInputPortInformation(int port, vtkInformation *info)
{
  if (port == 0)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
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

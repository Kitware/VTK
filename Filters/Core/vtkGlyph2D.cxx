/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGlyph2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGlyph2D.h"

#include "vtkCell.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkTransform.h"
#include "vtkUnsignedCharArray.h"

vtkStandardNewMacro(vtkGlyph2D);

int vtkGlyph2D::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  vtkPointData *pd;
  vtkDataArray *inScalars;
  vtkDataArray *inVectors;
  unsigned char* inGhostLevels = 0;
  vtkDataArray *inNormals, *sourceNormals = NULL;
  vtkIdType numPts, numSourcePts, numSourceCells, inPtId, i;
  int index;
  vtkPoints *sourcePts = NULL;
  vtkPoints *newPts;
  vtkDataArray *newScalars=NULL;
  vtkDataArray *newVectors=NULL;
  vtkDataArray *newNormals=NULL;
  double x[3], v[3], s = 0.0, vMag = 0.0, value, theta;
  vtkTransform *trans = vtkTransform::New();
  vtkCell *cell;
  vtkIdList *cellPts;
  int npts;
  vtkIdList *pts;
  vtkIdType ptIncr, cellId;
  int haveVectors, haveNormals;
  double scalex,scaley, den;
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPointData *outputPD = output->GetPointData();
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  int numberOfSources = this->GetNumberOfInputConnections(1);
  vtkPolyData *source = 0;

  vtkDebugMacro(<<"Generating 2D glyphs");

  pts = vtkIdList::New();
  pts->Allocate(VTK_CELL_SIZE);

  pd = input->GetPointData();

  inScalars = this->GetInputArrayToProcess(0,inputVector);
  inVectors = this->GetInputArrayToProcess(1,inputVector);
  inNormals = this->GetInputArrayToProcess(2,inputVector);

  vtkDataArray* temp = 0;
  if (pd)
    {
    temp = pd->GetArray("vtkGhostLevels");
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


  if (inNormals && numPts != inNormals->GetNumberOfTuples())
    {
    vtkErrorMacro(<< "Number of points (" << numPts << ") does not match "
                  << "number of normals (" << inNormals->GetNumberOfTuples()
                  << ").");
    pts->Delete();
    trans->Delete();
    return 1;
    }

  if (inVectors && numPts != inVectors->GetNumberOfTuples())
    {
    vtkErrorMacro(<< "Number of points (" << numPts << ") does not match "
                  << "number of vectors (" << inVectors->GetNumberOfTuples()
                  << ").");
    pts->Delete();
    trans->Delete();
    return 1;
    }

  if (inScalars && numPts != inScalars->GetNumberOfTuples())
    {
    vtkErrorMacro(<< "Number of points (" << numPts << ") does not match "
                  << "number of scalars (" << inScalars->GetNumberOfTuples()
                  << ").");
    pts->Delete();
    trans->Delete();
    return 1;
    }

  if ( (this->IndexMode == VTK_INDEXING_BY_SCALAR && !inScalars) ||
       (this->IndexMode == VTK_INDEXING_BY_VECTOR &&
       ((!inVectors && this->VectorMode == VTK_USE_VECTOR) ||
        (!inNormals && this->VectorMode == VTK_USE_NORMAL))) )
    {
    if ( this->GetSource(0, inputVector[1]) == NULL )
      {
      vtkErrorMacro(<<"Indexing on but don't have data to index with");
      pts->Delete();
      trans->Delete();
      return 1;
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
  if ( this->IndexMode != VTK_INDEXING_OFF )
    {
    pd = NULL;
    //numSourcePts = numSourceCells = 0;
    haveNormals = 1;
    for (numSourcePts=numSourceCells=i=0; i < numberOfSources; i++)
      {
      source = this->GetSource(i, inputVector[1]);
      if ( source != NULL )
        {
        numSourcePts += source->GetNumberOfPoints();
        numSourceCells += source->GetNumberOfCells();
        sourceNormals = source->GetPointData()->GetNormals();
        if ( !sourceNormals )
          {
          haveNormals = 0;
          }
        }
      }
    }
  else
    {
    source = this->GetSource(0, inputVector[1]);
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

    // Prepare to copy output.
    pd = source->GetPointData();
    outputPD->CopyAllocate(pd,numPts*numSourcePts);
    }

  newPts = vtkPoints::New();
  newPts->Allocate(numPts*numSourcePts);
  if ( this->ColorMode == VTK_COLOR_BY_SCALAR && inScalars )
    {
    newScalars = inScalars->NewInstance();
    newScalars->SetNumberOfComponents(inScalars->GetNumberOfComponents());
    newScalars->Allocate(inScalars->GetNumberOfComponents()*numPts*numSourcePts);
    }
  else if ( (this->ColorMode == VTK_COLOR_BY_SCALE) && inScalars)
    {
    newScalars = vtkDoubleArray::New();
    newScalars->Allocate(numPts*numSourcePts);
    newScalars->SetName("GlyphScale");
    }
  else if ( (this->ColorMode == VTK_COLOR_BY_VECTOR) && haveVectors)
    {
    newScalars = vtkDoubleArray::New();
    newScalars->Allocate(numPts*numSourcePts);
    newScalars->SetName("VectorMagnitude");
    }
  if ( haveVectors )
    {
    newVectors = vtkDoubleArray::New();
    newVectors->SetNumberOfComponents(3);
    newVectors->Allocate(3*numPts*numSourcePts);
    newVectors->SetName("GlyphVector");
    }
  if ( haveNormals )
    {
    newNormals = vtkDoubleArray::New();
    newNormals->SetNumberOfComponents(3);
    newNormals->Allocate(3*numPts*numSourcePts);
    newNormals->SetName("Normals");
    }

  // Setting up for calls to PolyData::InsertNextCell()
  if (this->IndexMode != VTK_INDEXING_OFF )
    {
    output->Allocate(3*numPts*numSourceCells,numPts*numSourceCells);
    }
  else
    {
    output->Allocate(this->GetSource(0, inputVector[1]),
                     3*numPts*numSourceCells, numPts*numSourceCells);
    }

  // Traverse all Input points, transforming Source points and copying
  // point attributes.
  //
  ptIncr=0;
  for (inPtId=0; inPtId < numPts; inPtId++)
    {
    scalex = scaley = 1.0;
    if ( ! (inPtId % 10000) )
      {
      this->UpdateProgress(static_cast<double>(inPtId)/numPts);
      if (this->GetAbortExecute())
        {
        break;
        }
      }

    // Get the scalar and vector data
    if ( inScalars )
      {
      s = inScalars->GetComponent(inPtId, 0);
      if ( this->ScaleMode == VTK_SCALE_BY_SCALAR ||
                this->ScaleMode == VTK_DATA_SCALING_OFF )
        {
        scalex = scaley = s;
        }
      }

    if ( haveVectors )
      {
      if ( this->VectorMode == VTK_USE_NORMAL )
        {
        inNormals->GetTuple(inPtId, v);
        }
      else
        {
        inVectors->GetTuple(inPtId, v);
        }
      vMag = vtkMath::Norm(v);
      if ( this->ScaleMode == VTK_SCALE_BY_VECTORCOMPONENTS )
        {
        scalex = v[0];
        scaley = v[1];
        }
      else if ( this->ScaleMode == VTK_SCALE_BY_VECTOR )
        {
        scalex = scaley = vMag;
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
      }

    // Compute index into table of glyphs
    if ( this->IndexMode == VTK_INDEXING_OFF )
      {
      index = 0;
      }
    else
      {
      if ( this->IndexMode == VTK_INDEXING_BY_SCALAR )
        {
        value = s;
        }
      else
        {
        value = vMag;
        }

      index = static_cast<int>((value-this->Range[0])*numberOfSources/den);
      index = (index < 0 ? 0 :
              (index >= numberOfSources ? (numberOfSources-1) : index));

      source = this->GetSource(index, inputVector[1]);
      if ( source != NULL )
        {
        sourcePts = source->GetPoints();
        sourceNormals = source->GetPointData()->GetNormals();
        numSourcePts = sourcePts->GetNumberOfPoints();
        numSourceCells = source->GetNumberOfCells();
        }
      }

    // Make sure we're not indexing into empty glyph
    if ( this->GetSource(index, inputVector[1]) == NULL )
      {
      continue;
      }

    // Check ghost points.
    if (inGhostLevels && inGhostLevels[inPtId] > 0)
      {
      continue;
      }

    // Now begin copying/transforming glyph
    trans->Identity();

    // Copy all topology (transformation independent)
    for (cellId=0; cellId < numSourceCells; cellId++)
      {
      cell = this->GetSource(index, inputVector[1])->GetCell(cellId);
      cellPts = cell->GetPointIds();
      npts = cellPts->GetNumberOfIds();
      for (pts->Reset(), i=0; i < npts; i++)
        {
        pts->InsertId(i,cellPts->GetId(i) + ptIncr);
        }
      output->InsertNextCell(cell->GetCellType(),pts);
      }

    // translate Source to Input point
    input->GetPoint(inPtId, x);
    trans->Translate(x[0], x[1], 0.0);

    if ( haveVectors )
      {
      // Copy Input vector
      for (i=0; i < numSourcePts; i++)
        {
        newVectors->InsertTuple(i+ptIncr, v);
        }
      if (this->Orient && (vMag > 0.0))
        {
        theta = vtkMath::DegreesFromRadians( atan2( v[1], v[0] ) );
        trans->RotateWXYZ( theta, 0.0, 0.0, 1.0 );
        }
      }

    // determine scale factor from scalars if appropriate
    if ( inScalars )
      {
      // Copy scalar value
      if (this->ColorMode == VTK_COLOR_BY_SCALE)
        {
        for (i=0; i < numSourcePts; i++)
          {
          newScalars->InsertTuple(i+ptIncr, &scalex);
          }
        }
      else if (this->ColorMode == VTK_COLOR_BY_SCALAR)
        {
        for (i=0; i < numSourcePts; i++)
          {
          outputPD->CopyTuple(inScalars, newScalars, inPtId, ptIncr+i);
          }
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
        scalex = scaley = this->ScaleFactor;
        }
      else
        {
        scalex *= this->ScaleFactor;
        scaley *= this->ScaleFactor;
        }

      if ( scalex == 0.0 )
        {
        scalex = 1.0e-10;
        }
      if ( scaley == 0.0 )
        {
        scaley = 1.0e-10;
        }
      trans->Scale(scalex,scaley,1.0);
      }

    // multiply points and normals by resulting matrix
    trans->TransformPoints(sourcePts,newPts);

    if ( haveNormals )
      {
      trans->TransformNormals(sourceNormals,newNormals);
      }

    // Copy point data from source (if possible)
    if ( pd )
      {
      for (i=0; i < numSourcePts; i++)
        {
        outputPD->CopyData(pd,i,ptIncr+i);
        }
      }
    ptIncr += numSourcePts;
    }

  // Update ourselves and release memory
  //
  output->SetPoints(newPts);
  newPts->Delete();

  if (newScalars)
    {
    outputPD->AddArray(newScalars);
    outputPD->SetActiveScalars(newScalars->GetName());
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

  output->Squeeze();
  trans->Delete();
  pts->Delete();

  return 1;
}

void vtkGlyph2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

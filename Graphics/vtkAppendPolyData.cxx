/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAppendPolyData.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkAppendPolyData.h"
#include "vtkObjectFactory.h"


//-----------------------------------------------------------------------
vtkAppendPolyData* vtkAppendPolyData::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkAppendPolyData");
  if(ret)
    {
    return (vtkAppendPolyData*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkAppendPolyData;
}

//----------------------------------------------------------------------------
vtkAppendPolyData::vtkAppendPolyData()
{
  this->ParallelStreaming = 0;
  this->UserManagedInputs = 0;
}

//----------------------------------------------------------------------------
vtkAppendPolyData::~vtkAppendPolyData()
{
}

//----------------------------------------------------------------------------
// Add a dataset to the list of data to append.
void vtkAppendPolyData::AddInput(vtkPolyData *ds)
{
  if (UserManagedInputs)
    {
    vtkErrorMacro(<<
      "AddInput is not supported if UserManagedInputs is true");
    return;
    }
  this->vtkProcessObject::AddInput(ds);
}

//----------------------------------------------------------------------------
// Remove a dataset from the list of data to append.
void vtkAppendPolyData::RemoveInput(vtkPolyData *ds)
{
  if (UserManagedInputs)
    {
    vtkErrorMacro(<<
      "RemoveInput is not supported if UserManagedInputs is true");
    return;
    }
  this->vtkProcessObject::RemoveInput(ds);
  this->vtkProcessObject::SqueezeInputArray();
}

//----------------------------------------------------------------------------
// make ProcessObject function visible
// should only be used when UserManagedInputs is true.
void vtkAppendPolyData::SetNumberOfInputs(int num)
{
  int overlap;
  if (!this->UserManagedInputs)
    {
    vtkErrorMacro(<<
      "SetNumberOfInputs is not supported if UserManagedInputs is false");
    return;
    }
  // if the user sets inputs to be less than we already have, we'd better
  // unregister the tailend ones that are being discarded
  overlap = this->GetNumberOfInputs() - num;
  if (overlap>0)
    {
    for (int i=0; i<overlap; i++)
      {
      this->SetNthInput(i+num,NULL);
      }
    }
  this->vtkProcessObject::SetNumberOfInputs(num);
}

//----------------------------------------------------------------------------
// Set Nth input, should only be used when UserManagedInputs is true.
void vtkAppendPolyData::SetInputByNumber(int num, vtkPolyData *input)
{
  if (!UserManagedInputs)
    {
    vtkErrorMacro(<<
      "SetInputByNumber is not supported if UserManagedInputs is false");
    return;
    }
  this->vtkProcessObject::SetNthInput(num, input);
}

//----------------------------------------------------------------------------
// This method is much too long, and has to be broken up!
// Append data sets into single unstructured grid
void vtkAppendPolyData::Execute()
{
  int idx;
  vtkPolyData *ds;
  vtkPoints  *inPts;
  vtkPoints *newPts;
  vtkCellArray *inVerts, *newVerts;
  vtkCellArray *inLines, *newLines;
  vtkCellArray *inPolys, *newPolys;
  vtkIdType sizePolys, numPolys;
  vtkCellArray *inStrips, *newStrips;
  vtkIdType numPts, numCells;
  vtkPointData *inPD = NULL;
  vtkCellData *inCD = NULL;
  vtkPolyData *output = this->GetOutput();
  vtkPointData *outputPD = output->GetPointData();
  vtkCellData *outputCD = output->GetCellData();
  vtkDataArray *newPtScalars = NULL;
  vtkDataArray *newPtVectors = NULL;
  vtkDataArray *newPtNormals = NULL;
  vtkDataArray *newPtTCoords = NULL;
  vtkDataArray *newPtTensors = NULL;
  int i;
  vtkIdType *pts, *pPolys, npts, ptId, cellId;
  
  vtkDebugMacro(<<"Appending polydata");

  // loop over all data sets, checking to see what point data is available.
  numPts = 0;
  numCells = 0;
  sizePolys = numPolys = 0;

  int countPD=0;
  int countCD=0;

  // These Field lists are very picky.  Count the number of non empty inputs
  // so we can initialize them properly.
  for (idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    ds = (vtkPolyData *)(this->Inputs[idx]);
    if (ds != NULL)
      {
      if ( ds->GetNumberOfPoints() > 0)
        {
        ++countPD;
        }
      if (ds->GetNumberOfCells() > 0 )  
        {
        ++countCD;
        } // for a data set that has cells
      } // for a non NULL input
    } // for each input

  // These are used to determine which fields are available for appending
  vtkDataSetAttributes::FieldList ptList(countPD);
  vtkDataSetAttributes::FieldList cellList(countCD);  
  
  countPD = countCD = 0;
  for (idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    ds = (vtkPolyData *)(this->Inputs[idx]);
    if (ds != NULL)
      {
      // Skip points and cells if there are no points.  Empty inputs may have no arrays.
      if ( ds->GetNumberOfPoints() > 0)
        {
        numPts += ds->GetNumberOfPoints();
        // Take intersection of available point data fields.
        inPD = ds->GetPointData();
        if ( countPD == 0 )
          {
          ptList.InitializeFieldList(inPD);
          }
        else
          {
          ptList.IntersectFieldList(inPD);
          }
        ++countPD;
        } // for a data set that has points
        
      // Although we cannot have cells without points ... let's not nest.
      if (ds->GetNumberOfCells() > 0 )  
        {
        // keep track of the size of the poly cell array
        if (ds->GetPolys())
          {
          numPolys += ds->GetPolys()->GetNumberOfCells();
          sizePolys += ds->GetPolys()->GetNumberOfConnectivityEntries();
          }
        numCells += ds->GetNumberOfCells();
        
        inCD = ds->GetCellData();
        if ( countCD == 0 )
          {
          cellList.InitializeFieldList(inCD);
          }
        else
          {
          cellList.IntersectFieldList(inCD);
          }
        ++countCD;
        } // for a data set that has cells
      } // for a non NULL input
    } // for each input

  if ( numPts < 1 || numCells < 1 )
    {
    //vtkErrorMacro(<<"No data to append!");
    return;
    }
  this->UpdateProgress(0.10);

  // Examine the points and check if they're the same type. If not,
  // use highest (double probably), otherwise the type of the first
  // array (float no doubt). Depends on defs in vtkSetGet.h - Warning.
  int ttype, firstType=1, AllSame=1;
  int pointtype = 0;

  // Keep track of types for fast point append
  for (idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    ds = (vtkPolyData *)(this->Inputs[idx]);
    if (ds != NULL && ds->GetNumberOfPoints()>0)
      {
      if ( firstType )
        {
        firstType = 0;
        pointtype = ds->GetPoints()->GetData()->GetDataType();
        }
      ttype = ds->GetPoints()->GetData()->GetDataType();
      
      if ( ttype != pointtype )
        {
        AllSame = 0;
        vtkDebugMacro(<<"Different point data types");
        }
      pointtype = pointtype > ttype ? pointtype : ttype;
      }
    }

  // Allocate geometry/topology
  newPts = vtkPoints::New(pointtype);
  newPts->SetNumberOfPoints(numPts);

  newVerts = vtkCellArray::New();
  newVerts->Allocate(numCells*4);

  newLines = vtkCellArray::New();
  newLines->Allocate(numCells*4);

  newStrips = vtkCellArray::New();
  newStrips->Allocate(numCells*4);

  newPolys = vtkCellArray::New();
  pPolys = newPolys->WritePointer(numPolys, sizePolys);

  // These are created manually for faster execution
  if ( ptList.IsAttributePresent(vtkDataSetAttributes::SCALARS) > -1 )
    {
    outputPD->CopyScalarsOff();
    newPtScalars = inPD->GetActiveScalars()->MakeObject();
    newPtScalars->SetName(inPD->GetActiveScalars()->GetName());
    newPtScalars->SetNumberOfTuples(numPts);
    }
  if ( ptList.IsAttributePresent(vtkDataSetAttributes::VECTORS) > -1 )
    {
    outputPD->CopyVectorsOff();
    newPtVectors = inPD->GetActiveVectors()->MakeObject();
    newPtVectors->SetNumberOfTuples(numPts);
    }
  if ( ptList.IsAttributePresent(vtkDataSetAttributes::TENSORS) > -1 )
    {
    outputPD->CopyTensorsOff();
    newPtTensors = inPD->GetActiveTensors()->MakeObject();
    newPtTensors->SetNumberOfTuples(numPts);
    }
  if ( ptList.IsAttributePresent(vtkDataSetAttributes::NORMALS) > -1 )
    {
    outputPD->CopyNormalsOff();
    newPtNormals = inPD->GetActiveNormals()->MakeObject();
    newPtNormals->SetNumberOfTuples(numPts);
    }
  if ( ptList.IsAttributePresent(vtkDataSetAttributes::TCOORDS) > -1 )
    {
    outputPD->CopyTCoordsOff();
    newPtTCoords = inPD->GetActiveTCoords()->MakeObject();
    newPtTCoords->SetNumberOfTuples(numPts);
    }

  // Allocate the point and cell data
  outputPD->CopyAllocate(ptList,numPts);
  outputCD->CopyAllocate(cellList,numCells);

  // loop over all input sets
  vtkIdType ptOffset = 0;
  vtkIdType cellOffset = 0;
  countPD = countCD = 0;
  for (idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    this->UpdateProgress(0.2 + 0.8*idx/this->NumberOfInputs);
    ds = (vtkPolyData *)(this->Inputs[idx]);
    // this check is not necessary, but I'll put it in anyway
    if (ds != NULL)
      {
      numPts = ds->GetNumberOfPoints();
      numCells = ds->GetNumberOfCells();
      if ( numPts <= 0 && numCells <= 0 )
        {
        continue; //no input, just skip
        }

      inPD = ds->GetPointData();
      inCD = ds->GetCellData();

      inPts = ds->GetPoints();
      inVerts = ds->GetVerts();
      inLines = ds->GetLines();
      inPolys = ds->GetPolys();
      inStrips = ds->GetStrips();

      if (ds->GetNumberOfPoints() > 0)
        {
        // copy points directly
        if (AllSame)
          {
          this->AppendData(newPts->GetData(),
                           inPts->GetData(), ptOffset);
          }
        else
          {
          this->AppendDifferentPoints(newPts->GetData(),
                                      inPts->GetData(), ptOffset);
          }
        // copy scalars directly
        if (newPtScalars)
          {
          this->AppendData(newPtScalars,inPD->GetActiveScalars(), ptOffset);
          }
        // copy normals directly
        if (newPtNormals)
          {
          this->AppendData(newPtNormals, inPD->GetActiveNormals(), ptOffset);
          }
        // copy vectors directly
        if (newPtVectors)
          {
          this->AppendData(newPtVectors, inPD->GetActiveVectors(), ptOffset);
          }
        // copy tcoords directly
        if (newPtTCoords)
          {
          this->AppendData(newPtTCoords, inPD->GetActiveTCoords() , ptOffset);
          }
        // copy tensors directly
        if (newPtTensors)
          {
          this->AppendData(newPtTensors, inPD->GetActiveTensors(), ptOffset);
          }
        // append the remainder of the field data
        for (ptId=0; ptId < numPts; ptId++)
          {
          outputPD->CopyData(ptList,inPD,countPD,ptId,ptId+ptOffset);
          }
        ++countPD;
        }

      if (ds->GetNumberOfCells() > 0)
        {
        // cell data could be made efficient like the point data,
        // but I will wait on that.
        // copy cell data
        for (cellId=0; cellId < numCells; cellId++)
          {
          outputCD->CopyData(cellList,inCD,countCD,cellId,cellId+cellOffset);
          }
        ++countCD;
        
        // copy the cells
        pPolys = this->AppendCells(pPolys, inPolys, ptOffset);
        
        // These other cell arrays could be made efficient like polys ...
        for (inVerts->InitTraversal(); inVerts->GetNextCell(npts,pts); )
          {
          newVerts->InsertNextCell(npts);
          for (i=0; i < npts; i++)
            {
            newVerts->InsertCellPoint(pts[i]+ptOffset);
            }
          }
        
        for (inLines->InitTraversal(); inLines->GetNextCell(npts,pts); )
          {
          newLines->InsertNextCell(npts);
          for (i=0; i < npts; i++)
            {
            newLines->InsertCellPoint(pts[i]+ptOffset);
            }
          }

        for (inStrips->InitTraversal(); inStrips->GetNextCell(npts,pts); )
          {
          newStrips->InsertNextCell(npts);
          for (i=0; i < npts; i++)
            {
            newStrips->InsertCellPoint(pts[i]+ptOffset);
            }
          }
        }
      ptOffset += numPts;
      cellOffset += numCells;
      }
    }
  
  // Update ourselves and release memory
  //
  output->SetPoints(newPts);
  newPts->Delete();

  if (newPtScalars)
    {
    output->GetPointData()->SetScalars(newPtScalars);
    newPtScalars->Delete();
    }
  if (newPtNormals)
    {
    output->GetPointData()->SetNormals(newPtNormals);
    newPtNormals->Delete();
    }
  if (newPtVectors)
    {
    output->GetPointData()->SetVectors(newPtVectors);
    newPtVectors->Delete();
    }
  if (newPtTCoords)
    {
    output->GetPointData()->SetTCoords(newPtTCoords);
    newPtTCoords->Delete();
    }
  if (newPtTensors)
    {
    output->GetPointData()->SetTensors(newPtTensors);
    newPtTensors->Delete();
    }
  
  if ( newVerts->GetNumberOfCells() > 0 )
    {
    output->SetVerts(newVerts);
    }
  newVerts->Delete();

  if ( newLines->GetNumberOfCells() > 0 )
    {
    output->SetLines(newLines);
    }
  newLines->Delete();

  if ( newPolys->GetNumberOfCells() > 0 )
    {
    output->SetPolys(newPolys);
    }
  newPolys->Delete();

  if ( newStrips->GetNumberOfCells() > 0 )
    {
    output->SetStrips(newStrips);
    }
  newStrips->Delete();

  // When all optimizations are complete, this squeeze will be unecessary.
  // (But it does not seem to cost much.)
  output->Squeeze();
}

//----------------------------------------------------------------------------
void vtkAppendPolyData::ComputeInputUpdateExtents(vtkDataObject *data)
{
  int piece, numPieces, ghostLevel;
  vtkPolyData *output = (vtkPolyData *)data;
  int idx;

  output->GetUpdateExtent(piece, numPieces, ghostLevel);
  
  // make sure piece is valid
  if (piece < 0 || piece >= numPieces)
    {
    return;
    }

  if (this->ParallelStreaming)
    {
    piece = piece * this->NumberOfInputs;
    numPieces = numPieces * this->NumberOfInputs;
    }
 
  // just copy the Update extent as default behavior.
  for (idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    if (this->Inputs[idx])
      {
      if (this->ParallelStreaming)
        {
        this->Inputs[idx]->SetUpdateExtent(piece + idx, numPieces, ghostLevel);
        }
      else
        {
        this->Inputs[idx]->SetUpdateExtent(piece, numPieces, ghostLevel);
        }
      }
    }
  
  // Save the piece so execute can use this information.
  this->ExecutePiece = piece;
  this->ExecuteNumberOfPieces = numPieces;
}

//----------------------------------------------------------------------------
vtkPolyData *vtkAppendPolyData::GetInput(int idx)
{
  if (idx >= this->NumberOfInputs || idx < 0)
    {
    return NULL;
    }

  return (vtkPolyData *)(this->Inputs[idx]);
}

//----------------------------------------------------------------------------
void vtkAppendPolyData::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyDataToPolyDataFilter::PrintSelf(os,indent);

  if ( this->ParallelStreaming )
    {
    os << indent << "ParallelStreamingOn\n";
    }
  else
    {
    os << indent << "ParallelStreamingOff\n";
    }
  //
  os << indent << "UserManagedInputs: "
     << this->UserManagedInputs << endl;
}

void vtkAppendPolyData::AppendData(vtkDataArray *dest, vtkDataArray *src,
                                   vtkIdType offset)
{
  void *pSrc, *pDest;
  vtkIdType length;

  // sanity checks
  if (src->GetDataType() != dest->GetDataType())
    {
    vtkErrorMacro("Data type mismatch.");
    return;
    }
  if (src->GetNumberOfComponents() != dest->GetNumberOfComponents())
    {
    vtkErrorMacro("NumberOfComponents mismatch.");
    return;
    }
  if (src->GetNumberOfTuples() + offset > dest->GetNumberOfTuples())
    {
    vtkErrorMacro("Destination not big enough");
    return;
    }
  
  // convert from tuples to components.
  offset *= src->GetNumberOfComponents();
  length = src->GetMaxId() + 1;

  switch (src->GetDataType())
    {
    case VTK_FLOAT:
      length *= sizeof(float);
      break;
    case VTK_DOUBLE:
      length *= sizeof(double);
      break;
    case VTK_INT:
      length *= sizeof(int);
      break;
    case VTK_UNSIGNED_INT:
      length *= sizeof(unsigned int);
      break;
    case VTK_LONG:
      length *= sizeof(long);
      break;
    case VTK_UNSIGNED_LONG:
      length *= sizeof(unsigned long);
      break;
    case VTK_SHORT:
      length *= sizeof(short);
      break;
    case VTK_UNSIGNED_SHORT:
      length *= sizeof(unsigned short);
      break;
    case VTK_UNSIGNED_CHAR:
      length *= sizeof(unsigned char);
      break;
    case VTK_CHAR:
      length *= sizeof(char);
      break;
    default:
      vtkErrorMacro("Unknown data type " << src->GetDataType());
    }

  pSrc  = src->GetVoidPointer(0);
  pDest = dest->GetVoidPointer(offset);

  memcpy(pDest, pSrc, length);
}

void vtkAppendPolyData::AppendDifferentPoints(vtkDataArray *dest,
                                              vtkDataArray *src,
                                              vtkIdType offset)
{
  float  *fSrc;
  double *dSrc, *dDest;
  vtkIdType p;

  if (src->GetNumberOfTuples() + offset > dest->GetNumberOfTuples())
    {
    vtkErrorMacro("Destination not big enough");
    return;
    }

  vtkIdType vals = src->GetMaxId()+1;
  switch (dest->GetDataType())
    {
    //
    // Dest is FLOAT - if sources are not all same type, dest ought to
    // be double. (assuming float and double are the only choices)
    //
    case VTK_FLOAT:
        vtkErrorMacro("Dest type should be double? "
            << dest->GetDataType());
        break;
    //
    // Dest is DOUBLE - sources may be mixed float/double combinations
    //

    case VTK_DOUBLE:
      dDest = (double*)(dest->GetVoidPointer(offset*src->GetNumberOfComponents()));
      //
      switch (src->GetDataType())
        {
        case VTK_FLOAT:
          fSrc = (float*)(src->GetVoidPointer(0));
          for (p=0; p<vals; p++)
            {
              dDest[p] = (double) fSrc[p];
            }
          break;
        case VTK_DOUBLE:
          dSrc = (double*)(src->GetVoidPointer(0));
          memcpy(dDest, dSrc, vals*sizeof(double));
          break;
        default:
          vtkErrorMacro("Unknown data type " << dest->GetDataType());
        }
      break;
      //
    default:
      vtkErrorMacro("Unknown data type " << dest->GetDataType());
    }

}


// returns the next pointer in dest
vtkIdType *vtkAppendPolyData::AppendCells(vtkIdType *pDest, vtkCellArray *src,
                                          vtkIdType offset)
{
  vtkIdType *pSrc, *end, *pNum;

  if (src == NULL)
    {
    return pDest;
    }

  pSrc = (vtkIdType*)(src->GetPointer());
  end = pSrc + src->GetNumberOfConnectivityEntries();
  pNum = pSrc;
  
  while (pSrc < end)
    {
    if (pSrc == pNum)
      {
      // move cell pointer to next cell
      pNum += 1+*pSrc;
      // copy the number of cells
      *pDest++ = *pSrc++;
      }
    else
      {
      // offset the point index
      *pDest++ = offset + *pSrc++;
      }
    }
  
  return pDest;
}

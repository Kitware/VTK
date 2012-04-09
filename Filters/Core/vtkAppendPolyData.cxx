/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAppendPolyData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAppendPolyData.h"

#include "vtkAlgorithmOutput.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataSetAttributes.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTrivialProducer.h"

vtkStandardNewMacro(vtkAppendPolyData);

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
void vtkAppendPolyData::AddInputData(vtkPolyData *ds)
{
  if (this->UserManagedInputs)
    {
    vtkErrorMacro(<<
      "AddInput is not supported if UserManagedInputs is true");
    return;
    }
  this->Superclass::AddInputData(ds);
}

//----------------------------------------------------------------------------
// Remove a dataset from the list of data to append.
void vtkAppendPolyData::RemoveInputData(vtkPolyData *ds)
{
  if (this->UserManagedInputs)
    {
    vtkErrorMacro(<<
      "RemoveInput is not supported if UserManagedInputs is true");
    return;
    }

  if (!ds)
    {
    return;
    }
  int numCons = this->GetNumberOfInputConnections(0);
  for(int i=0; i<numCons; i++)
    {
    if (this->GetInput(i) == ds)
      {
      this->RemoveInputConnection(0,
        this->GetInputConnection(0, i));
      }
    }
}

//----------------------------------------------------------------------------
// make ProcessObject function visible
// should only be used when UserManagedInputs is true.
void vtkAppendPolyData::SetNumberOfInputs(int num)
{
  if (!this->UserManagedInputs)
    {
    vtkErrorMacro(<<
      "SetNumberOfInputs is not supported if UserManagedInputs is false");
    return;
    }

  // Ask the superclass to set the number of connections.
  this->SetNumberOfInputConnections(0, num);
}

//----------------------------------------------------------------------------
void vtkAppendPolyData::SetInputDataByNumber(int num, vtkPolyData* input)
{
  vtkTrivialProducer* tp = vtkTrivialProducer::New();
  tp->SetOutput(input);
  this->SetInputConnectionByNumber(num, tp->GetOutputPort());
  tp->Delete();
}

//----------------------------------------------------------------------------
// Set Nth input, should only be used when UserManagedInputs is true.
void vtkAppendPolyData::SetInputConnectionByNumber(int num,
                                                   vtkAlgorithmOutput *input)
{
  if (!this->UserManagedInputs)
    {
    vtkErrorMacro(<<
      "SetInputConnectionByNumber is not supported if UserManagedInputs is false");
    return;
    }

  // Ask the superclass to connect the input.
  this->SetNthInputConnection(0, num, input);
}

//----------------------------------------------------------------------------
int vtkAppendPolyData::ExecuteAppend(vtkPolyData* output,
    vtkPolyData* inputs[], int numInputs)
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
  vtkPointData *outputPD = output->GetPointData();
  vtkCellData *outputCD = output->GetCellData();
  vtkDataArray *newPtScalars = NULL;
  vtkDataArray *newPtVectors = NULL;
  vtkDataArray *newPtNormals = NULL;
  vtkDataArray *newPtTCoords = NULL;
  vtkDataArray *newPtTensors = NULL;
  int i;
  vtkIdType *pts = 0;
  vtkIdType *pPolys;
  vtkIdType npts = 0;
  vtkIdType ptId, cellId;

  vtkDebugMacro(<<"Appending polydata");

  // loop over all data sets, checking to see what point data is available.
  numPts = 0;
  numCells = 0;
  sizePolys = numPolys = 0;

  int countPD=0;
  int countCD=0;

  vtkIdType numVerts = 0, numLines = 0, numStrips = 0;

  // These Field lists are very picky.  Count the number of non empty inputs
  // so we can initialize them properly.
  for (idx = 0; idx < numInputs; ++idx)
    {
    ds = inputs[idx];
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
  for (idx = 0; idx < numInputs; ++idx)
    {
    ds = inputs[idx];
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
          sizePolys += ds->GetPolys()->GetNumberOfConnectivityEntries();
          }
        numCells += ds->GetNumberOfCells();
        // Count the cells of each type.
        // This is used to ensure that cell data is copied at the correct
        // locations in the output.
        numVerts += ds->GetNumberOfVerts();
        numLines += ds->GetNumberOfLines();
        numPolys += ds->GetNumberOfPolys();
        numStrips += ds->GetNumberOfStrips();

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
    vtkDebugMacro(<<"No data to append!");
    return 1;
    }
  this->UpdateProgress(0.10);

  // Examine the points and check if they're the same type. If not,
  // use highest (double probably), otherwise the type of the first
  // array (float no doubt). Depends on defs in vtkSetGet.h - Warning.
  int ttype, firstType=1, AllSame=1;
  int pointtype = 0;

  // Keep track of types for fast point append
  for (idx = 0; idx < numInputs; ++idx)
    {
    ds = inputs[idx];
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

  if (!pPolys && sizePolys > 0)
    {
    vtkErrorMacro(<<"Memory allocation failed in append filter");
    return 0;
    }

  // These are created manually for faster execution
  // Uses the properties of the last input
  vtkDataArray *inDA=0;
  if ( ptList.IsAttributePresent(vtkDataSetAttributes::SCALARS) > -1 )
    {
    inDA=inPD->GetScalars();
    outputPD->CopyScalarsOff();
    newPtScalars = inDA->NewInstance();
    newPtScalars->SetNumberOfComponents(inDA->GetNumberOfComponents());
    newPtScalars->CopyComponentNames( inDA );
    newPtScalars->SetName(inDA->GetName());
    newPtScalars->SetNumberOfTuples(numPts);
    if (inDA->HasInformation())
      {
      newPtScalars->CopyInformation(inDA->GetInformation(),/*deep=*/1);
      }
    }
  if ( ptList.IsAttributePresent(vtkDataSetAttributes::VECTORS) > -1 )
    {
    inDA=inPD->GetVectors();
    outputPD->CopyVectorsOff();
    newPtVectors = inDA->NewInstance();
    newPtVectors->SetNumberOfComponents(inDA->GetNumberOfComponents());
    newPtVectors->CopyComponentNames( inDA );
    newPtVectors->SetName(inDA->GetName());
    newPtVectors->SetNumberOfTuples(numPts);
    if (inDA->HasInformation())
      {
      newPtVectors->CopyInformation(inDA->GetInformation(),/*deep=*/1);
      }
    }
  if ( ptList.IsAttributePresent(vtkDataSetAttributes::TENSORS) > -1 )
    {
    inDA=inPD->GetTensors();
    outputPD->CopyTensorsOff();
    newPtTensors = inDA->NewInstance();
    newPtTensors->SetNumberOfComponents(inDA->GetNumberOfComponents());
    newPtTensors->CopyComponentNames( inDA );
    newPtTensors->SetName(inDA->GetName());
    newPtTensors->SetNumberOfTuples(numPts);
    if (inDA->HasInformation())
      {
      newPtTensors->CopyInformation(inDA->GetInformation(),/*deep=*/1);
      }
    }
  if ( ptList.IsAttributePresent(vtkDataSetAttributes::NORMALS) > -1 )
    {
    inDA=inPD->GetNormals();
    outputPD->CopyNormalsOff();
    newPtNormals = inDA->NewInstance();
    newPtNormals->SetNumberOfComponents(inDA->GetNumberOfComponents());
    newPtNormals->CopyComponentNames( inDA );
    newPtNormals->SetName(inDA->GetName());
    newPtNormals->SetNumberOfTuples(numPts);
    if (inDA->HasInformation())
      {
      newPtNormals->CopyInformation(inDA->GetInformation(),/*deep=*/1);
      }
    }
  if ( ptList.IsAttributePresent(vtkDataSetAttributes::TCOORDS) > -1 )
    {
    inDA=inPD->GetTCoords();
    outputPD->CopyTCoordsOff();
    newPtTCoords = inDA->NewInstance();
    newPtTCoords->SetNumberOfComponents(inDA->GetNumberOfComponents());
    newPtTCoords->CopyComponentNames( inDA );
    newPtTCoords->SetName(inDA->GetName());
    newPtTCoords->SetNumberOfTuples(numPts);
    if (inDA->HasInformation())
      {
      newPtTCoords->CopyInformation(inDA->GetInformation(),/*deep=*/1);
      }
    }

  // Allocate the point and cell data
  outputPD->CopyAllocate(ptList,numPts);
  outputCD->CopyAllocate(cellList,numCells);

  // loop over all input sets
  vtkIdType ptOffset = 0;
  vtkIdType vertOffset = 0;
  vtkIdType linesOffset = 0;
  vtkIdType polysOffset = 0;
  vtkIdType stripsOffset = 0;
  countPD = countCD = 0;
  for (idx = 0; idx < numInputs; ++idx)
    {
    this->UpdateProgress(0.2 + 0.8*idx/numInputs);
    ds = inputs[idx];
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
          this->AppendData(newPtScalars,inPD->GetScalars(), ptOffset);
          }
        // copy normals directly
        if (newPtNormals)
          {
          this->AppendData(newPtNormals, inPD->GetNormals(), ptOffset);
          }
        // copy vectors directly
        if (newPtVectors)
          {
          this->AppendData(newPtVectors, inPD->GetVectors(), ptOffset);
          }
        // copy tcoords directly
        if (newPtTCoords)
          {
          this->AppendData(newPtTCoords, inPD->GetTCoords() , ptOffset);
          }
        // copy tensors directly
        if (newPtTensors)
          {
          this->AppendData(newPtTensors, inPD->GetTensors(), ptOffset);
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
        // These are the cellIDs at which each of the cell types start.
        vtkIdType linesIndex = ds->GetNumberOfVerts();
        vtkIdType polysIndex = linesIndex + ds->GetNumberOfLines();
        vtkIdType stripsIndex = polysIndex + ds->GetNumberOfPolys();

        // cell data could be made efficient like the point data,
        // but I will wait on that.
        // copy cell data
        for (cellId=0; cellId < numCells; cellId++)
          {
          vtkIdType outCellId = 0;
          if (cellId < linesIndex)
            {
            outCellId = vertOffset;
            vertOffset++;
            }
          else if (cellId < polysIndex)
            {
            // outCellId = number of lines we already added + total number of
            // verts expected in the output.
            outCellId = linesOffset + numVerts;
            linesOffset++;
            }
          else if (cellId < stripsIndex)
            {
            // outCellId = number of polys we already added + total number of
            // verts and lines expected in the output.
            outCellId = polysOffset + numLines + numVerts;
            polysOffset++;
            }
          else
            {
            // outCellId = number of tstrips we already added + total number of
            // polys, verts and lines expected in the output.
            outCellId = stripsOffset + numPolys + numLines + numVerts;
            stripsOffset++;
            }
          outputCD->CopyData(cellList,inCD,countCD,cellId,outCellId);
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

  // When all optimizations are complete, this squeeze will be unnecessary.
  // (But it does not seem to cost much.)
  output->Squeeze();

  return 1;
}

//----------------------------------------------------------------------------
// This method is much too long, and has to be broken up!
// Append data sets into single polygonal data set.
int vtkAppendPolyData::RequestData(vtkInformation *vtkNotUsed(request),
                                   vtkInformationVector **inputVector,
                                   vtkInformationVector *outputVector)
{
  // get the info object
  // get the ouptut
  vtkPolyData *output = vtkPolyData::GetData(outputVector, 0);

  int numInputs = inputVector[0]->GetNumberOfInformationObjects();
  if (numInputs == 1)
    {
    output->ShallowCopy(vtkPolyData::GetData(inputVector[0], 0));
    return 1;
    }

  vtkPolyData** inputs = new vtkPolyData*[numInputs];
  for (int idx = 0; idx < numInputs; ++idx)
    {
    inputs[idx] = vtkPolyData::GetData(inputVector[0], idx);
    }
  int retVal = this->ExecuteAppend(output, inputs, numInputs);
  delete [] inputs;
  return retVal;
}

//----------------------------------------------------------------------------
int vtkAppendPolyData::RequestUpdateExtent(vtkInformation *vtkNotUsed(request),
                                           vtkInformationVector **inputVector,
                                           vtkInformationVector *outputVector)
{
  // get the output info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  int piece, numPieces, ghostLevel;
  int idx;

  piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  numPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  ghostLevel = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());

  // make sure piece is valid
  if (piece < 0 || piece >= numPieces)
    {
    return 0;
    }

  int numInputs = this->GetNumberOfInputConnections(0);
  if (this->ParallelStreaming)
    {
    piece = piece * numInputs;
    numPieces = numPieces * numInputs;
    }

  vtkInformation *inInfo;
  // just copy the Update extent as default behavior.
  for (idx = 0; idx < numInputs; ++idx)
    {
    inInfo = inputVector[0]->GetInformationObject(idx);
    if (inInfo)
      {
      if (this->ParallelStreaming)
        {
        inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
                    piece + idx);
        inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
                    numPieces);
        inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
                    ghostLevel);
        }
      else
        {
        inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
                    piece);
        inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
                    numPieces);
        inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
                    ghostLevel);
        }
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
vtkPolyData *vtkAppendPolyData::GetInput(int idx)
{
  return vtkPolyData::SafeDownCast(
    this->GetExecutive()->GetInputData(0, idx));
}

//----------------------------------------------------------------------------
void vtkAppendPolyData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << "ParallelStreaming:" << (this->ParallelStreaming?"On":"Off") << endl;
  os << "UserManagedInputs:" << (this->UserManagedInputs?"On":"Off") << endl;
}

//----------------------------------------------------------------------------
template <class T>
size_t vtkAppendPolyDataGetTypeSize(T*)
{
  return sizeof(T);
}

//----------------------------------------------------------------------------
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
    vtkTemplateMacro(
      length *= vtkAppendPolyDataGetTypeSize(static_cast<VTK_TT*>(0))
      );
    default:
      vtkErrorMacro("Unknown data type " << src->GetDataType());
    }

  pSrc  = src->GetVoidPointer(0);
  pDest = dest->GetVoidPointer(offset);

  memcpy(pDest, pSrc, length);
}

//----------------------------------------------------------------------------
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
      dDest = static_cast<double*>(
        dest->GetVoidPointer(offset*src->GetNumberOfComponents()));
      //
      switch (src->GetDataType())
        {
        case VTK_FLOAT:
          fSrc = static_cast<float*>(src->GetVoidPointer(0));
          for (p=0; p<vals; p++)
            {
            dDest[p] = static_cast<double>(fSrc[p]);
            }
          break;
        case VTK_DOUBLE:
          dSrc = static_cast<double*>(src->GetVoidPointer(0));
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


//----------------------------------------------------------------------------
// returns the next pointer in dest
vtkIdType *vtkAppendPolyData::AppendCells(vtkIdType *pDest, vtkCellArray *src,
                                          vtkIdType offset)
{
  vtkIdType *pSrc, *end, *pNum;

  if (src == NULL)
    {
    return pDest;
    }

  pSrc = src->GetPointer();
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

//----------------------------------------------------------------------------
int vtkAppendPolyData::FillInputPortInformation(int port, vtkInformation *info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
    {
    return 0;
    }
  info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
  return 1;
}

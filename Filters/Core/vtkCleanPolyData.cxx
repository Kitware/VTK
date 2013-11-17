/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCleanPolyData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCleanPolyData.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkMergePoints.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkIncrementalPointLocator.h"

vtkStandardNewMacro(vtkCleanPolyData);

//---------------------------------------------------------------------------
// Specify a spatial locator for speeding the search process. By
// default an instance of vtkPointLocator is used.
vtkCxxSetObjectMacro(vtkCleanPolyData,Locator,vtkIncrementalPointLocator);

//---------------------------------------------------------------------------
// Construct object with initial Tolerance of 0.0
vtkCleanPolyData::vtkCleanPolyData()
{
  this->PointMerging = 1;
  this->ToleranceIsAbsolute  = 0;
  this->Tolerance            = 0.0;
  this->AbsoluteTolerance    = 1.0;
  this->ConvertPolysToLines  = 1;
  this->ConvertLinesToPoints = 1;
  this->ConvertStripsToPolys = 1;
  this->Locator = NULL;
  this->PieceInvariant = 1;
  this->OutputPointsPrecision = vtkAlgorithm::DEFAULT_PRECISION;
}

//--------------------------------------------------------------------------
vtkCleanPolyData::~vtkCleanPolyData()
{
  this->SetLocator(NULL);
}

//--------------------------------------------------------------------------
void vtkCleanPolyData::OperateOnPoint(double in[3], double out[3])
{
  out[0] = in[0];
  out[1] = in[1];
  out[2] = in[2];
}

//--------------------------------------------------------------------------
void vtkCleanPolyData::OperateOnBounds(double in[6], double out[6])
{
  out[0] = in[0];
  out[1] = in[1];
  out[2] = in[2];
  out[3] = in[3];
  out[4] = in[4];
  out[5] = in[5];
}

//--------------------------------------------------------------------------
int vtkCleanPolyData::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  // get the info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  if (this->PieceInvariant)
    {
    outInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),
                 1);
    }
  else
    {
    outInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),
                 -1);
    }

  return 1;
}

//--------------------------------------------------------------------------
int vtkCleanPolyData::RequestUpdateExtent(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  if (this->PieceInvariant)
    {
    // Although piece > 1 is handled by superclass, we should be thorough.
    if (outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()) == 0)
      {
      inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
      inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
                  1);
      }
    else
      {
      inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
      inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
                  0);
      }
    }
  else
    {
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
                outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
                outInfo->Get(
                  vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
                outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()));
    }

  return 1;
}

//--------------------------------------------------------------------------
int vtkCleanPolyData::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPolyData *input = vtkPolyData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkPoints   *inPts = input->GetPoints();
  vtkIdType   numPts = input->GetNumberOfPoints();

  vtkDebugMacro(<<"Beginning PolyData clean");
  if ( (numPts<1) || (inPts == NULL ) )
    {
    vtkDebugMacro(<<"No data to Operate On!");
    return 1;
    }
  vtkIdType *updatedPts = new vtkIdType[input->GetMaxCellSize()];

  vtkIdType numNewPts;
  vtkIdType numUsedPts=0;
  vtkPoints *newPts = inPts->NewInstance();

  // Set the desired precision for the points in the output.
  if(this->OutputPointsPrecision == vtkAlgorithm::DEFAULT_PRECISION)
    {
    newPts->SetDataType(inPts->GetDataType());
    }
  else if(this->OutputPointsPrecision == vtkAlgorithm::SINGLE_PRECISION)
    {
    newPts->SetDataType(VTK_FLOAT);
    }
  else if(this->OutputPointsPrecision == vtkAlgorithm::DOUBLE_PRECISION)
    {
    newPts->SetDataType(VTK_DOUBLE);
    }

  newPts->Allocate(numPts);

  // we'll be needing these
  vtkIdType inCellID, newId;
  int i;
  vtkIdType ptId;
  vtkIdType npts = 0;
  vtkIdType *pts = 0;
  double x[3];
  double newx[3];
  vtkIdType *pointMap=0; //used if no merging

  vtkCellArray *inVerts  = input->GetVerts(),  *newVerts  = NULL;
  vtkCellArray *inLines  = input->GetLines(),  *newLines  = NULL;
  vtkCellArray *inPolys  = input->GetPolys(),  *newPolys  = NULL;
  vtkCellArray *inStrips = input->GetStrips(), *newStrips = NULL;

  vtkPointData *inputPD = input->GetPointData();
  vtkCellData  *inputCD = input->GetCellData();

  // We must be careful to 'operate' on the bounds of the locator so
  // that all inserted points lie inside it
  if ( this->PointMerging )
    {
    this->CreateDefaultLocator(input);
    if (this->ToleranceIsAbsolute)
      {
      this->Locator->SetTolerance(this->AbsoluteTolerance);
      }
    else
      {
      this->Locator->SetTolerance(this->Tolerance*input->GetLength());
      }
    double originalbounds[6], mappedbounds[6];
    input->GetBounds(originalbounds);
    this->OperateOnBounds(originalbounds,mappedbounds);
    this->Locator->InitPointInsertion(newPts, mappedbounds);
    }
  else
    {
    pointMap = new vtkIdType [numPts];
    for (i=0; i < numPts; i++)
      {
      pointMap[i] = -1; //initialize unused
      }
    }

  vtkPointData *outputPD = output->GetPointData();
  vtkCellData  *outputCD = output->GetCellData();
  outputPD->CopyAllocate(inputPD);
  outputCD->CopyAllocate(inputCD);

  // Celldata needs to be copied correctly. If a poly is converted to
  // a line, or a line to a point, then using a CellCounter will not
  // do, as the cells should be ordered verts, lines, polys,
  // strips. We need to maintain separate cell data lists so we can
  // copy them all correctly. Tedious but easy to implement. We can
  // use outputCD for vertex cell data, then add the rest at the end.
  vtkCellData  *outLineData = NULL;
  vtkCellData  *outPolyData = NULL;
  vtkCellData  *outStrpData = NULL;
  vtkIdType vertIDcounter = 0, lineIDcounter = 0;
  vtkIdType polyIDcounter = 0, strpIDcounter = 0;

  // Begin to adjust topology.
  //
  // Vertices are renumbered and we remove duplicates
  inCellID = 0;
  if ( !this->GetAbortExecute() && inVerts->GetNumberOfCells() > 0 )
    {
    newVerts = vtkCellArray::New();
    newVerts->Allocate(inVerts->GetSize());

    vtkDebugMacro(<<"Starting Verts "<<inCellID);
    for (inVerts->InitTraversal(); inVerts->GetNextCell(npts,pts);
         inCellID++)
      {
      for ( numNewPts=0, i=0; i < npts; i++ )
        {
        inPts->GetPoint(pts[i],x);
        this->OperateOnPoint(x, newx);
        if ( ! this->PointMerging )
          {
          if ( (ptId=pointMap[pts[i]]) == -1 )
            {
            pointMap[pts[i]] = ptId = numUsedPts++;
            newPts->SetPoint(ptId,newx);
            outputPD->CopyData(inputPD,pts[i],ptId);
            }
          }
        else if ( this->Locator->InsertUniquePoint(newx, ptId) )
          {
          outputPD->CopyData(inputPD,pts[i],ptId);
          }
        updatedPts[numNewPts++] = ptId;
        }//for all points of vertex cell

      if ( numNewPts > 0 )
        {
        newId = newVerts->InsertNextCell(numNewPts,updatedPts);
        outputCD->CopyData(inputCD, inCellID, newId);
        if ( vertIDcounter != newId)
          {
          vtkErrorMacro(<<"Vertex ID fault in vertex test");
          }
        vertIDcounter++;
      }
    }
  }
  this->UpdateProgress(0.25);

  // lines reduced to one point are eliminated or made into verts
  if ( !this->GetAbortExecute() && inLines->GetNumberOfCells() > 0 )
    {
    newLines = vtkCellArray::New();
    newLines->Allocate(inLines->GetSize());
    outLineData = vtkCellData::New();
    outLineData->CopyAllocate(inputCD);
    //
    vtkDebugMacro(<<"Starting Lines "<<inCellID);
    for (inLines->InitTraversal(); inLines->GetNextCell(npts,pts); inCellID++)
      {
      for ( numNewPts=0, i=0; i<npts; i++ )
        {
        inPts->GetPoint(pts[i],x);
        this->OperateOnPoint(x, newx);
        if ( ! this->PointMerging )
          {
          if ( (ptId=pointMap[pts[i]]) == -1 )
            {
            pointMap[pts[i]] = ptId = numUsedPts++;
            newPts->SetPoint(ptId,newx);
            outputPD->CopyData(inputPD,pts[i],ptId);
            }
          }
        else if ( this->Locator->InsertUniquePoint(newx, ptId) )
          {
          outputPD->CopyData(inputPD,pts[i],ptId);
          }
        if ( i == 0 || ptId != updatedPts[numNewPts-1] )
          {
          updatedPts[numNewPts++] = ptId;
          }
        }//for all cell points
      if ( (numNewPts>1) || !this->ConvertLinesToPoints )
        {
        newId = newLines->InsertNextCell(numNewPts,updatedPts);
        outLineData->CopyData(inputCD, inCellID, newId);
        if (lineIDcounter!=newId)
          {
          vtkErrorMacro(<<"Line ID fault in line test");
          }
        lineIDcounter++;
        }
      else if ( numNewPts==1 )
        {
        if (!newVerts)
          {
          newVerts = vtkCellArray::New();
          newVerts->Allocate(5);
          }
        newId = newVerts->InsertNextCell(numNewPts,updatedPts);
        outputCD->CopyData(inputCD, inCellID, newId);
        if (vertIDcounter!=newId)
          {
          vtkErrorMacro(<<"Vertex ID fault in line test");
          }
        vertIDcounter++;
        }
      }
    vtkDebugMacro(<<"Removed "
             << inLines->GetNumberOfCells() - newLines->GetNumberOfCells()
             << " lines");
    }
  this->UpdateProgress(0.50);

  // polygons reduced to two points or less are either eliminated
  // or converted to lines or points if enabled
  if ( !this->GetAbortExecute() && inPolys->GetNumberOfCells() > 0 )
    {
    newPolys = vtkCellArray::New();
    newPolys->Allocate(inPolys->GetSize());
    outPolyData = vtkCellData::New();
    outPolyData->CopyAllocate(inputCD);

    vtkDebugMacro(<<"Starting Polys "<<inCellID);
    for (inPolys->InitTraversal(); inPolys->GetNextCell(npts,pts); inCellID++)
      {
      for ( numNewPts=0, i=0; i<npts; i++ )
        {
        inPts->GetPoint(pts[i],x);
        this->OperateOnPoint(x, newx);
        if ( ! this->PointMerging )
          {
          if ( (ptId=pointMap[pts[i]]) == -1 )
            {
            pointMap[pts[i]] = ptId = numUsedPts++;
            newPts->SetPoint(ptId,newx);
            outputPD->CopyData(inputPD,pts[i],ptId);
            }
          }
        else if ( this->Locator->InsertUniquePoint(newx, ptId) )
          {
          outputPD->CopyData(inputPD,pts[i],ptId);
          }
        if ( i == 0 || ptId != updatedPts[numNewPts-1] )
          {
          updatedPts[numNewPts++] = ptId;
          }
        } //for points in cell
      if ( numNewPts>2 && updatedPts[0] == updatedPts[numNewPts-1] )
        {
        numNewPts--;
        }
      if ( (numNewPts > 2) || !this->ConvertPolysToLines )
        {
        newId = newPolys->InsertNextCell(numNewPts,updatedPts);
        outPolyData->CopyData(inputCD, inCellID, newId);
        if (polyIDcounter!=newId)
          {
          vtkErrorMacro(<<"Poly ID fault in poly test");
          }
        polyIDcounter++;
        }
      else if ( (numNewPts==2) || !this->ConvertLinesToPoints )
        {
        if (!newLines)
          {
          newLines = vtkCellArray::New();
          newLines->Allocate(5);
          outLineData = vtkCellData::New();
          outLineData->CopyAllocate(inputCD);
          }
        newId = newLines->InsertNextCell(numNewPts,updatedPts);
        outLineData->CopyData(inputCD, inCellID, newId);
        if (lineIDcounter!=newId)
          {
          vtkErrorMacro(<<"Line ID fault in poly test");
          }
        lineIDcounter++;
        }
      else if ( numNewPts==1 )
        {
        if (!newVerts)
          {
          newVerts = vtkCellArray::New();
          newVerts->Allocate(5);
          }
        newId = newVerts->InsertNextCell(numNewPts,updatedPts);
        outputCD->CopyData(inputCD, inCellID, newId);
        if (vertIDcounter!=newId)
          {
          vtkErrorMacro(<<"Vertex ID fault in poly test");
          }
        vertIDcounter++;
        }
      }
    vtkDebugMacro(<<"Removed "
           << inPolys->GetNumberOfCells() - newPolys->GetNumberOfCells()
           << " polys");
    }
  this->UpdateProgress(0.75);

  // triangle strips can reduced to polys/lines/points etc
  if ( !this->GetAbortExecute() && inStrips->GetNumberOfCells() > 0 )
    {
    newStrips = vtkCellArray::New();
    newStrips->Allocate(inStrips->GetSize());
    outStrpData = vtkCellData::New();
    outStrpData->CopyAllocate(inputCD);

    for (inStrips->InitTraversal(); inStrips->GetNextCell(npts,pts);
         inCellID++)
      {
      for ( numNewPts=0, i=0; i < npts; i++ )
        {
        inPts->GetPoint(pts[i],x);
        this->OperateOnPoint(x, newx);
        if ( ! this->PointMerging )
          {
          if ( (ptId=pointMap[pts[i]]) == -1 )
            {
            pointMap[pts[i]] = ptId = numUsedPts++;
            newPts->SetPoint(ptId,newx);
            outputPD->CopyData(inputPD,pts[i],ptId);
            }
          }
        else if ( this->Locator->InsertUniquePoint(newx, ptId) )
          {
          outputPD->CopyData(inputPD,pts[i],ptId);
          }
        if ( i == 0 || ptId != updatedPts[numNewPts-1] )
          {
          updatedPts[numNewPts++] = ptId;
          }
        }
      if ( (numNewPts > 3) || !this->ConvertStripsToPolys )
        {
        newId = newStrips->InsertNextCell(numNewPts,updatedPts);
        outStrpData->CopyData(inputCD, inCellID, newId);
        if (strpIDcounter!=newId)
          {
          vtkErrorMacro(<<"Strip ID fault in strip test");
          }
        strpIDcounter++;
        }
      else if ( (numNewPts==3) || !this->ConvertPolysToLines )
        {
        if (!newPolys)
          {
          newPolys = vtkCellArray::New();
          newPolys->Allocate(5);
          outPolyData = vtkCellData::New();
          outPolyData->CopyAllocate(inputCD);
          }
        newId = newPolys->InsertNextCell(numNewPts,updatedPts);
        outPolyData->CopyData(inputCD, inCellID, newId);
        if (polyIDcounter!=newId)
          {
          vtkErrorMacro(<<"Poly ID fault in strip test");
          }
        polyIDcounter++;
        }
      else if ( (numNewPts==2) || !this->ConvertLinesToPoints )
        {
        if (!newLines)
          {
          newLines = vtkCellArray::New();
          newLines->Allocate(5);
          outLineData = vtkCellData::New();
          outLineData->CopyAllocate(inputCD);
          }
        newId = newLines->InsertNextCell(numNewPts,updatedPts);
        outLineData->CopyData(inputCD, inCellID, newId);
        if (lineIDcounter!=newId)
          {
          vtkErrorMacro(<<"Line ID fault in strip test");
          }
        lineIDcounter++;
        }
      else if ( numNewPts==1 )
        {
        if (!newVerts)
          {
          newVerts = vtkCellArray::New();
          newVerts->Allocate(5);
          }
        newId = newVerts->InsertNextCell(numNewPts,updatedPts);
        outputCD->CopyData(inputCD, inCellID, newId);
        if (vertIDcounter!=newId)
          {
          vtkErrorMacro(<<"Vertex ID fault in strip test");
          }
        vertIDcounter++;
        }
      }
    vtkDebugMacro(<<"Removed "
              << inStrips->GetNumberOfCells() - newStrips->GetNumberOfCells()
              << " strips");
    }

  vtkDebugMacro(<<"Removed "
                << numPts - newPts->GetNumberOfPoints() << " points");

  // Update ourselves and release memory
  //
  delete [] updatedPts;
  if ( this->PointMerging )
    {
    this->Locator->Initialize(); //release memory.
    }
  else
    {
    newPts->SetNumberOfPoints(numUsedPts);
    delete [] pointMap;
    }

  // Now transfer all CellData from Lines/Polys/Strips into final
  // Cell data output
  int CombinedCellID = vertIDcounter;
  if (newLines)
    {
    for (i=0; i<lineIDcounter; i++, CombinedCellID++)
      {
      outputCD->CopyData(outLineData, i, CombinedCellID);
      }
    outLineData->Delete();
    }
  if (newPolys)
    {
    for (i=0; i<polyIDcounter; i++, CombinedCellID++)
      {
      outputCD->CopyData(outPolyData, i, CombinedCellID);
      }
    outPolyData->Delete();
    }
  if (newStrips)
    {
    for (i=0; i<strpIDcounter; i++, CombinedCellID++)
      {
      outputCD->CopyData(outStrpData, i, CombinedCellID);
      }
    outStrpData->Delete();
    }

  output->SetPoints(newPts);
  newPts->Squeeze();
  newPts->Delete();
  if (newVerts)
    {
    newVerts->Squeeze();
    output->SetVerts(newVerts);
    newVerts->Delete();
    }
  if (newLines)
    {
    newLines->Squeeze();
    output->SetLines(newLines);
    newLines->Delete();
    }
  if (newPolys)
    {
    newPolys->Squeeze();
    output->SetPolys(newPolys);
    newPolys->Delete();
    }
  if (newStrips)
    {
    newStrips->Squeeze();
    output->SetStrips(newStrips);
    newStrips->Delete();
    }

  return 1;
}

//--------------------------------------------------------------------------
// Method manages creation of locators. It takes into account the potential
// change of tolerance (zero to non-zero).
void vtkCleanPolyData::CreateDefaultLocator(vtkPolyData *input)
{
  double tol;
  if (this->ToleranceIsAbsolute)
    {
    tol = this->AbsoluteTolerance;
    }
  else
    {
    if (input)
      {
      tol = this->Tolerance*input->GetLength();
      }
    else
      {
      tol = this->Tolerance;
      }
    }

  if ( this->Locator == NULL)
    {
    if (tol==0.0)
      {
      this->Locator = vtkMergePoints::New();
      this->Locator->Register(this);
      this->Locator->Delete();
      }
    else
      {
      this->Locator = vtkPointLocator::New();
      this->Locator->Register(this);
      this->Locator->Delete();
      }
    }
  else
    {
    // check that the tolerance wasn't changed from zero to non-zero
    if ((tol>0.0) && (this->GetLocator()->GetTolerance()==0.0))
      {
      this->SetLocator(NULL);
      this->Locator = vtkPointLocator::New();
      this->Locator->Register(this);
      this->Locator->Delete();
      }
    }
}

//--------------------------------------------------------------------------
void vtkCleanPolyData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Point Merging: "
     << (this->PointMerging ? "On\n" : "Off\n");
  os << indent << "ToleranceIsAbsolute: "
     << (this->ToleranceIsAbsolute ? "On\n" : "Off\n");
  os << indent << "Tolerance: "
     << (this->Tolerance ? "On\n" : "Off\n");
  os << indent << "AbsoluteTolerance: "
     << (this->AbsoluteTolerance ? "On\n" : "Off\n");
  os << indent << "ConvertPolysToLines: "
     << (this->ConvertPolysToLines ? "On\n" : "Off\n");
  os << indent << "ConvertLinesToPoints: "
     << (this->ConvertLinesToPoints ? "On\n" : "Off\n");
  os << indent << "ConvertStripsToPolys: "
     << (this->ConvertStripsToPolys ? "On\n" : "Off\n");
  if ( this->Locator )
    {
    os << indent << "Locator: " << this->Locator << "\n";
    }
  else
    {
    os << indent << "Locator: (none)\n";
    }
  os << indent << "PieceInvariant: "
     << (this->PieceInvariant ? "On\n" : "Off\n");
  os << indent << "Output Points Precision: " << this->OutputPointsPrecision
     << "\n";
}

//--------------------------------------------------------------------------
unsigned long int vtkCleanPolyData::GetMTime()
{
  unsigned long mTime=this->vtkObject::GetMTime();
  unsigned long time;
  if ( this->Locator != NULL )
    {
    time = this->Locator->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }
  return mTime;
}

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStaticCleanPolyData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkStaticCleanPolyData.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkMergePoints.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStaticPointLocator.h"
#include "vtkArrayListTemplate.h" // For processing attribute data
#include "vtkSMPTools.h"

#include <algorithm>

vtkStandardNewMacro(vtkStaticCleanPolyData);

// These are created to support a (float,double) fast path. They work in
// tandem with vtkTemplate2Macro found in vtkSetGet.h.
#define vtkTemplate2MacroCP(call) \
  vtkTemplate2MacroCase1CP(VTK_DOUBLE, double, call);                           \
  vtkTemplate2MacroCase1CP(VTK_FLOAT, float, call);                             \
  vtkTemplate2MacroCase1CP(VTK_LONG_LONG, long long, call);                     \
  vtkTemplate2MacroCase1CP(VTK_UNSIGNED_LONG_LONG, unsigned long long, call);   \
  vtkTemplate2MacroCase1CP(VTK_ID_TYPE, vtkIdType, call);                       \
  vtkTemplate2MacroCase1CP(VTK_LONG, long, call);                               \
  vtkTemplate2MacroCase1CP(VTK_UNSIGNED_LONG, unsigned long, call);             \
  vtkTemplate2MacroCase1CP(VTK_INT, int, call);                                 \
  vtkTemplate2MacroCase1CP(VTK_UNSIGNED_INT, unsigned int, call);               \
  vtkTemplate2MacroCase1CP(VTK_SHORT, short, call);                             \
  vtkTemplate2MacroCase1CP(VTK_UNSIGNED_SHORT, unsigned short, call);           \
  vtkTemplate2MacroCase1CP(VTK_CHAR, char, call);                               \
  vtkTemplate2MacroCase1CP(VTK_SIGNED_CHAR, signed char, call);                 \
  vtkTemplate2MacroCase1CP(VTK_UNSIGNED_CHAR, unsigned char, call)
#define vtkTemplate2MacroCase1CP(type1N, type1, call) \
  vtkTemplate2MacroCase2(type1N, type1, VTK_DOUBLE, double, call);                 \
  vtkTemplate2MacroCase2(type1N, type1, VTK_FLOAT, float, call);

namespace { //anonymous

//----------------------------------------------------------------------------
// Fast, threaded way to copy new points and attribute data to output.
template <typename TPIn, typename TPOut>
struct CopyPoints
{
  vtkIdType *PtMap;
  TPIn  *InPts;
  TPOut *OutPts;
  ArrayList Arrays;

  CopyPoints(vtkIdType *ptMap, TPIn *inPts, vtkPointData *inPD,
             vtkIdType numNewPts, TPOut* outPts, vtkPointData *outPD) :
    PtMap(ptMap), InPts(inPts), OutPts(outPts)
  {
    this->Arrays.AddArrays(numNewPts,inPD,outPD);
  }

  void operator() (vtkIdType ptId, vtkIdType endPtId)
  {
    const TPIn *inP=this->InPts + 3*ptId;
    TPOut *outP;
    const vtkIdType *ptMap=this->PtMap;
    vtkIdType outPtId;

    for ( ; ptId < endPtId; ++ptId, inP+=3)
    {
      outPtId = ptMap[ptId];
      if ( outPtId != -1 )
      {
        outP = this->OutPts + 3*outPtId;
        *outP++ = static_cast<TPOut>(inP[0]);
        *outP++ = static_cast<TPOut>(inP[1]);
        *outP   = static_cast<TPOut>(inP[2]);
        this->Arrays.Copy(ptId,outPtId);
      }
    }
  }

  static void Execute(vtkIdType numPts, vtkIdType *ptMap, TPIn *inPts,
                      vtkPointData *inPD, vtkIdType numNewPts,
                      TPOut *outPts, vtkPointData *outPD)
  {
    CopyPoints copyPts(ptMap, inPts, inPD, numNewPts, outPts, outPD);
    vtkSMPTools::For(0,numPts, copyPts);
  }

};

} //anonymous namespace



//---------------------------------------------------------------------------
// Construct object with initial Tolerance of 0.0
vtkStaticCleanPolyData::vtkStaticCleanPolyData()
{
  this->ToleranceIsAbsolute  = 0;
  this->Tolerance            = 0.0;
  this->AbsoluteTolerance    = 1.0;
  this->ConvertPolysToLines  = 1;
  this->ConvertLinesToPoints = 1;
  this->ConvertStripsToPolys = 1;
  this->Locator = vtkStaticPointLocator::New();
  this->PieceInvariant = 1;
  this->OutputPointsPrecision = vtkAlgorithm::DEFAULT_PRECISION;
}

//--------------------------------------------------------------------------
vtkStaticCleanPolyData::~vtkStaticCleanPolyData()
{
  this->Locator->Delete();
  this->Locator = nullptr;
}


//--------------------------------------------------------------------------
int vtkStaticCleanPolyData::RequestUpdateExtent(
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
int vtkStaticCleanPolyData::RequestData(
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
  if ( (numPts<1) || (inPts == nullptr ) )
  {
    vtkDebugMacro(<<"No data to Operate On!");
    return 1;
  }
  vtkIdType *updatedPts = new vtkIdType[input->GetMaxCellSize()];

  // we'll be needing these
  vtkIdType inCellID, newId;
  int i;
  vtkIdType ptId;
  vtkIdType npts = 0;
  vtkIdType *pts = nullptr;

  vtkCellArray *inVerts  = input->GetVerts(),  *newVerts  = nullptr;
  vtkCellArray *inLines  = input->GetLines(),  *newLines  = nullptr;
  vtkCellArray *inPolys  = input->GetPolys(),  *newPolys  = nullptr;
  vtkCellArray *inStrips = input->GetStrips(), *newStrips = nullptr;

  vtkPointData *inPD = input->GetPointData();
  vtkCellData  *inCD = input->GetCellData();

  // The merge map indicates which points are merged with what points
  vtkIdType *mergeMap = new vtkIdType [numPts];
  this->Locator->SetDataSet(input);
  this->Locator->BuildLocator();
  double tol = ( this->ToleranceIsAbsolute ? this->AbsoluteTolerance :
                 this->Tolerance*input->GetLength() );
  this->Locator->MergePoints(tol,mergeMap);

  vtkPointData *outPD = output->GetPointData();
  vtkCellData  *outCD = output->GetCellData();
  outPD->CopyAllocate(inPD);
  outCD->CopyAllocate(inCD);

  // Prefix sum: count the number of new points; allocate memory. Populate the
  // point map (old points to new).
  vtkIdType *pointMap = new vtkIdType [numPts];
  vtkIdType id, numNewPts=0;
  // Count and map points to new points
  for ( id=0; id < numPts; ++id )
  {
    if ( mergeMap[id] == id )
    {
      pointMap[id] = numNewPts++;
    }
  }
  // Now map old merged points to new points
  for ( id=0; id < numPts; ++id )
  {
    if ( mergeMap[id] != id )
    {
      pointMap[id] = pointMap[mergeMap[id]];
    }
  }
  delete [] mergeMap;

  vtkPoints *newPts = inPts->NewInstance();
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
  newPts->SetNumberOfPoints(numNewPts);

  // Now copy points and point data (in parallel)
  void *inPtr = inPts->GetVoidPointer(0);
  int inPtsType = inPts->GetDataType();
  void *outPtr = newPts->GetVoidPointer(0);
  int outPtsType = newPts->GetDataType();

  switch (vtkTemplate2PackMacro(inPtsType, outPtsType))
  {
    vtkTemplate2MacroCP((CopyPoints<VTK_T1,VTK_T2>::Execute(numPts, pointMap,
                        (VTK_T1*)inPtr, inPD, numNewPts, (VTK_T2*)outPtr, outPD)));
    default:
      vtkErrorMacro(<<"Type not supported");
      return 0;
  }

  // Finally, remap the topology to use new point ids. Celldata needs to be
  // copied correctly. If a poly is converted to a line, or a line to a
  // point, then using a CellCounter will not do, as the cells should be
  // ordered verts, lines, polys, strips. We need to maintain separate cell
  // data lists so we can copy them all correctly. Tedious but easy to
  // implement. We can use outCD for vertex cell data, then add the rest
  // at the end.
  vtkCellData  *outLineData = nullptr;
  vtkCellData  *outPolyData = nullptr;
  vtkCellData  *outStrpData = nullptr;
  vtkIdType vertIDcounter = 0, lineIDcounter = 0;
  vtkIdType polyIDcounter = 0, strpIDcounter = 0;

  // Begin to adjust topology.
  //
  // Vertices are renumbered and we remove duplicates
  vtkIdType numCellPts;
  inCellID = 0;
  if ( !this->GetAbortExecute() && inVerts->GetNumberOfCells() > 0 )
  {
    newVerts = vtkCellArray::New();
    newVerts->Allocate(inVerts->GetSize());

    vtkDebugMacro(<<"Starting Verts "<<inCellID);
    for (inVerts->InitTraversal(); inVerts->GetNextCell(npts,pts);
         inCellID++)
    {
      for ( numCellPts=0, i=0; i < npts; i++ )
      {
        ptId = pointMap[pts[i]];
        updatedPts[numCellPts++] = ptId;
      }//for all points of vertex cell

      if ( numCellPts > 0 )
      {
        newId = newVerts->InsertNextCell(numCellPts,updatedPts);
        outCD->CopyData(inCD, inCellID, newId);
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
    outLineData->CopyAllocate(inCD);
    //
    vtkDebugMacro(<<"Starting Lines "<<inCellID);
    for (inLines->InitTraversal(); inLines->GetNextCell(npts,pts); inCellID++)
    {
      for ( numCellPts=0, i=0; i<npts; i++ )
      {
        ptId = pointMap[pts[i]];
        updatedPts[numCellPts++] = ptId;
      }//for all cell points

      if ( (numCellPts>1) || !this->ConvertLinesToPoints )
      {
        newId = newLines->InsertNextCell(numCellPts,updatedPts);
        outLineData->CopyData(inCD, inCellID, newId);
        if ( lineIDcounter != newId)
        {
          vtkErrorMacro(<<"Line ID fault in line test");
        }
        lineIDcounter++;
      }
      else if ( numCellPts==1 )
      {
        if (!newVerts)
        {
          newVerts = vtkCellArray::New();
          newVerts->Allocate(5);
        }
        newId = newVerts->InsertNextCell(numCellPts,updatedPts);
        outCD->CopyData(inCD, inCellID, newId);
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
    outPolyData->CopyAllocate(inCD);

    vtkDebugMacro(<<"Starting Polys "<<inCellID);
    for (inPolys->InitTraversal(); inPolys->GetNextCell(npts,pts); inCellID++)
    {
      for ( numCellPts=0, i=0; i<npts; i++ )
      {
        ptId = pointMap[pts[i]];
        updatedPts[numCellPts++] = ptId;
      } //for points in cell

      if ( numCellPts>2 && updatedPts[0] == updatedPts[numCellPts-1] )
      {
        numCellPts--;
      }
      if ( (numCellPts > 2) || !this->ConvertPolysToLines )
      {
        newId = newPolys->InsertNextCell(numCellPts,updatedPts);
        outPolyData->CopyData(inCD, inCellID, newId);
        if (polyIDcounter!=newId)
        {
          vtkErrorMacro(<<"Poly ID fault in poly test");
        }
        polyIDcounter++;
      }
      else if ( (numCellPts==2) || !this->ConvertLinesToPoints )
      {
        if (!newLines)
        {
          newLines = vtkCellArray::New();
          newLines->Allocate(5);
          outLineData = vtkCellData::New();
          outLineData->CopyAllocate(inCD);
        }
        newId = newLines->InsertNextCell(numCellPts,updatedPts);
        outLineData->CopyData(inCD, inCellID, newId);
        if (lineIDcounter!=newId)
        {
          vtkErrorMacro(<<"Line ID fault in poly test");
        }
        lineIDcounter++;
      }
      else if ( numCellPts==1 )
      {
        if (!newVerts)
        {
          newVerts = vtkCellArray::New();
          newVerts->Allocate(5);
        }
        newId = newVerts->InsertNextCell(numCellPts,updatedPts);
        outCD->CopyData(inCD, inCellID, newId);
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
    outStrpData->CopyAllocate(inCD);

    for (inStrips->InitTraversal(); inStrips->GetNextCell(npts,pts);
         inCellID++)
    {
      for ( numCellPts=0, i=0; i < npts; i++ )
      {
        ptId = pointMap[pts[i]];
        updatedPts[numCellPts++] = ptId;
      }
      if ( (numCellPts > 3) || !this->ConvertStripsToPolys )
      {
        newId = newStrips->InsertNextCell(numCellPts,updatedPts);
        outStrpData->CopyData(inCD, inCellID, newId);
        if (strpIDcounter!=newId)
        {
          vtkErrorMacro(<<"Strip ID fault in strip test");
        }
        strpIDcounter++;
      }
      else if ( (numCellPts==3) || !this->ConvertPolysToLines )
      {
        if (!newPolys)
        {
          newPolys = vtkCellArray::New();
          newPolys->Allocate(5);
          outPolyData = vtkCellData::New();
          outPolyData->CopyAllocate(inCD);
        }
        newId = newPolys->InsertNextCell(numCellPts,updatedPts);
        outPolyData->CopyData(inCD, inCellID, newId);
        if (polyIDcounter!=newId)
        {
          vtkErrorMacro(<<"Poly ID fault in strip test");
        }
        polyIDcounter++;
      }
      else if ( (numCellPts==2) || !this->ConvertLinesToPoints )
      {
        if (!newLines)
        {
          newLines = vtkCellArray::New();
          newLines->Allocate(5);
          outLineData = vtkCellData::New();
          outLineData->CopyAllocate(inCD);
        }
        newId = newLines->InsertNextCell(numCellPts,updatedPts);
        outLineData->CopyData(inCD, inCellID, newId);
        if (lineIDcounter!=newId)
        {
          vtkErrorMacro(<<"Line ID fault in strip test");
        }
        lineIDcounter++;
      }
      else if ( numCellPts==1 )
      {
        if (!newVerts)
        {
          newVerts = vtkCellArray::New();
          newVerts->Allocate(5);
        }
        newId = newVerts->InsertNextCell(numCellPts,updatedPts);
        outCD->CopyData(inCD, inCellID, newId);
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
                << numNewPts - newPts->GetNumberOfPoints() << " points");

  // Update ourselves and release memory
  //
  this->Locator->Initialize(); //release memory.
  delete [] updatedPts;
  delete [] pointMap;

  // Now transfer all CellData from Lines/Polys/Strips into final
  // Cell data output
  int CombinedCellID = vertIDcounter;
  if (newLines)
  {
    for (i=0; i<lineIDcounter; i++, CombinedCellID++)
    {
      outCD->CopyData(outLineData, i, CombinedCellID);
    }
    outLineData->Delete();
  }
  if (newPolys)
  {
    for (i=0; i<polyIDcounter; i++, CombinedCellID++)
    {
      outCD->CopyData(outPolyData, i, CombinedCellID);
    }
    outPolyData->Delete();
  }
  if (newStrips)
  {
    for (i=0; i<strpIDcounter; i++, CombinedCellID++)
    {
      outCD->CopyData(outStrpData, i, CombinedCellID);
    }
    outStrpData->Delete();
  }

  output->SetPoints(newPts);
  newPts->Delete();
  if (newVerts)
  {
    output->SetVerts(newVerts);
    newVerts->Delete();
  }
  if (newLines)
  {
    output->SetLines(newLines);
    newLines->Delete();
  }
  if (newPolys)
  {
    output->SetPolys(newPolys);
    newPolys->Delete();
  }
  if (newStrips)
  {
    output->SetStrips(newStrips);
    newStrips->Delete();
  }

  return 1;
}

//--------------------------------------------------------------------------
vtkMTimeType vtkStaticCleanPolyData::GetMTime()
{
  vtkMTimeType mTime=this->vtkObject::GetMTime();
  vtkMTimeType time = this->Locator->GetMTime();
  return ( time > mTime ? time : mTime );
}

//--------------------------------------------------------------------------
void vtkStaticCleanPolyData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

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

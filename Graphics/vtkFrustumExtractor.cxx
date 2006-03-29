/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFrustumExtractor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkFrustumExtractor.h"

#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkIdList.h"
#include "vtkImplicitFunction.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkUnstructuredGrid.h"
#include "vtkPlane.h"
#include "vtkPlanes.h"
#include "vtkCharArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkCellArray.h"
#include "vtkTimerLog.h"

vtkCxxRevisionMacro(vtkFrustumExtractor, "1.1");
vtkStandardNewMacro(vtkFrustumExtractor);
vtkCxxSetObjectMacro(vtkFrustumExtractor,Frustum,vtkPlanes);

//----------------------------------------------------------------------------
vtkFrustumExtractor::vtkFrustumExtractor(vtkPlanes *f)
{
  this->Frustum = f;
  if (this->Frustum)
    {
    this->Frustum->Register(this);
    }

  this->PassThrough = 0;
  this->IncludePartial = 1;
  this->AllowExecute = 1;
}

//----------------------------------------------------------------------------
vtkFrustumExtractor::~vtkFrustumExtractor()
{
  this->SetFrustum(NULL);
}

//----------------------------------------------------------------------------
// Overload standard modified time function. If implicit function is modified,
// then this object is modified as well.
unsigned long vtkFrustumExtractor::GetMTime()
{
  unsigned long mTime=this->MTime.GetMTime();
  unsigned long impFuncMTime;

  if ( this->Frustum != NULL )
    {
    impFuncMTime = this->Frustum->GetMTime();
    mTime = ( impFuncMTime > mTime ? impFuncMTime : mTime );
    }

  return mTime;
}

//----------------------------------------------------------------------------
int vtkFrustumExtractor::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  if (!this->AllowExecute)
    {
    //cerr << "Frustum Extractor " << this << " skipping" << endl;
    return 1;
    }
  //cerr << "Frustum Extractor " << this << " executing" << endl;

  if ( !this->Frustum )
    {
    vtkErrorMacro(<<"No frustum specified");
    return 0;
    }

  if ( this->Frustum->GetNumberOfPlanes() != 6)
    {
    vtkErrorMacro(<<"Frustum must have six planes.");
    return 0;
    }

  vtkDebugMacro(<< "Extracting geometry");

  if (this->PassThrough)
    {
    return this->ExecutePassThrough(inputVector, outputVector);
    }
  else
    {
    return this->ExecuteCreateUGrid(inputVector, outputVector);
    }
}

//----------------------------------------------------------------------------
//this method simply flags each point and cell as inside or outside
int vtkFrustumExtractor::ExecutePassThrough(
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
//cerr << "PASSTHROUGH {" << endl;

vtkTimerLog *timer = vtkTimerLog::New();
timer->StartTimer();

  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and ouptut
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet *output = vtkDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  //the output is a copy of the input, with two new arrays defined
  output->ShallowCopy(input);

  vtkIdType numPts, numCells, i;
  numPts = input->GetNumberOfPoints();
  numCells = input->GetNumberOfCells();

  vtkPointData *outputPD = output->GetPointData();
  vtkCharArray *pointInArray = vtkCharArray::New();
  pointInArray->SetNumberOfComponents(1);
  pointInArray->SetNumberOfTuples(numPts);
  for (i=0; i < numPts; i++)
    {
    pointInArray->SetValue(i, -1);
    }
  pointInArray->SetName("vtkInsidedness");
  outputPD->AddArray(pointInArray);
  outputPD->SetScalars(pointInArray);

  vtkCellData *outputCD = output->GetCellData();
  vtkCharArray *cellInArray = vtkCharArray::New();
  cellInArray->SetNumberOfComponents(1);
  cellInArray->SetNumberOfTuples(numCells);
  cellInArray->SetName("vtkInsidedness");
  outputCD->AddArray(cellInArray);
  outputCD->SetScalars(cellInArray);

  vtkIdType ptId, cellId;
  vtkIdList *cellPts;
  vtkCell *cell;
  int numCellPts;
  double x[3];

  // Loop over all points determining whether they are inside the
  // implicit function.
  //
  vtkUnsignedCharArray *planeTests = NULL;
  if (this->IncludePartial)
    {    
    planeTests = vtkUnsignedCharArray::New();
    planeTests->SetNumberOfComponents(1);
    planeTests->SetNumberOfTuples(numPts);
    }

  int NUMCELLS = 0;
  int NUMPTS = 0;

  if (planeTests != NULL)
    {
    vtkPlane *plane;
    for ( ptId=0; ptId < numPts; ptId++ )
      {
      input->GetPoint(ptId, x);      
      int pid;
      unsigned char result = 0;
      for (pid = 0; pid < 6; pid++)
        {
        plane = this->Frustum->GetPlane(pid);
        double dist = plane->EvaluateFunction(x);
        if (dist < 0.0)
          {
          //remember which planes this point passes
          result = result | (1<<pid);
          }
        }
      planeTests->SetValue(ptId, result);
      if (result == 0x3F)
        {
        //inside the frustum
        NUMPTS++;
        pointInArray->SetValue(ptId,1);
        }
      }
    }
  else
    {
    //quick and dirty point inside test
    for ( ptId=0; ptId < numPts; ptId++ )
      {
      input->GetPoint(ptId, x);      
      if ( (this->Frustum->FunctionValue(x)) < 0.0 )
        {
        NUMPTS++;
        pointInArray->SetValue(ptId,1);
        }
      }
    }

  timer->StopTimer();
  //cerr << "  " << timer->GetElapsedTime() << endl;
  timer->StartTimer();

  //cerr << "  @MID NUMPTS = " << NUMPTS << endl;

  // Now loop over all cells to see whether they are inside.
  for (cellId=0; cellId < numCells; cellId++)
    {
    cell = input->GetCell(cellId);
    cellPts = cell->GetPointIds();
    numCellPts = cell->GetNumberOfPoints();

    if (planeTests != NULL)
      {
      cellInArray->SetValue(cellId,-1);
      unsigned char cellresults = 0;
      for (i=0; i < numCellPts; i++)
        {
        ptId = cellPts->GetId(i);
        unsigned char testresults = planeTests->GetValue(ptId);
        cellresults = cellresults | testresults;        
        if (cellresults == 0x3F)
          {
          //a vert was completely inside or each plane has passed something 
          //accept and move on
          break;
          }
        }
      if (cellresults == 0x3F) 
        {
        NUMCELLS++;
        cellInArray->SetValue(cellId,1);
        
        //make sure all of the points for this cell are in the list,
        //any that were outside frustum were skipped before and
        //need to be added now
        for (i=0; i < numCellPts; i++)
          {
          ptId = cellPts->GetId(i);
          if (pointInArray->GetValue(ptId) < 0)
            {      
            NUMPTS++;
            pointInArray->SetValue(ptId,1);
            }
          }
        }
      }
    else
      {
      //quick and dirty, all points inside test
      cellInArray->SetValue(cellId,1);;
      NUMCELLS++;
      for (i=0; i < numCellPts; i++)
        {
        ptId = cellPts->GetId(i);
        if ( pointInArray->GetValue(ptId) < 0 )
          {
          NUMCELLS--;
          cellInArray->SetValue(cellId,-1);
          break; //this cell won't be inserted
          }
        }
      }      
    }//for all cells

  //cerr << "  @END NUMPTS = " << NUMPTS << endl;
  //cerr << "  @END NUMCELLS = " << NUMCELLS << endl;
  
  // Update ourselves and release memory
  //
  if (planeTests)
    {
    planeTests->Delete();
    }
  
  pointInArray->Delete();
  cellInArray->Delete();

timer->StopTimer();
//cerr << "  " << timer->GetElapsedTime() << endl;
timer->Delete();

//cerr << " } PASSTHROUGH" << endl;

  return 1;
}

//----------------------------------------------------------------------------
//this method creates a new unstructured grid containing only the inside points and cells
int vtkFrustumExtractor::ExecuteCreateUGrid(
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
//cerr << "CREATE {" << endl;
vtkTimerLog *timer = vtkTimerLog::New();
timer->StartTimer();

  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and ouptut
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkUnstructuredGrid *output = vtkUnstructuredGrid::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkIdType ptId, numPts, numCells, i, cellId, newCellId, newPointId;
  vtkIdType *pointMap;
  vtkIdList *cellPts;
  vtkCell *cell;
  int numCellPts;
  double x[3];
  vtkPoints *newPts;
  vtkIdList *newCellPts;
  vtkPointData *pd = input->GetPointData();
  vtkCellData *cd = input->GetCellData();
  vtkPointData *outputPD = output->GetPointData();
  vtkCellData *outputCD = output->GetCellData();
  int npts;
  
  newCellPts = vtkIdList::New();
  newCellPts->Allocate(VTK_CELL_SIZE);

  // Loop over all points determining whether they are inside the
  // implicit function. Copy the points and point data if they are.
  //
  numPts = input->GetNumberOfPoints();
  numCells = input->GetNumberOfCells();
  pointMap = new vtkIdType[numPts]; // maps old point ids into new
  for (i=0; i < numPts; i++)
    {
    pointMap[i] = -1;
    }

  output->Allocate(numCells/4); //allocate storage for geometry/topology
  newPts = vtkPoints::New();
  newPts->Allocate(numPts/4,numPts);
  outputPD->CopyAllocate(pd);
  outputCD->CopyAllocate(cd);
  vtkUnsignedCharArray *planeTests = NULL;
  if (this->IncludePartial)
    {    
    planeTests = vtkUnsignedCharArray::New();
    planeTests->SetNumberOfComponents(1);
    planeTests->SetNumberOfTuples(numPts);
    }

  int NUMCELLS = 0;
  int NUMPTS = 0;

  if (planeTests != NULL)
    {
    vtkPlane *plane;
    for ( ptId=0; ptId < numPts; ptId++ )
      {
      input->GetPoint(ptId, x);      
      int pid;
      unsigned char result = 0;
      for (pid = 0; pid < 6; pid++)
        {
        plane = this->Frustum->GetPlane(pid);
        double dist = plane->EvaluateFunction(x);
        if (dist < 0.0)
          {
          //remember which planes this passed
          result = result | (1<<pid);
          }
        }
      planeTests->SetValue(ptId, result);
      if (result == 0x3F)
        {
        //inside all six planes
        NUMPTS++;
        //inside the frustum
        newPointId = newPts->InsertNextPoint(x);
        pointMap[ptId] = newPointId;
        outputPD->CopyData(pd,ptId,newPointId);
        }
      }
    }
  else
    {
    //quick and dirty point inside test
    for ( ptId=0; ptId < numPts; ptId++ )
      {
      input->GetPoint(ptId, x);      
      if ( (this->Frustum->FunctionValue(x)) < 0.0 )
        {
        NUMPTS++;
        newPointId = newPts->InsertNextPoint(x);
        pointMap[ptId] = newPointId;
        outputPD->CopyData(pd,ptId,newPointId);
        }
      }
    }

  timer->StopTimer();
  //cerr << "  " << timer->GetElapsedTime() << endl;
  timer->StartTimer();

  //cerr << "  @MID NUMPTS = " << NUMPTS << endl;

  // Now loop over all cells to see whether they are inside.
  for (cellId=0; cellId < numCells; cellId++)
    {
    cell = input->GetCell(cellId);
    cellPts = cell->GetPointIds();
    numCellPts = cell->GetNumberOfPoints();
    
    newCellPts->Reset();
    if (planeTests != NULL)
      {
      unsigned char cellresults = 0;
      for ( npts=0, i=0; i < numCellPts; i++, npts++)
        {
        ptId = cellPts->GetId(i);
        unsigned char testresults = planeTests->GetValue(ptId);
        cellresults = cellresults | testresults;
        if (cellresults == 0x3F)
          {
          //a vert was completely inside or each plane has passed something
          break;
          }
        }
      if (cellresults == 0x3F) 
        {
        NUMCELLS++;
        //at least one vert passed each plane so the cell likely intersects 
        //frustum
        
        //make sure all of the points for this cell are in the list,
        //any that were outside frustum were skipped before and
        //need to be added now
        for ( npts=0, i=0; i < numCellPts; i++, npts++)
          {
          ptId = cellPts->GetId(i);
          newPointId = pointMap[ptId];
          if (newPointId < 0)
            {             
            NUMPTS++;
            input->GetPoint(ptId, x);      
            newPointId = newPts->InsertNextPoint(x);
            outputPD->CopyData(pd,ptId,newPointId);
            pointMap[ptId] = newPointId;
            }
          newCellPts->InsertId(i,newPointId);
          }
        }
      else
        {
        //some plane rejected all verts, this cell is completely outside
        npts = 0;
        }           
      }
    else
      {
      //quick and dirty, all points inside test
      for ( npts=0, i=0; i < numCellPts; i++, npts++)
        {
        ptId = cellPts->GetId(i);
        if ( pointMap[ptId] < 0 )
          {
          break; //this cell won't be inserted
          }
        else
          {
          NUMCELLS++;
          newCellPts->InsertId(i,pointMap[ptId]);
          }
        }
      }    
  
    if ( npts >= numCellPts  )
      {
      newCellId = output->InsertNextCell(cell->GetCellType(),newCellPts);
      outputCD->CopyData(cd,cellId,newCellId);
      }

    }//for all cells
  
  //cerr << "  @END NUMPTS = " << NUMPTS << endl;
  //cerr << "  @END NUMCELLS = " << NUMCELLS << endl;
  
  // Update ourselves and release memory
  //
  delete [] pointMap;
  newCellPts->Delete();
  output->SetPoints(newPts);
  newPts->Delete();
  if (planeTests)
    {
    planeTests->Delete();
    }
  
  output->Squeeze();

timer->StopTimer();
//cerr << "  " << timer->GetElapsedTime() << endl;
timer->Delete();
//cerr << "} CREATE" << endl;
  return 1;
}

//----------------------------------------------------------------------------
//needed because parent class sets output type to input type
//and we sometimes want to change it to make an UnstructuredGrid regardless of 
//input type
int vtkFrustumExtractor::RequestDataObject(
  vtkInformation*, 
  vtkInformationVector** inputVector , 
  vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
    {
    return 0;
    }
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  
  if (input)
    {
    // for each output
    for(int i=0; i < this->GetNumberOfOutputPorts(); ++i)
      {
      vtkInformation* info = outputVector->GetInformationObject(i);
      vtkDataSet *output = vtkDataSet::SafeDownCast(
        info->Get(vtkDataObject::DATA_OBJECT()));
    
      if (!output || 
          (this->PassThrough && !output->IsA(input->GetClassName())) ||
          (!this->PassThrough && !output->IsA("vtkUnstructuredGrid"))
        ) 
        {
        if (this->PassThrough)
          { 
          vtkDataSet* newOutput;
          newOutput = input->NewInstance();
          newOutput->SetPipelineInformation(info);
          newOutput->Delete();
          this->GetOutputPortInformation(0)->Set(
            vtkDataObject::DATA_EXTENT_TYPE(), newOutput->GetExtentType());
          }
        else
          {
          vtkUnstructuredGrid* newOutput = vtkUnstructuredGrid::New();
          newOutput->SetPipelineInformation(info);
          newOutput->Delete();
          this->GetOutputPortInformation(0)->Set(
            vtkDataObject::DATA_EXTENT_TYPE(), newOutput->GetExtentType());
          }
        }
      }
    return 1;
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkFrustumExtractor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Frustum: " 
     << (void *)this->Frustum << "\n";

  os << indent << "PassThrough: " 
     << (this->PassThrough ? "On\n" : "Off\n");

  os << indent << "IncludePartial: "
     << this->IncludePartial << "\n";

  os << indent << "AllowExecute: "
     << this->AllowExecute << "\n";
}

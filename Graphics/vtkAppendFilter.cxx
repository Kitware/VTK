/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAppendFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAppendFilter.h"

#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkDataSetAttributes.h"
#include "vtkDataSetCollection.h"
#include "vtkExecutive.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkUnstructuredGrid.h"
#include "vtkIncrementalOctreePointLocator.h"

vtkStandardNewMacro(vtkAppendFilter);

//----------------------------------------------------------------------------
vtkAppendFilter::vtkAppendFilter()
{
  this->InputList = NULL;
}

//----------------------------------------------------------------------------
vtkAppendFilter::~vtkAppendFilter()
{
  if (this->InputList != NULL)
    {
    this->InputList->Delete();
    this->InputList = NULL;
    }
}

//----------------------------------------------------------------------------
vtkDataSet *vtkAppendFilter::GetInput(int idx)
{
  if (idx >= this->GetNumberOfInputConnections(0) || idx < 0)
    {
    return NULL;
    }
  
  return vtkDataSet::SafeDownCast(
    this->GetExecutive()->GetInputData(0, idx));
}

//----------------------------------------------------------------------------
// Remove a dataset from the list of data to append.
void vtkAppendFilter::RemoveInput(vtkDataSet *ds)
{
  vtkAlgorithmOutput *algOutput = 0;
  if (ds)
    {
    algOutput = ds->GetProducerPort();
    }

  this->RemoveInputConnection(0, algOutput);
}

//----------------------------------------------------------------------------
vtkDataSetCollection *vtkAppendFilter::GetInputList()
{
  int idx;
  
  if (this->InputList)
    {
    this->InputList->Delete();
    }
  this->InputList = vtkDataSetCollection::New();
  
  for (idx = 0; idx < this->GetNumberOfInputConnections(0); ++idx)
    {
    if (this->GetInput(idx))
      {
      this->InputList->AddItem(this->GetInput(idx));
      }
    }  
  
  return this->InputList;
}

//----------------------------------------------------------------------------
// Append data sets into single unstructured grid
int vtkAppendFilter::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
   if ( inputVector[0]->GetNumberOfInformationObjects() > 0 )
     {
     vtkDataSet * tempData = 
     vtkDataSet::SafeDownCast
                 (  inputVector[0]->GetInformationObject( 0 )
                                  ->Get( vtkDataObject::DATA_OBJECT() )  );
     if (  tempData && tempData->GetCellData() &&
          !tempData->GetCellData()->GetArray( "vtkGhostLevels" )  ) 
       {
       tempData = NULL;
       return this->AppendBlocksWithPointLocator( inputVector, outputVector );
       }
     }
  
  // get the output info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the ouptut
  vtkUnstructuredGrid *output = vtkUnstructuredGrid::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkIdType numPts, numCells, ptOffset;
  int   tenth, count, abort=0;
  float decimal;
  vtkPoints *newPts;
  vtkPointData *pd;
  vtkCellData *cd;
  vtkIdList *ptIds, *newPtIds;
  int i, idx;
  vtkDataSet *ds;
  vtkIdType ptId, cellId, newCellId;
  vtkPointData *outputPD = output->GetPointData();
  vtkCellData *outputCD = output->GetCellData();

  vtkDebugMacro(<<"Appending data together");

  // Loop over all data sets, checking to see what data is common to 
  // all inputs. Note that data is common if 1) it is the same attribute 
  // type (scalar, vector, etc.), 2) it is the same native type (int, 
  // float, etc.), and 3) if a data array in a field, if it has the same name.
  count   = 0;
  decimal = 0.0;

  numPts = 0;
  numCells = 0;

  int numInputs = inputVector[0]->GetNumberOfInformationObjects();
  vtkDataSetAttributes::FieldList ptList(numInputs);
  vtkDataSetAttributes::FieldList cellList(numInputs);
  int firstPD=1;
  int firstCD=1;
  vtkInformation *inInfo = 0;
  for (idx = 0; idx < numInputs; ++idx)
    {
    inInfo = inputVector[0]->GetInformationObject(idx);
    ds = 0;
    if (inInfo)
      {
      ds = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
      }
    if (ds != NULL)
      {
      if ( ds->GetNumberOfPoints() <= 0 && ds->GetNumberOfCells() <= 0 )
        {
        continue; //no input, just skip
        }

      numPts += ds->GetNumberOfPoints();
      numCells += ds->GetNumberOfCells();

      pd = ds->GetPointData();
      if ( firstPD )
        {
        ptList.InitializeFieldList(pd);
        firstPD = 0;
        }
      else
        {
        ptList.IntersectFieldList(pd);
        }
      
      cd = ds->GetCellData();
      if ( firstCD )
        {
        cellList.InitializeFieldList(cd);
        firstCD = 0;
        }
      else
        {
        cellList.IntersectFieldList(cd);
        }
      }//if non-empty dataset
    }//for all inputs

  if ( numPts < 1)
    {
    vtkDebugMacro(<<"No data to append!");
    return 1;
    }
  
  // Now can allocate memory
  output->Allocate(numCells); //allocate storage for geometry/topology
  outputPD->CopyGlobalIdsOn();
  outputPD->CopyAllocate(ptList,numPts);
  outputCD->CopyGlobalIdsOn();
  outputCD->CopyAllocate(cellList,numCells);

  newPts = vtkPoints::New();
  newPts->SetNumberOfPoints(numPts);
  ptIds = vtkIdList::New(); ptIds->Allocate(VTK_CELL_SIZE);
  newPtIds = vtkIdList::New(); newPtIds->Allocate(VTK_CELL_SIZE);
  
  // Append each input dataset together
  //
  tenth = (numPts + numCells)/10 + 1;
  ptOffset=0;
  int inputCount = 0; // Since empty inputs are not in the list.
  for (idx = 0; idx < numInputs && !abort; ++idx)
    {
    inInfo = inputVector[0]->GetInformationObject(idx);
    ds = 0;
    if (inInfo)
      {
      ds = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
      }
    if ( ds != NULL &&
         (ds->GetNumberOfPoints() > 0 || ds->GetNumberOfCells() > 0) )
      {
      numPts = ds->GetNumberOfPoints();
      numCells = ds->GetNumberOfCells();
      pd = ds->GetPointData();
      
      // copy points and point data
      for (ptId=0; ptId < numPts && !abort; ptId++)
        {
        newPts->SetPoint(ptId+ptOffset,ds->GetPoint(ptId));
        outputPD->CopyData(ptList,pd,inputCount,ptId,ptId+ptOffset);
        
        // Update progress
        count++;
        if ( !(count % tenth) ) 
          {
          decimal += 0.1;
          this->UpdateProgress (decimal);
          abort = this->GetAbortExecute();
          }
        }
      
      cd = ds->GetCellData();
      // copy cell and cell data
      for (cellId=0; cellId < numCells && !abort; cellId++)
        {
        ds->GetCellPoints(cellId, ptIds);
        newPtIds->Reset ();
        for (i=0; i < ptIds->GetNumberOfIds(); i++)
          {
          newPtIds->InsertId(i,ptIds->GetId(i)+ptOffset);
          }
        newCellId = output->InsertNextCell(ds->GetCellType(cellId),newPtIds);
        outputCD->CopyData(cellList,cd,inputCount,cellId,newCellId);
        
        // Update progress
        count++;
        if ( !(count % tenth) ) 
          {
          decimal += 0.1;
          this->UpdateProgress (decimal);
          abort = this->GetAbortExecute();
          }
        }
      ptOffset+=numPts;
      ++inputCount;
      }
    }
  
  // Update ourselves and release memory
  //
  output->SetPoints(newPts);
  newPts->Delete();
  ptIds->Delete();
  newPtIds->Delete();

  return 1;
}
  
//----------------------------------------------------------------------------
int vtkAppendFilter::AppendBlocksWithPointLocator
 ( vtkInformationVector ** inputVector, vtkInformationVector  * outputVector )
{
  vtkInformation      * outInf = outputVector->GetInformationObject( 0 );
  vtkUnstructuredGrid * output = 
  vtkUnstructuredGrid::SafeDownCast
                       (  outInf->Get( vtkDataObject::DATA_OBJECT() )  );
  outInf = NULL;

  int            i, idx, tenth, count, abort = 0;
  float          decimal;
  double       * localBox    = NULL;
  double         dataBbox[6] = { VTK_DOUBLE_MAX, VTK_DOUBLE_MIN,
                                 VTK_DOUBLE_MAX, VTK_DOUBLE_MIN,
                                 VTK_DOUBLE_MAX, VTK_DOUBLE_MIN
                               };
  vtkIdType      ptId,   cellId;
  vtkIdType      numPts, numCells;
  vtkIdType      inputCount, ptOffset, cellOffset;           
  vtkIdList    * ptIds    = NULL;
  vtkIdList    * newPtIds = NULL;
  vtkPoints    * newPts   = NULL;
  vtkDataSet   * ds       = NULL;
  vtkCellData  * cd       = NULL;
  vtkCellData  * outputCD = output->GetCellData();
  vtkPointData * pd       = NULL;
  vtkPointData * outputPD = output->GetPointData();
                          
  vtkDebugMacro( <<"Appending data together" );

  // Loop over all data sets, checking to see what data is common to 
  // all inputs. Note that data is common if 1) it is the same attribute 
  // type (scalar, vector, etc.), 2) it is the same native type (int, 
  // float, etc.), and 3) if a data array in a field, if it has the same name.
  count    = 0;
  numPts   = 0;
  decimal  = 0.0;
  numCells = 0;
  
  int   firstPD   = 1;
  int   firstCD   = 1;
  int   numInputs = inputVector[0]->GetNumberOfInformationObjects();
  vtkInformation  * inInfo = NULL;
  vtkDataSetAttributes::FieldList ptList  ( numInputs );
  vtkDataSetAttributes::FieldList cellList( numInputs );
  
  for ( idx = 0; idx < numInputs; idx ++ )
    {
    ds     = NULL;
    inInfo = inputVector[0]->GetInformationObject( idx );
    if (inInfo)
      {
      ds = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
      }
      
    if ( ds != NULL )
      {
      if ( ds->GetNumberOfPoints() <= 0 && ds->GetNumberOfCells() <= 0 )
        {
        continue; //no input, just skip
        }

      numPts   += ds->GetNumberOfPoints();
      numCells += ds->GetNumberOfCells();
      
      // for merging duplicate points
      localBox    = ds->GetBounds();
      dataBbox[0] = ( localBox[0] < dataBbox[0] ) ? localBox[0] : dataBbox[0];
      dataBbox[2] = ( localBox[2] < dataBbox[2] ) ? localBox[2] : dataBbox[2];
      dataBbox[4] = ( localBox[4] < dataBbox[4] ) ? localBox[4] : dataBbox[4];
      dataBbox[1] = ( localBox[1] > dataBbox[1] ) ? localBox[1] : dataBbox[1];
      dataBbox[3] = ( localBox[3] > dataBbox[3] ) ? localBox[3] : dataBbox[3];
      dataBbox[5] = ( localBox[5] > dataBbox[5] ) ? localBox[5] : dataBbox[5];
      localBox    = NULL;

      pd = ds->GetPointData();
      if ( firstPD )
        {
        ptList.InitializeFieldList( pd );
        firstPD = 0;
        }
      else
        {
        ptList.IntersectFieldList( pd );
        }
      
      cd = ds->GetCellData();
      if ( firstCD )
        {
        cellList.InitializeFieldList( cd );
        firstCD = 0;
        }
      else
        {
        cellList.IntersectFieldList(cd);
        }
      }//if non-empty dataset
    }//for all inputs

  if ( numPts < 1 )
    {
    vtkDebugMacro( <<"No data to append!" );
    return 1;
    }
  
  // Now can allocate memory
  output->Allocate( numCells );
  newPts = vtkPoints::New();
  ptIds  = vtkIdList::New(); 
  ptIds->Allocate( VTK_CELL_SIZE );
  newPtIds = vtkIdList::New(); 
  newPtIds->Allocate( VTK_CELL_SIZE );
  
  // for merging duplicate points
  int             beInserted;
  vtkIdType       globalPtId;
  vtkIdType     * globalIdxs = new vtkIdType     [ numPts ];
  unsigned char * duplicated = new unsigned char [ numPts ];
  vtkIncrementalOctreePointLocator * ptInserter =
  vtkIncrementalOctreePointLocator::New();
  ptInserter->SetTolerance( 0.0 );
  ptInserter->InitPointInsertion( newPts, dataBbox );
  
  // append the blocks / pieces in terms of the geoemtry and topology
  count    = 0;
  tenth    = ( numPts + numCells ) / 10 + 1;
  ptOffset = 0;
  for ( idx = 0; idx < numInputs && !abort; idx ++ )
    {
    ds     = NULL;
    inInfo = inputVector[0]->GetInformationObject( idx );
    if ( inInfo )
      {
      ds = vtkDataSet::SafeDownCast
                       (  inInfo->Get( vtkDataObject::DATA_OBJECT() )  );
      }
      
    if (   ds != NULL &&
         ( ds->GetNumberOfPoints() > 0 || ds->GetNumberOfCells() > 0 )
       )
      {
      numPts   = ds->GetNumberOfPoints();
      numCells = ds->GetNumberOfCells();
      
      // copy points --- merge duplicate points if any
      for ( ptId = 0; ptId < numPts && !abort; ptId ++ )
        {
        beInserted = ptInserter->InsertUniquePoint
                     (  ds->GetPoint( ptId ),  globalPtId  );
        globalIdxs[ ptId + ptOffset ] = globalPtId;
        duplicated[ ptId + ptOffset ] = static_cast <unsigned char> 
                                        ( 1 - beInserted );
        
        count ++;
        if (  !( count % tenth )  ) 
          {
          decimal += 0.1;
          this->UpdateProgress ( decimal );
          abort    = this->GetAbortExecute();
          }
        }
      
      // copy cells --- using the new (global) point Ids
      for ( cellId = 0; cellId < numCells && !abort; cellId ++ )
        {
        newPtIds->Reset();
        ds->GetCellPoints( cellId, ptIds );
        for ( i = 0; i < ptIds->GetNumberOfIds(); i ++ )
          {
          newPtIds->InsertId(  i,  globalIdxs[ ptIds->GetId(i) + ptOffset ]  );
          }
        output->InsertNextCell(  ds->GetCellType( cellId ),  newPtIds  );
        
        count ++;
        if (  !( count % tenth )  ) 
          {
          decimal += 0.1;
          this->UpdateProgress( decimal );
          abort    = this->GetAbortExecute();
          }
        }
      
      ptOffset += numPts;
      }
    }
  
  // copy the associated point data and cell data
  count      = 0;  
  numPts     = newPts->GetNumberOfPoints(); // unique points
  tenth      = ( numPts + numCells ) / 10 + 1;
  ptOffset   = 0;
  cellOffset = 0;
  inputCount = 0;
  outputPD->CopyGlobalIdsOn();
  outputPD->CopyAllocate( ptList, numPts );
  outputCD->CopyGlobalIdsOn();
  outputCD->CopyAllocate( cellList, numCells );
  
  for ( idx = 0; idx < numInputs && !abort; idx ++ )
    {
    ds     = NULL;
    inInfo = inputVector[0]->GetInformationObject( idx );
    if ( inInfo )
      {
      ds = vtkDataSet::SafeDownCast
                       (  inInfo->Get( vtkDataObject::DATA_OBJECT() )  );
      }
      
    if (   ds != NULL &&
         ( ds->GetNumberOfPoints() > 0 || ds->GetNumberOfCells() > 0 ) 
       )
      {
      numPts   = ds->GetNumberOfPoints();
      numCells = ds->GetNumberOfCells();
      
      // copy point data
      pd = ds->GetPointData();
      for ( ptId = 0; ptId < numPts && !abort; ptId ++ )
        {
        if (  duplicated[ ptId + ptOffset ]  ==  0  )
          {
          outputPD->CopyData(  ptList,  pd,  inputCount,  ptId, 
                               globalIdxs[ ptId + ptOffset ]  );
          }
        
        count ++;
        if (  !( count % tenth )  ) 
          {
          decimal += 0.1;
          this->UpdateProgress( decimal );
          abort    = this->GetAbortExecute();
          }
        }
      
      // copy cell data
      cd = ds->GetCellData();
      for ( cellId = 0; cellId < numCells && !abort; cellId ++ )
        {
        outputCD->CopyData( cellList, cd, inputCount, cellId, 
                            cellOffset + cellId );
        
        count ++;
        if (  !( count % tenth )  ) 
          {
          decimal += 0.1;
          this->UpdateProgress ( decimal );
          abort    = this->GetAbortExecute();
          }
        }
        
      inputCount ++;
      ptOffset   += numPts;
      cellOffset += numCells;
      }
    }
  
  // attach the points
  output->SetPoints( newPts );
  
  // memory deallocation
  delete [] duplicated;
  delete [] globalIdxs;
  ptInserter->Delete();
  newPtIds->Delete();
  newPts->Delete();
  ptIds->Delete();
  
  duplicated = NULL;
  globalIdxs = NULL;
  ptInserter = NULL;
  newPtIds   = NULL;
  outputPD   = NULL;
  outputCD   = NULL;
  output     = NULL;
  newPts     = NULL;
  inInfo     = NULL;
  ptIds      = NULL;
  pd         = NULL;
  cd         = NULL;
  ds         = NULL;
  
  return 1;
}

//----------------------------------------------------------------------------
int vtkAppendFilter::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
  return 1;
}

//----------------------------------------------------------------------------
void vtkAppendFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

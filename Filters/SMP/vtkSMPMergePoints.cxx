/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMPMergePoints.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/

#include "vtkSMPMergePoints.h"
#include "vtkPoints.h"
#include "vtkIdList.h"
#include "vtkObjectFactory.h"
#include "vtkFloatArray.h"
#include "vtkPointData.h"

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMPMergePoints)

//------------------------------------------------------------------------------
vtkSMPMergePoints::vtkSMPMergePoints()
{
}

//------------------------------------------------------------------------------
vtkSMPMergePoints::~vtkSMPMergePoints()
{
}

//------------------------------------------------------------------------------
void vtkSMPMergePoints::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkSMPMergePoints::InitializeMerge()
{
  this->AtomicInsertionId = this->InsertionPointId;
}

//------------------------------------------------------------------------------
void vtkSMPMergePoints::Merge(vtkSMPMergePoints* locator,
                              vtkIdType idx,
                              vtkPointData* outPd,
                              vtkPointData* ptData,
                              vtkIdList* idList )
{
  if ( !locator->HashTable[idx] )
  {
    return;
  }

  vtkIdType i;
  vtkIdList *bucket, *oldIdToMerge;
  vtkFloatArray* floatOldDataArray = 0;

  if ( !(bucket = this->HashTable[idx]) )
  {
    this->HashTable[idx] = bucket = vtkIdList::New();
    bucket->Allocate( this->NumberOfPointsPerBucket/2,
                      this->NumberOfPointsPerBucket/3 );
    oldIdToMerge = locator->HashTable[idx];
    oldIdToMerge->Register( this );
    if ( this->Points->GetData()->GetDataType() == VTK_FLOAT )
    {
      floatOldDataArray = static_cast<vtkFloatArray*>( locator->Points->GetData() );
    }
  }
  else
  {
    oldIdToMerge = vtkIdList::New();

    int nbOfIds = bucket->GetNumberOfIds ();
    int nbOfOldIds = locator->HashTable[idx]->GetNumberOfIds();
    oldIdToMerge->Allocate( nbOfOldIds );

    vtkDataArray *dataArray = this->Points->GetData();
    vtkDataArray *oldDataArray = locator->Points->GetData();
    vtkIdType *idArray = bucket->GetPointer(0);
    vtkIdType *idOldArray = locator->HashTable[idx]->GetPointer(0);

    bool found;

    if (dataArray->GetDataType() == VTK_FLOAT)
    {
      vtkFloatArray* floatDataArray = static_cast<vtkFloatArray*>(dataArray);
      floatOldDataArray = static_cast<vtkFloatArray*>(oldDataArray);

      for ( int oldIdIdx = 0; oldIdIdx < nbOfOldIds; ++oldIdIdx )
      {
        found = false;
        vtkIdType oldId = idOldArray[oldIdIdx];
        float *x = floatOldDataArray->GetPointer(0) + 3*oldId;
        float *pt;
        for ( i=0; i < nbOfIds; i++ )
        {
          vtkIdType existingId = idArray[i];
          pt = floatDataArray->GetPointer(0) + 3*existingId;
          if ( x[0] == pt[0] && x[1] == pt[1] && x[2] == pt[2] )
          {
            // point is already in the list, return 0 and set the id parameter
            found = true;
            idList->SetId( oldId, existingId );
            break;
          }
        }
        if ( !found )
        {
          oldIdToMerge->InsertNextId( oldId );
        }
      }
    }
    else
    {
      for ( int oldIdIdx = 0; oldIdIdx < nbOfOldIds; ++oldIdIdx )
      {
        found = false;
        vtkIdType oldId = idOldArray[oldIdIdx];
        double *x = oldDataArray->GetTuple( oldId );
        double *pt;
        for ( i=0; i < nbOfIds; i++ )
        {
          vtkIdType existingId = idArray[i];
          pt = dataArray->GetTuple( existingId );
          if ( x[0] == pt[0] && x[1] == pt[1] && x[2] == pt[2] )
          {
            // point is already in the list, return 0 and set the id parameter
            found = true;
            idList->SetId( oldId, existingId );
            break;
          }
        }
        if ( !found )
        {
          oldIdToMerge->InsertNextId( oldId );
        }
      }
    }
  }

  // points have to be added
  vtkIdType NumberOfInsertions = oldIdToMerge->GetNumberOfIds();
  vtkIdType first_id = (this->AtomicInsertionId += NumberOfInsertions);
  bucket->Resize( bucket->GetNumberOfIds() + NumberOfInsertions );
  for ( i = 0; i < NumberOfInsertions; ++i )
  {
    vtkIdType newId = first_id + i, oldId = oldIdToMerge->GetId( i );
    idList->SetId( oldId, newId );
    bucket->InsertNextId( newId );
    if ( floatOldDataArray )
    {
      const float *pt = floatOldDataArray->GetPointer(0) + 3*oldId;
      this->Points->SetPoint( newId, pt );
    }
    else
    {
      this->Points->SetPoint( newId, locator->Points->GetPoint( oldId ) );
    }
    outPd->SetTuple( newId, oldId, ptData );
  }
  oldIdToMerge->UnRegister( this );
}

//------------------------------------------------------------------------------
void vtkSMPMergePoints::FixSizeOfPointArray()
{
  this->Points->SetNumberOfPoints(this->AtomicInsertionId);
}

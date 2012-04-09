/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOBBDicer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOBBDicer.h"

#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkOBBTree.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkShortArray.h"

vtkStandardNewMacro(vtkOBBDicer);

void vtkOBBDicer::BuildTree(vtkIdList *ptIds, vtkOBBNode *OBBptr,
                            vtkDataSet *input)
{
  vtkIdType i, numPts=ptIds->GetNumberOfIds();
  vtkIdType ptId;
  vtkOBBTree *OBB = vtkOBBTree::New();

  double size[3];

  // Gather all the points into a single list
  //
  for ( this->PointsList->Reset(), i=0; i < numPts; i++ )
    {
    ptId = ptIds->GetId(i);
    this->PointsList->InsertNextPoint(input->GetPoint(ptId));
    }

  // Now compute the OBB
  //
  OBB->ComputeOBB(this->PointsList, OBBptr->Corner, OBBptr->Axes[0],
                  OBBptr->Axes[1], OBBptr->Axes[2], size);
  OBB->Delete();

  // Check whether to continue recursing; if so, create two children and
  // assign cells to appropriate child.
  //
  if ( numPts > this->NumberOfPointsPerPiece )
    {
    vtkOBBNode *LHnode= new vtkOBBNode;
    vtkOBBNode *RHnode= new vtkOBBNode;
    OBBptr->Kids = new vtkOBBNode *[2];
    OBBptr->Kids[0] = LHnode;
    OBBptr->Kids[1] = RHnode;
    vtkIdList *LHlist = vtkIdList::New();
    LHlist->Allocate(numPts/2);
    vtkIdList *RHlist = vtkIdList::New();
    RHlist->Allocate(numPts/2);
    LHnode->Parent = OBBptr;
    RHnode->Parent = OBBptr;
    double n[3], p[3], x[3], val;

    //split the longest axis down the middle
    for (i=0; i < 3; i++) //compute split point
      {
      p[i] = OBBptr->Corner[i] + OBBptr->Axes[0][i]/2.0 +
             OBBptr->Axes[1][i]/2.0 + OBBptr->Axes[2][i]/2.0;
      }

    // compute split normal
    for (i=0 ; i < 3; i++)
      {
      n[i] = OBBptr->Axes[0][i];
      }
    vtkMath::Normalize(n);

    //traverse cells, assigning to appropriate child list as necessary
    for ( i=0; i < numPts; i++ )
      {
      ptId = ptIds->GetId(i);
      input->GetPoint(ptId, x);
      val = n[0]*(x[0]-p[0]) + n[1]*(x[1]-p[1]) + n[2]*(x[2]-p[2]);

      if ( val < 0.0 )
        {
        LHlist->InsertNextId(ptId);
        }
      else
        {
        RHlist->InsertNextId(ptId);
        }

      }//for all points

    ptIds->Delete(); //don't need to keep anymore
    this->BuildTree(LHlist, LHnode, input);
    this->BuildTree(RHlist, RHnode, input);
    }//if should build tree

  else //terminate recursion
    {
    ptIds->Squeeze();
    OBBptr->Cells = ptIds;
    }
}

// Current implementation uses an OBBTree to split up the dataset.
int vtkOBBDicer::RequestData(
  vtkInformation *,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  vtkIdType ptId, numPts;
  vtkIdList *ptIds;
  vtkShortArray *groupIds;
  vtkOBBNode *root;
  vtkDataSet *input= vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet *output= vtkDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDebugMacro(<<"Dicing object");

  // First, copy the input to the output as a starting point
  output->CopyStructure( input );

  if ( (numPts = input->GetNumberOfPoints()) < 1 )
    {
    vtkErrorMacro(<<"No data to dice!");
    return 1;
    }

  // The superclass computes piece size limits based on filter ivars
  this->UpdatePieceMeasures(input);

  // Create list of points
  //
  this->PointsList = vtkPoints::New();
  this->PointsList->Allocate(numPts);
  ptIds = vtkIdList::New();
  ptIds->SetNumberOfIds(numPts);
  for ( ptId=0; ptId < numPts; ptId++ )
    {
    ptIds->SetId(ptId,ptId);
    }

  root = new vtkOBBNode;
  this->BuildTree(ptIds,root, input);

  // Generate scalar values
  //
  this->PointsList->Delete();
  this->PointsList = NULL;
  groupIds = vtkShortArray::New();
  groupIds->SetNumberOfTuples(numPts);
  groupIds->SetName("vtkOBBDicer_GroupIds");
  this->NumberOfActualPieces = 0;
  this->MarkPoints(root,groupIds);
  this->DeleteTree(root);
  delete root;

  vtkDebugMacro(<<"Created " << this->NumberOfActualPieces << " pieces");

  // Update self
  //
  if ( this->FieldData )
    {
    output->GetPointData()->AddArray(groupIds);
    output->GetPointData()->CopyFieldOff("vtkOBBDicer_GroupIds");
    output->GetPointData()->PassData(input->GetPointData());
    }
  else
    {
    output->GetPointData()->AddArray(groupIds);
    output->GetPointData()->SetActiveScalars(groupIds->GetName());
    output->GetPointData()->CopyScalarsOff();
    output->GetPointData()->PassData(input->GetPointData());
    }

  output->GetCellData()->PassData(input->GetCellData());

  groupIds->Delete();

  return 1;
}

void vtkOBBDicer::MarkPoints(vtkOBBNode *OBBptr, vtkShortArray *groupIds)
{
  if ( OBBptr->Kids == NULL ) //leaf OBB
    {
    vtkIdList *ptIds;
    vtkIdType i, ptId, numIds;

    ptIds = OBBptr->Cells;
    if ( (numIds=ptIds->GetNumberOfIds()) > 0 )
      {
      for ( i=0; i < numIds; i++ )
        {
        ptId = ptIds->GetId(i);
        groupIds->SetValue(ptId,this->NumberOfActualPieces);
        }
      this->NumberOfActualPieces++;
      }//if any points in this leaf OBB
    }
  else
    {
    this->MarkPoints(OBBptr->Kids[0],groupIds);
    this->MarkPoints(OBBptr->Kids[1],groupIds);
    }
}

void vtkOBBDicer::DeleteTree(vtkOBBNode *OBBptr)
{
  if ( OBBptr->Kids != NULL )
    {
    this->DeleteTree(OBBptr->Kids[0]);
    this->DeleteTree(OBBptr->Kids[1]);
    delete OBBptr->Kids[0];
    delete OBBptr->Kids[1];
    }
}

void vtkOBBDicer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOBBDicer.cxx
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
#include "vtkOBBDicer.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkShortArray.h"


//------------------------------------------------------------------------------
vtkOBBDicer* vtkOBBDicer::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkOBBDicer");
  if(ret)
    {
    return (vtkOBBDicer*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkOBBDicer;
}




void vtkOBBDicer::BuildTree(vtkIdList *ptIds, vtkOBBNode *OBBptr)
{
  vtkIdType i, numPts=ptIds->GetNumberOfIds();
  vtkIdType ptId;
  vtkOBBTree *OBB = vtkOBBTree::New();
  vtkDataSet *input= this->GetInput();

  float size[3];

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
  OBB = NULL;
  
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
    float n[3], p[3], *x, val;

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
      x = input->GetPoint(ptId);
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
    this->BuildTree(LHlist, LHnode);
    this->BuildTree(RHlist, RHnode);
    }//if should build tree

  else //terminate recursion
    {
    ptIds->Squeeze();
    OBBptr->Cells = ptIds;
    }
}

// Current implementation uses an OBBTree to split up the dataset.
void vtkOBBDicer::Execute()
{
  vtkIdType ptId, numPts;
  vtkIdList *ptIds;
  vtkShortArray *groupIds;
  vtkOBBNode *root;
  vtkDataSet *input= this->GetInput();
  vtkDataSet *output= this->GetOutput();

  vtkDebugMacro(<<"Dicing object");

  // First, copy the input to the output as a starting point
  output->CopyStructure( input );

  if ( (numPts = input->GetNumberOfPoints()) < 1 )
    {
    vtkErrorMacro(<<"No data to dice!");
    return;
    }

  // The superclass computes piece size limits based on filter ivars
  this->UpdatePieceMeasures();

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
  this->BuildTree(ptIds,root);

  // Generate scalar values
  //
  this->PointsList->Delete(); 
  this->PointsList = NULL;
  groupIds = vtkShortArray::New();
  groupIds->SetNumberOfTuples(numPts);
  this->NumberOfActualPieces = 0;
  this->MarkPoints(root,groupIds);
  this->DeleteTree(root);
  delete root;
  
  vtkDebugMacro(<<"Created " << this->NumberOfActualPieces << " pieces");

  // Update self
  //
  if ( this->FieldData )
    {
    groupIds->SetName("vtkOBBDicer_GroupIds");
    output->GetPointData()->AddArray(groupIds);
    output->GetPointData()->CopyFieldOff("vtkOBBDicer_GroupIds");
    output->GetPointData()->PassData(input->GetPointData());
    }
  else
    {
    output->GetPointData()->SetScalars(groupIds);
    output->GetPointData()->CopyScalarsOff();
    output->GetPointData()->PassData(input->GetPointData());
    }

  output->GetCellData()->PassData(input->GetCellData());

  groupIds->Delete();
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
  vtkDicer::PrintSelf(os,indent);
}

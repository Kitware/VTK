/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDicer.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkDicer.h"
#include "vtkMath.h"
#include "vtkShortScalars.h"

// Description:
// Create object with 5000 points per piece.
vtkDicer::vtkDicer()
{
  this->NumberOfPointsPerPiece = 5000;
  this->NumberOfPieces = 0;
}

void vtkDicer::BuildTree(vtkIdList *ptIds, vtkOBBNode *OBBptr)
{
  int i, numPts=ptIds->GetNumberOfIds();
  int ptId;
  static vtkOBBTree OBB;

  float size[3];

  //
  // Gather all the points into a single list
  //
  for ( this->PointsList->Reset(), i=0; i < numPts; i++ )
    {
    ptId = ptIds->GetId(i);
    this->PointsList->InsertNextPoint(this->Input->GetPoint(ptId));
    }

  //
  // Now compute the OBB
  //
  OBB.ComputeOBB(this->PointsList, OBBptr->Corner, OBBptr->Axes[0], 
                  OBBptr->Axes[1], OBBptr->Axes[2], size);

  //
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
    for (i=0 ; i < 3; i++) n[i] = OBBptr->Axes[0][i];
    vtkMath::Normalize(n);

    //traverse cells, assigning to appropriate child list as necessary
    for ( i=0; i < numPts; i++ )
      {
      ptId = ptIds->GetId(i);
      x = this->Input->GetPoint(ptId);
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

    delete ptIds; //don't need to keep anymore
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
void vtkDicer::Execute()
{
  int ptId, numPts;
  vtkIdList *ptIds;
  vtkShortScalars *groupIds;
  vtkOBBNode *root;

  vtkDebugMacro(<<"Dicing object");

  if ( (numPts = this->Input->GetNumberOfPoints()) < 1 )
    {
    vtkErrorMacro(<<"No data to dice!");
    return;
    }

  //
  // Create list of points
  //
  this->PointsList = vtkFloatPoints::New();
  this->PointsList->Allocate(numPts);
  ptIds = vtkIdList::New();
  ptIds->SetNumberOfIds(numPts);
  for ( ptId=0; ptId < numPts; ptId++ )
    {
    ptIds->SetId(ptId,ptId);
    }

  root = new vtkOBBNode;
  this->BuildTree(ptIds,root);

  //
  // Generate scalar values
  //
  this->PointsList->Delete(); this->PointsList = NULL;
  groupIds = vtkShortScalars::New();
  groupIds->SetNumberOfScalars(numPts);
  this->NumberOfPieces = 0;
  this->MarkPoints(root,groupIds);
  this->DeleteTree(root);

  vtkDebugMacro(<<"Created " << this->NumberOfPieces << " pieces");
//
// Update self
//
  this->Output->GetPointData()->CopyScalarsOff();
  this->Output->GetPointData()->PassData(this->Input->GetPointData());

  this->Output->GetPointData()->SetScalars(groupIds);
  groupIds->Delete();
}

void vtkDicer::MarkPoints(vtkOBBNode *OBBptr, vtkShortScalars *groupIds)
{
  if ( OBBptr->Kids == NULL ) //leaf OBB
    {
    vtkIdList *ptIds;
    int i, ptId, numIds;

    ptIds = OBBptr->Cells;
    if ( (numIds=ptIds->GetNumberOfIds()) > 0 )
      {
      for ( i=0; i < numIds; i++ )
        {
        ptId = ptIds->GetId(i);
        groupIds->SetScalar(ptId,this->NumberOfPieces);
        }
      this->NumberOfPieces++;
      }//if any points in this leaf OBB
    }
  else
    {
    this->MarkPoints(OBBptr->Kids[0],groupIds);
    this->MarkPoints(OBBptr->Kids[1],groupIds);
    }
}

void vtkDicer::DeleteTree(vtkOBBNode *OBBptr)
{
  if ( OBBptr->Kids != NULL )
    {
    this->DeleteTree(OBBptr->Kids[0]);
    this->DeleteTree(OBBptr->Kids[1]);
    delete OBBptr->Kids[0];
    delete OBBptr->Kids[1];
    }
}

void vtkDicer::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToDataSetFilter::PrintSelf(os,indent);

  os << indent << "Number of Points per Piece: " << this->NumberOfPointsPerPiece << "\n";;
  os << indent << "Number of Pieces: " << this->NumberOfPieces << "\n";;
}

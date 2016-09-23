/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperOctreeDepth.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHyperOctreeDepth.h"

#include "vtkObjectFactory.h"
#include "vtkHyperOctree.h"
#include "vtkHyperOctreeCursor.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkIntArray.h"

vtkStandardNewMacro(vtkHyperOctreeDepth);

//----------------------------------------------------------------------------
vtkHyperOctreeDepth::vtkHyperOctreeDepth()
{
  this->GeneratedDepths = 0;
}

//----------------------------------------------------------------------------
vtkHyperOctreeDepth::~vtkHyperOctreeDepth()
{
  if (this->GeneratedDepths)
  {
    this->GeneratedDepths->Delete();
    this->GeneratedDepths = 0;
  }
}

//----------------------------------------------------------------------------
int vtkHyperOctreeDepth::RequestData(vtkInformation *vtkNotUsed(request),
                                      vtkInformationVector **inputVector,
                                      vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  this->Input = vtkHyperOctree::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  this->Output=vtkHyperOctree::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  this->Output->ShallowCopy(this->Input);

  int MaximumLevel = inInfo->Get(vtkHyperOctree::LEVELS());
  vtkIdType fact=(1<<(MaximumLevel-1));
  vtkIdType maxNumberOfCells=fact*fact;

  if (this->GeneratedDepths)
  {
    this->GeneratedDepths->Delete();
    this->GeneratedDepths = 0;
  }
  this->GeneratedDepths = vtkIntArray::New();
  this->GeneratedDepths->SetNumberOfComponents(1);

  this->GeneratedDepths->Allocate(maxNumberOfCells);
  this->GeneratedDepths->SetName("Depth");
  this->Output->GetLeafData()->AddArray(this->GeneratedDepths);

  vtkHyperOctreeCursor *cursor=this->Input->NewCellCursor();
  cursor->ToRoot();

  this->NumChildren = cursor->GetNumberOfChildren();
  this->TraverseAndCount(cursor, 0);

  cursor->Delete();

  return 1;
}

//----------------------------------------------------------------------------
void vtkHyperOctreeDepth::TraverseAndCount(vtkHyperOctreeCursor *cursor,
                                           int depth)
{
  if (cursor->CurrentIsLeaf())
  {
    //this node is a leaf, we must stop now
    vtkIdType id=cursor->GetLeafId();
    this->Output->GetLeafData()->GetArray("Depth")->InsertTuple1(id,depth);
  }
  else
  {
    //this node has 'nchildren' children,
    //some of which are internal nodes, so we must continue down
    int i=0;
    while(i<this->NumChildren)
    {
      cursor->ToChild(i);
      this->TraverseAndCount(cursor, depth+1);
      cursor->ToParent();
      ++i;
    }
  }
}

//----------------------------------------------------------------------------
int vtkHyperOctreeDepth::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperOctree");
  return 1;
}

//----------------------------------------------------------------------------
int vtkHyperOctreeDepth::FillOutputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkHyperOctree");
  return 1;
}

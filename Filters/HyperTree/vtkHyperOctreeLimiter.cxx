/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperOctreeLimiter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHyperOctreeLimiter.h"

#include "vtkObjectFactory.h"
#include "vtkHyperOctree.h"
#include "vtkHyperOctreeCursor.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"

#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkCellArray.h"
#include "vtkDataArray.h"

#include "vtkPoints.h"
#include "vtkCellType.h"
#include "vtkDoubleArray.h"

vtkStandardNewMacro(vtkHyperOctreeLimiter);

//----------------------------------------------------------------------------
vtkHyperOctreeLimiter::vtkHyperOctreeLimiter()
{
  this->MaximumLevel = 5;
  this->AccumScratch =  new double[1024];
}

//----------------------------------------------------------------------------
vtkHyperOctreeLimiter::~vtkHyperOctreeLimiter()
{
  delete[] this->AccumScratch;
}

//----------------------------------------------------------------------------
int vtkHyperOctreeLimiter::RequestData(vtkInformation *vtkNotUsed(request),
                                      vtkInformationVector **inputVector,
                                      vtkInformationVector *outputVector)
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  this->Input = vtkHyperOctree::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  this->Output=vtkHyperOctree::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  outInfo->Set(vtkHyperOctree::LEVELS(), this->GetMaximumLevel());

  double Size[3];
  this->Input->GetSize(Size);
  this->Output->SetSize(Size);

  this->TopSize = 1.0;
  if (Size[0] != 0.0) TopSize = TopSize * Size[0];
  if (Size[1] != 0.0) TopSize = TopSize * Size[1];
  if (Size[2] != 0.0) TopSize = TopSize * Size[2];

  double Origin[3];
  this->Input->GetOrigin(Origin);
  this->Output->SetOrigin(Origin);

  this->Dimension = this->Input->GetDimension();
  this->Output->SetDimension(Dimension);

  //TODO: this is incorrect, so I use Insert and Squeeze instead of Set.
  int aMaximumLevel = inInfo->Get(vtkHyperOctree::LEVELS());
  vtkIdType fact=(1<<(aMaximumLevel-1));
  vtkIdType maxNumberOfCells=fact*fact;

  //give the output the same number and type of attribute data arrays
  int pos = 0;
  int a;
  vtkPointData *ipd = this->Input->GetPointData();
  vtkPointData *opd = this->Output->GetPointData();
  int nparrays = ipd->GetNumberOfArrays();
  for (a = 0; a < nparrays; a++)
  {
    vtkDataArray *ida = ipd->GetArray(a);
    vtkDataArray *oda = opd->GetArray(ida->GetName());
    if (oda == NULL)
    {
      oda = ida->NewInstance();
      oda->SetName(ida->GetName());
      oda->SetNumberOfTuples(maxNumberOfCells);
      opd->AddArray(oda);

      int ncomp = oda->GetNumberOfComponents();
      pos += ncomp;

      oda->Delete();
    }
    else
    {
      oda->Reset();
      oda->SetNumberOfTuples(maxNumberOfCells);

      int ncomp = oda->GetNumberOfComponents();
      pos += ncomp;
    }
  }

  vtkCellData *icd = this->Input->GetCellData();
  vtkCellData *ocd = this->Output->GetCellData();
  int ncarrays = icd->GetNumberOfArrays();
  for (a = 0; a < ncarrays; a++)
  {
    vtkDataArray *ida = icd->GetArray(a);
    vtkDataArray *oda = ocd->GetArray(ida->GetName());
    if (oda == NULL)
    {
      oda = ida->NewInstance();
      oda->SetName(ida->GetName());
      oda->SetNumberOfTuples(maxNumberOfCells);
      ocd->AddArray(oda);

      int ncomp = oda->GetNumberOfComponents();
      pos += ncomp;

      oda->Delete();
    }
    else
    {
      oda->Reset();
      oda->SetNumberOfTuples(maxNumberOfCells);

      int ncomp = oda->GetNumberOfComponents();
      pos += ncomp;

    }
  }

  this->AccumSize = pos;

  //build the output tree, copying over attribute data as we go
  vtkHyperOctreeCursor *incursor=this->Input->NewCellCursor();
  incursor->ToRoot();

  vtkHyperOctreeCursor *outcursor=this->Output->NewCellCursor();
  outcursor->ToRoot();

  this->NumChildren = incursor->GetNumberOfChildren();
  this->BuildNextCell(incursor, outcursor, 0);

  for (a = 0; a < nparrays; a++)
  {
    vtkDataArray *oda = opd->GetArray(a);
    oda->Squeeze();
  }
  for (a = 0; a < ncarrays; a++)
  {
    vtkDataArray *oda = ocd->GetArray(a);
    oda->Squeeze();
  }

  incursor->Delete();
  outcursor->Delete();

  return 1;
}

/*
//----------------------------------------------------------------------------
void vtkHyperOctreeLimiter::CollapseNode(vtkHyperOctreeCursor *tocollapse)
{
//cerr << "COLLAPSING" << endl;
  if (!tocollapse->CurrentIsTerminalNode())
    {
    //cerr << "collapse nonterminal" << endl;
    //keep searching until we find leaves or reach the specified depth
    int i=0;
    while(i<this->NumChildren)
      {
      tocollapse->ToChild(i);

      this->CollapseNode(tocollapse);

      tocollapse->ToParent();
      ++i;
      }
    }
  else
    {
    //cerr << "collapse terminal" << endl;
    this->Output->CollapseTerminalNode(tocollapse);
    }
}
*/

//----------------------------------------------------------------------------
void vtkHyperOctreeLimiter::BuildNextCell(vtkHyperOctreeCursor *incursor,
                                          vtkHyperOctreeCursor *outcursor,
                                          int depth)
{
  if (incursor->CurrentIsLeaf() || depth == this->GetMaximumLevel())
  {
    //make sure the output cell is a leaf
    if (!outcursor->CurrentIsLeaf())
    {
      //this is never called anyway, topology is created afresh each time?
      //this->CollapseNode(outcursor);
    }

    //if node is a leaf in the input. copy attribute data over
    if (incursor->CurrentIsLeaf())
    {
      //cerr << "copying at " << depth << endl;
      vtkIdType iid=incursor->GetLeafId();
      vtkIdType oid=outcursor->GetLeafId();

      vtkPointData *ipd = this->Input->GetPointData();
      vtkPointData *opd = this->Output->GetPointData();
      int nparrays = ipd->GetNumberOfArrays();
      int a;
      for (a = 0; a < nparrays; a++)
      {
        vtkDataArray *ida = ipd->GetArray(a);
        vtkDataArray *oda = opd->GetArray(a);
        oda->InsertTuple(oid, ida->GetTuple(iid));
      }

      vtkCellData *icd = this->Input->GetCellData();
      vtkCellData *ocd = this->Output->GetCellData();
      int ncarrays = icd->GetNumberOfArrays();
      for (a = 0; a < ncarrays; a++)
      {
        vtkDataArray *ida = icd->GetArray(a);
        vtkDataArray *oda = ocd->GetArray(a);
        oda->InsertTuple(oid, ida->GetTuple(iid));
      }
    }
    else
    {
      //we have reached the requested depth limit.
      //recursively add each contained leaf's data to this node, weighting
      //by length/area/volume fraction.
      //cerr << "pruning at " << depth << endl;

      //remember where to add and how big
      vtkIdType oid=outcursor->GetLeafId();

      this->SizeAtPrunePoint = 1.0/this->MeasureCell(depth);

      //start off with nothing before accumulating

      for (int s = 0; s < this->AccumSize; s++)
      {
        this->AccumScratch[s] = 0.0;
      }

      //recursively accumulate the length/area/volume weighted attribute data
      //contribution from all leaves interior
      int i=0;
      while(i<this->NumChildren)
      {
        incursor->ToChild(i);

        this->AddInteriorAttributes(incursor, depth+1);

        incursor->ToParent();
        ++i;
      }

      //put the accumulated results back into this new leaf
      int pos = 0;
      int a;
      vtkPointData *opd = this->Output->GetPointData();
      int nparrays = opd->GetNumberOfArrays();
      for (a = 0; a < nparrays; a++)
      {
        vtkDataArray *oda = opd->GetArray(a);
        int ncomp = oda->GetNumberOfComponents();
        for (int j = 0; j < ncomp; j++)
        {
          //cerr << "total contr=" << this->AccumScratch[pos] << endl;
          oda->InsertComponent(oid, j, this->AccumScratch[pos++]);
        }
      }
      vtkCellData *ocd = this->Output->GetCellData();
      int ncarrays = ocd->GetNumberOfArrays();
      for (a = 0; a < ncarrays; a++)
      {
        vtkDataArray *oda = ocd->GetArray(a);
        int ncomp = oda->GetNumberOfComponents();
        for (int j = 0; j < ncomp; j++)
        {
          //cerr << "total contr=" << this->AccumScratch[pos] << endl;
          oda->InsertComponent(oid, j, this->AccumScratch[pos++]);
        }
      }
    }
  }
  else
  {
    if (outcursor->CurrentIsLeaf())
    {
      //create the new cell in the output tree
      //cerr << "creating " << depth << endl;
      this->Output->SubdivideLeaf(outcursor);
    }

    //keep searching until we find leaves or reach the specified depth
    int i=0;
    while(i<this->NumChildren)
    {
      incursor->ToChild(i);
      outcursor->ToChild(i);

      //cerr << "going to " << depth+1 << endl;
      this->BuildNextCell(incursor, outcursor, depth+1);

      incursor->ToParent();
      outcursor->ToParent();
      ++i;
    }
  }
}

//----------------------------------------------------------------------------
double vtkHyperOctreeLimiter::MeasureCell(int depth)
{
  double ret;
  double inh = 0.25;
  switch (this->Dimension) {
  case 1:
    inh = 0.5;
    break;
  case 2:
    inh = 0.25;
    break;
  case 3:
    inh = 0.125;
    break;
  }
  ret = this->TopSize * pow(inh,depth);
  return ret;
}

//----------------------------------------------------------------------------
void vtkHyperOctreeLimiter::AddInteriorAttributes(vtkHyperOctreeCursor *incursor,
                                                  int depth)
{
  if (incursor->CurrentIsLeaf())
  {
    double size = this->MeasureCell(depth);
    double weight = size * this->SizeAtPrunePoint;
    //cerr << "contributing at " << depth << " w=" << weight << endl;

    //add this leaf's contribution to its pruned ancestor
    vtkIdType iid=incursor->GetLeafId();

    int pos = 0;
    int a;
    vtkPointData *ipd = this->Input->GetPointData();
    vtkPointData *opd = this->Output->GetPointData();
    int nparrays = opd->GetNumberOfArrays();
    for (a = 0; a < nparrays; a++)
    {
      vtkDataArray *ida = ipd->GetArray(a);
      vtkDataArray *oda = opd->GetArray(a);
      int ncomp = oda->GetNumberOfComponents();
      for (int j = 0; j < ncomp; j++)
      {
        double now = this->AccumScratch[pos];
        //cerr << "w " << now << endl;
        double contr = static_cast<double>(ida->GetComponent(iid, j)) * weight;
        //cerr << "c " << contr << endl;
        this->AccumScratch[pos++] = now + contr;
      }
    }

    vtkCellData *icd = this->Input->GetCellData();
    vtkCellData *ocd = this->Output->GetCellData();
    int ncarrays = ocd->GetNumberOfArrays();
    for (a = 0; a < ncarrays; a++)
    {
      vtkDataArray *ida = icd->GetArray(a);
      vtkDataArray *oda = ocd->GetArray(a);
      int ncomp = oda->GetNumberOfComponents();
      for (int j = 0; j < ncomp; j++)
      {
        double now = this->AccumScratch[pos];
        double contr = static_cast<double>(ida->GetComponent(iid, j)) * weight;
        this->AccumScratch[pos++] = now+contr;
      }
    }
  }
  else
  {
    //keep going down
    int i=0;
    while(i<this->NumChildren)
    {
      incursor->ToChild(i);

      //cerr << "seeking into " << depth+1 << endl;
      this->AddInteriorAttributes(incursor, depth+1);

      incursor->ToParent();
      ++i;
    }
  }
}

//----------------------------------------------------------------------------
int vtkHyperOctreeLimiter::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperOctree");
  return 1;
}

//----------------------------------------------------------------------------
int vtkHyperOctreeLimiter::FillOutputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkHyperOctree");
  return 1;
}

//----------------------------------------------------------------------------
int vtkHyperOctreeLimiter::GetMaximumLevel()
{
  return this->MaximumLevel;
}

//----------------------------------------------------------------------------
void vtkHyperOctreeLimiter::SetMaximumLevel(int levels)
{
  if (levels < 1)
  {
    levels = 1;
  }

  this->Modified();
  this->MaximumLevel=levels;
}

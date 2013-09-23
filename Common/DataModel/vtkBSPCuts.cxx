/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBSPCuts.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkBSPCuts.h"
#include "vtkKdNode.h"
#include "vtkKdTree.h"
#include "vtkObjectFactory.h"

#ifdef _MSC_VER
#pragma warning ( disable : 4100 )
#endif

vtkStandardNewMacro(vtkBSPCuts);

//----------------------------------------------------------------------------
vtkBSPCuts::vtkBSPCuts()
{
  this->Top = NULL;
  this->NumberOfCuts = 0;
  this->Dim = NULL;
  this->Coord = NULL;
  this->Lower = NULL;
  this->Upper = NULL;
  this->LowerDataCoord = NULL;
  this->UpperDataCoord = NULL;
  this->Npoints = NULL;
}

//----------------------------------------------------------------------------
vtkBSPCuts::~vtkBSPCuts()
{
  if (this->Top)
    {
    vtkBSPCuts::DeleteAllDescendants(this->Top);
    this->Top->Delete();
    }

  this->ResetArrays();
}

//----------------------------------------------------------------------------
void vtkBSPCuts::Initialize()
{
  if (this->Top)
    {
    vtkBSPCuts::DeleteAllDescendants(this->Top);
    this->Top->Delete();
    this->Top = NULL;
    }
  this->ResetArrays();
  this->Superclass::Initialize();
}

//----------------------------------------------------------------------------
void vtkBSPCuts::ResetArrays()
{
  delete [] this->Dim;
  this->Dim = NULL;

  delete [] this->Coord;
  this->Coord = NULL;

  delete [] this->Lower;
  this->Lower = NULL;

  delete [] this->Upper;
  this->Upper = NULL;

  delete [] this->LowerDataCoord;
  this->LowerDataCoord = NULL;

  delete [] this->UpperDataCoord;
  this->UpperDataCoord = NULL;

  delete [] this->Npoints;
  this->Npoints = NULL;

  this->NumberOfCuts = 0;
}
//----------------------------------------------------------------------------
void vtkBSPCuts::AllocateArrays(int nNodes)
{
  this->Dim = new int [nNodes];
  this->Coord = new double [nNodes];
  this->Lower = new int [nNodes];
  this->Upper = new int [nNodes];
  this->LowerDataCoord = new double [nNodes];
  this->UpperDataCoord = new double [nNodes];
  this->Npoints = new int [nNodes];
}
//----------------------------------------------------------------------------
void vtkBSPCuts::DeleteAllDescendants(vtkKdNode *nd)
{
  vtkKdNode *left = nd->GetLeft();
  vtkKdNode *right = nd->GetRight();

  if (left && left->GetLeft())
    {
    vtkBSPCuts::DeleteAllDescendants(left);
    }

  if (right && right->GetLeft())
    {
    vtkBSPCuts::DeleteAllDescendants(right);
    }

  if (left && right)
    {
    nd->DeleteChildNodes();   // undo AddChildNodes
    left->Delete();           // undo vtkKdNode::New()
    right->Delete();
    }
}


//----------------------------------------------------------------------------
void vtkBSPCuts::ShallowCopy(vtkDataObject* src)
{
  this->Superclass::ShallowCopy(src);
  vtkBSPCuts* srcCuts = vtkBSPCuts::SafeDownCast(src);

  this->ResetArrays();
  if (this->Top)
    {
    vtkBSPCuts::DeleteAllDescendants(this->Top);
    this->Top->Delete();
    this->Top = NULL;
    }

  if (srcCuts)
    {
    if (srcCuts->Top)
      {
      this->CreateCuts(srcCuts->Top);
      }
    }
}

//----------------------------------------------------------------------------
void vtkBSPCuts::DeepCopy(vtkDataObject* src)
{
  this->Superclass::DeepCopy(src);
  this->ResetArrays();
  if (this->Top)
    {
    vtkBSPCuts::DeleteAllDescendants(this->Top);
    this->Top->Delete();
    this->Top = NULL;
    }

  vtkBSPCuts* srcCuts = vtkBSPCuts::SafeDownCast(src);
  if (srcCuts)
    {
    if (srcCuts->Top)
      {
      this->CreateCuts(srcCuts->Top);
      }
    }
}

//----------------------------------------------------------------------------
void vtkBSPCuts::CreateCuts(vtkKdNode *kd)
{
  // Given a tree of vtkKdNodes, create the arrays that describe this
  // spatial partitioning.

  kd->GetBounds(this->Bounds);

  // preallocate vectors

  this->ResetArrays();

  int nNodes = vtkBSPCuts::CountNodes(kd);

  this->AllocateArrays(nNodes);

  // convert kd node tree to vector form

  this->NumberOfCuts = this->WriteArray(kd, 0);

  // keep a copy of vtkKdNode tree

  if (this->Top)
    {
    vtkBSPCuts::DeleteAllDescendants(this->Top);
    this->Top->Delete();
    this->Top = NULL;
    }

  this->Top = vtkKdTree::CopyTree(kd);
}
//----------------------------------------------------------------------------
int vtkBSPCuts::CountNodes(vtkKdNode *kd)
{
  int leftCount=0;
  int rightCount=0;

  if (kd->GetLeft())
    {
    leftCount = vtkBSPCuts::CountNodes(kd->GetLeft());
    rightCount = vtkBSPCuts::CountNodes(kd->GetRight());
    }

  return leftCount + rightCount + 1;
}
//----------------------------------------------------------------------------
int vtkBSPCuts::WriteArray(vtkKdNode *kd, int loc)
{
  int nextloc = loc + 1;

  int dim = kd->GetDim();   // 0 (X), 1 (Y), 2 (Z)

  this->Npoints[loc] = kd->GetNumberOfPoints();

  if (kd->GetLeft())
    {
    this->Dim[loc]     = dim;

    vtkKdNode *left = kd->GetLeft();
    vtkKdNode *right = kd->GetRight();

    this->Coord[loc] = left->GetMaxBounds()[dim];
    this->LowerDataCoord[loc] = left->GetMaxDataBounds()[dim];
    this->UpperDataCoord[loc] = right->GetMinDataBounds()[dim];

    int locleft = loc + 1;

    int locright = this->WriteArray(left, locleft);

    nextloc = this->WriteArray(right, locright);

    this->Lower[loc] = locleft;
    this->Upper[loc] = locright;
    }
  else
    {
    this->Dim[loc] = -1;
    this->Coord[loc] = 0.0;
    this->LowerDataCoord[loc] = 0.0;
    this->UpperDataCoord[loc] = 0.0;
    this->Lower[loc] = kd->GetID() * -1;  // partition ID
    this->Upper[loc] = kd->GetID() * -1;
    }

  return nextloc;   // next available array location
}
//----------------------------------------------------------------------------
void vtkBSPCuts::CreateCuts(double *bnds, int ncuts, int *dim, double *coord,
                  int *lower, int *upper,
                  double *lowerDataCoord, double *upperDataCoord,
                  int *npoints)
{
  // Keep a copy of these arrays

  vtkBSPCuts::ResetArrays();
  vtkBSPCuts::AllocateArrays(ncuts);

  for (int i=0; i<6; i++)
    {
    this->Bounds[i] = bnds[i];
    }

  this->NumberOfCuts = ncuts;

  memcpy(this->Dim, dim, sizeof(int) * ncuts);
  memcpy(this->Coord, coord, sizeof(double) * ncuts);
  memcpy(this->Lower, lower, sizeof(int) * ncuts);
  memcpy(this->Upper, upper, sizeof(int) * ncuts);

  if (lowerDataCoord)
    {
    memcpy(this->LowerDataCoord, lowerDataCoord, sizeof(double) * ncuts);
    }
  else
    {
    delete [] this->LowerDataCoord;
    this->LowerDataCoord = NULL;
    }

  if (upperDataCoord)
    {
    memcpy(this->UpperDataCoord, upperDataCoord, sizeof(double) * ncuts);
    }
  else
    {
    delete [] this->UpperDataCoord;
    this->UpperDataCoord = NULL;
    }

  if (npoints)
    {
    memcpy(this->Npoints, npoints, sizeof(int) * ncuts);
    }
  else
    {
    delete [] this->Npoints;
    this->Npoints = NULL;
    }

  // Now build tree from arrays

  if (this->Top)
    {
    vtkBSPCuts::DeleteAllDescendants(this->Top);
    this->Top->Delete();
    this->Top = NULL;
    }

  this->Top = vtkKdNode::New();
  this->Top->SetBounds(bnds[0], bnds[1],bnds[2],bnds[3],bnds[4],bnds[5]);
  this->Top->SetDataBounds(bnds[0], bnds[1],bnds[2],bnds[3],bnds[4],bnds[5]);

  this->BuildTree(this->Top, 0);

  vtkBSPCuts::SetMinMaxId(this->Top);
}
//----------------------------------------------------------------------------
void vtkBSPCuts::BuildTree(vtkKdNode *kd, int idx)
{
  int dim = this->Dim[idx];

  if (this->Npoints)
    {
    kd->SetNumberOfPoints(this->Npoints[idx]);
    }

  if (this->Lower[idx] > 0)
    {
    vtkKdNode *left = vtkKdNode::New();
    vtkKdNode *right = vtkKdNode::New();

    kd->SetDim(dim);

    double b2[6];
    double db2[6];

    kd->GetBounds(b2);
    kd->GetDataBounds(db2);

    b2[dim*2 + 1] = this->Coord[idx];  // new upper bound for lower half

    if (this->LowerDataCoord)
      {
      db2[dim*2 + 1] = this->LowerDataCoord[idx];
      }
    else
      {
      db2[dim*2 + 1] = this->Coord[idx];
      }

    left->SetBounds(b2[0],b2[1],b2[2],b2[3],b2[4],b2[5]);
    left->SetDataBounds(db2[0],db2[1],db2[2],db2[3],db2[4],db2[5]);

    kd->GetBounds(b2);
    kd->GetDataBounds(db2);

    b2[dim*2] = this->Coord[idx];  // new lower bound for upper half

    if (this->UpperDataCoord)
      {
      db2[dim*2] = this->UpperDataCoord[idx];
      }
    else
      {
      db2[dim*2] = this->Coord[idx];
      }

    right->SetBounds(b2[0],b2[1],b2[2],b2[3],b2[4],b2[5]);
    right->SetDataBounds(db2[0],db2[1],db2[2],db2[3],db2[4],db2[5]);

    kd->AddChildNodes(left, right);

    this->BuildTree(left, this->Lower[idx]);
    this->BuildTree(right, this->Upper[idx]);
    }
  else
    {
    kd->SetID(this->Lower[idx] * -1);  // partition ID of leaf node
    }
}
//----------------------------------------------------------------------------
void vtkBSPCuts::SetMinMaxId(vtkKdNode *kd)
{
  if (kd->GetLeft())
    {
    vtkBSPCuts::SetMinMaxId(kd->GetLeft());
    vtkBSPCuts::SetMinMaxId(kd->GetRight());
    }
  else
    {
    kd->SetMinID(kd->GetID());
    kd->SetMaxID(kd->GetID());
    return;
    }

  int min1 = kd->GetLeft()->GetMinID();
  int max1 = kd->GetLeft()->GetMaxID();
  int min2 = kd->GetRight()->GetMinID();
  int max2 = kd->GetRight()->GetMaxID();

  kd->SetMinID( (min1 < min2) ? min1 : min2);
  kd->SetMaxID( (max1 > max2) ? max1 : max2);
}

//----------------------------------------------------------------------------
int vtkBSPCuts::GetArrays(int len,
                int *dim, double *coord, int *lower, int *upper,
                double *lowerDataCoord, double *upperDataCoord, int *npoints)
{
  int l = (len < this->NumberOfCuts) ? len : this->NumberOfCuts;

  if (l < 1) return 1;

  if (dim)
    {
    memcpy(dim, this->Dim, l * sizeof(int));
    }
  if (coord)
    {
    memcpy(coord, this->Coord, l * sizeof(double));
    }
  if (lower)
    {
    memcpy(lower, this->Lower, l * sizeof(int));
    }
  if (upper)
    {
    memcpy(upper, this->Upper, l * sizeof(int));
    }

  if (lowerDataCoord && this->LowerDataCoord)
    {
    memcpy(lowerDataCoord, this->LowerDataCoord, l * sizeof(double));
    }
  if (upperDataCoord && this->UpperDataCoord)
    {
    memcpy(upperDataCoord, this->UpperDataCoord, l * sizeof(double));
    }
  if (npoints && this->Npoints)
    {
    memcpy(npoints, this->Npoints, l * sizeof(int));
    }

  return 0;
}

//-----------------------------------------------------------------------------
int vtkBSPCuts::Equals(vtkBSPCuts *other, double tolerance)
{
#define EQ(x, y)        (((x)-(y) <= tolerance) && (y)-(x) <= tolerance)
  if (!other)
    {
    return 0;
    }

  if (this->NumberOfCuts != other->NumberOfCuts)
    {
    return 0;
    }

  for (int i = 0; i < this->NumberOfCuts; i++)
    {
    if (this->Dim[i] != other->Dim[i])
      {
      return 0;
      }

    if (this->Dim[i] < 0)
      {
      // Leaf nodes have no real data.
      continue;
      }

    if (!EQ(this->Coord[i], other->Coord[i]))
      {
      return 0;
      }
    if (this->Lower[i] != other->Lower[i])
      {
      return 0;
      }
    if (this->Upper[i] != other->Upper[i])
      {
      return 0;
      }
    if (!EQ(this->LowerDataCoord[i], other->LowerDataCoord[i]))
      {
      return 0;
      }
    if (!EQ(this->UpperDataCoord[i], other->UpperDataCoord[i]))
      {
      return 0;
      }
    if (this->Npoints[i] != other->Npoints[i])
      {
      return 0;
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkBSPCuts::PrintArrays()
{
  int i;
  if (this->NumberOfCuts == 0)
    {
    return;
    }

  cout << "xmin: " << this->Bounds[0] << " xmax: " << this->Bounds[1] << endl ;
  cout << "ymin: " << this->Bounds[2] << " ymax: " << this->Bounds[3] << endl;
  cout << "zmin: " << this->Bounds[4] << " zmax: " << this->Bounds[5] << endl;

  cout << "index / dimension / coordinate / lower region / upper region" << endl;

  for (i=0; i<this->NumberOfCuts; i++)
    {
    cout << i << " / " << this->Dim[i] << " / " << this->Coord[i];
    cout << " / " << this->Lower[i] << " / " << this->Upper[i] << endl;
    }

  if (this->LowerDataCoord)
    {
    cout << "index / lower data bdry / upper data bdry / data points" << endl;

    for (i=0; i<this->NumberOfCuts; i++)
      {
      cout << i << " / " << this->LowerDataCoord[i] << " / " << this->UpperDataCoord[i];
      cout << " / " << this->Npoints[i] << endl;
      }
    }
}
//----------------------------------------------------------------------------
void vtkBSPCuts::PrintTree()
{
  if (this->Top == NULL)
    {
    return;
    }

  vtkBSPCuts::_PrintTree(this->Top, 0);
}
//----------------------------------------------------------------------------
void vtkBSPCuts::_PrintTree(vtkKdNode *kd, int depth)
{
  kd->PrintNode(depth);

  if (kd->GetLeft())
    {
    vtkBSPCuts::_PrintTree(kd->GetLeft(),depth+1);
    vtkBSPCuts::_PrintTree(kd->GetRight(),depth+1);
    }
}

//----------------------------------------------------------------------------
vtkBSPCuts* vtkBSPCuts::GetData(vtkInformation* info)
{
  return vtkBSPCuts::SafeDownCast(vtkDataObject::GetData(info));
}
//----------------------------------------------------------------------------
vtkBSPCuts* vtkBSPCuts::GetData(vtkInformationVector* v, int i)
{
  return vtkBSPCuts::SafeDownCast(vtkDataObject::GetData(v, i));
}

//----------------------------------------------------------------------------
void vtkBSPCuts::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Top: " << this->Top << endl;
  os << indent << "NumberOfCuts: " << this->NumberOfCuts << endl;
  os << indent << "Dim: " << this->Dim << endl;
  os << indent << "Coord: " << this->Coord << endl;
  os << indent << "Lower: " << this->Lower << endl;
  os << indent << "Upper: " << this->Upper << endl;
  os << indent << "LowerDataCoord: " << this->LowerDataCoord << endl;
  os << indent << "UpperDataCoord: " << this->UpperDataCoord << endl;
  os << indent << "Npoints: " << this->Npoints << endl;
}

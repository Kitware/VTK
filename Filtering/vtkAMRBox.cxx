/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAMRBox.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAMRBox.h"

#include "vtkUnsignedCharArray.h"
#include "vtkCellData.h"
#include "vtkType.h"

#include <vector>
#include <algorithm>
using std::vector;
using std::copy;


//-----------------------------------------------------------------------------
void vtkAMRBox::Invalidate()
{
  this->LoCorner[0]=this->LoCorner[1]=this->LoCorner[2]=0;
  this->HiCorner[0]=this->HiCorner[1]=this->HiCorner[2]=-1;
}

//-----------------------------------------------------------------------------
vtkAMRBox::vtkAMRBox(int dim)
{
  this->SetDimensionality(dim);
  this->Invalidate();
  this->X0[0]=this->X0[1]=this->X0[2]=0.0;
  this->DX[0]=this->DX[1]=this->DX[2]=1.0;
}

//-----------------------------------------------------------------------------
vtkAMRBox::vtkAMRBox(
    int ilo,int jlo,
    int ihi,int jhi)
{
  this->SetDimensionality(2);
  this->SetDimensions(ilo,jlo,0,ihi,jhi,0);
  this->X0[0]=this->X0[1]=this->X0[2]=0.0;
  this->DX[0]=this->DX[1]=this->DX[2]=1.0;
}

//-----------------------------------------------------------------------------
vtkAMRBox::vtkAMRBox(
    int ilo,int jlo,int klo,
    int ihi,int jhi,int khi)
{
  this->SetDimensionality(3);
  this->SetDimensions(ilo,jlo,klo,ihi,jhi,khi);
  this->X0[0]=this->X0[1]=this->X0[2]=0.0;
  this->DX[0]=this->DX[1]=this->DX[2]=1.0;
}

//-----------------------------------------------------------------------------
vtkAMRBox::vtkAMRBox(const int *lo, const int *hi)
{
  this->SetDimensionality(3);
  this->SetDimensions(lo,hi);
  this->X0[0]=this->X0[1]=this->X0[2]=0.0;
  this->DX[0]=this->DX[1]=this->DX[2]=1.0;
}

//-----------------------------------------------------------------------------
vtkAMRBox::vtkAMRBox(int dim, const int *lo, const int *hi)
{
  this->SetDimensionality(dim);
  this->SetDimensions(lo,hi);
  this->X0[0]=this->X0[1]=this->X0[2]=0.0;
  this->DX[0]=this->DX[1]=this->DX[2]=1.0;
}

//-----------------------------------------------------------------------------
vtkAMRBox::vtkAMRBox(const int *dims)
{
  this->SetDimensionality(3);
  this->SetDimensions(dims);
  this->X0[0]=this->X0[1]=this->X0[2]=0.0;
  this->DX[0]=this->DX[1]=this->DX[2]=1.0;
}

//-----------------------------------------------------------------------------
vtkAMRBox::vtkAMRBox(int dim, const int *dims)
{
  this->SetDimensionality(dim);
  this->SetDimensions(dims);
  this->X0[0]=this->X0[1]=this->X0[2]=0.0;
  this->DX[0]=this->DX[1]=this->DX[2]=1.0;
}

//-----------------------------------------------------------------------------
vtkAMRBox::vtkAMRBox(const vtkAMRBox &other)
{
  if (this==&other) return;
  this->SetDimensionality(other.GetDimensionality());
  int lo[3];
  int hi[3];
  other.GetDimensions(lo,hi);
  this->SetDimensions(lo,hi);
  this->SetGridSpacing(other.GetGridSpacing());
  this->SetDataSetOrigin(other.GetDataSetOrigin());
}

//-----------------------------------------------------------------------------
vtkAMRBox &vtkAMRBox::operator=(const vtkAMRBox &other)
{
  if (this==&other) return *this;
  this->SetDimensionality(other.GetDimensionality());
  int lo[3];
  int hi[3];
  other.GetDimensions(lo,hi);
  this->SetDimensions(lo,hi);
  this->SetGridSpacing(other.GetGridSpacing());
  this->SetDataSetOrigin(other.GetDataSetOrigin());
  return *this;
}

//-----------------------------------------------------------------------------
void vtkAMRBox::SetDimensionality(int dim)
{
  if (dim<1 || dim>3)
    {
    vtkGenericWarningMacro(
      "Invalid spatial dimension, " << dim << ", given.");
    return;
    }
  this->Dimension=dim;
}

//-----------------------------------------------------------------------------
void vtkAMRBox::SetDimensions(
    int ilo,int jlo,int klo,
    int ihi,int jhi,int khi)
{
  if (ilo>ihi || jlo>jhi || klo>khi)
    {
    this->Invalidate();
    }
  else
    {
    this->LoCorner[0]=ilo;
    this->LoCorner[1]=jlo;
    this->LoCorner[2]=klo;
    this->HiCorner[0]=ihi;
    this->HiCorner[1]=jhi;
    this->HiCorner[2]=khi;
    }
}

//-----------------------------------------------------------------------------
void vtkAMRBox::SetDimensions(const int *lo, const int *hi)
{
  switch (this->Dimension)
    {
    case 1:
      this->SetDimensions(lo[0],0,0,hi[0],0,0);
      break;
    case 2:
      this->SetDimensions(lo[0],lo[1],0,hi[0],hi[1],0);
      break;
    case 3:
      this->SetDimensions(lo[0],lo[1],lo[2],hi[0],hi[1],hi[2]);
      break;
    }
}

//-----------------------------------------------------------------------------
void vtkAMRBox::SetDimensions(const int *dims)
{
  switch (this->Dimension)
    {
    case 1:
    this->SetDimensions(dims[0],0,0,dims[1],0,0);
    break;
    case 2:
    this->SetDimensions(dims[0],dims[2],0,dims[1],dims[3],0);
    break;
    case 3:
    this->SetDimensions(dims[0],dims[2],dims[4],dims[1],dims[3],dims[5]);
    break;
    }
}

//-----------------------------------------------------------------------------
void vtkAMRBox::GetDimensions(int *lo, int *hi) const
{
  for (int q=0; q<this->Dimension; ++q)
    {
    lo[q]=this->LoCorner[q];
    hi[q]=this->HiCorner[q];
    }
}

//-----------------------------------------------------------------------------
void vtkAMRBox::GetDimensions(int dims[6]) const
{
  dims[0]=this->LoCorner[0];
  dims[1]=this->HiCorner[0];
  dims[2]=this->LoCorner[1];
  dims[3]=this->HiCorner[1];
  dims[4]=this->LoCorner[2];
  dims[5]=this->HiCorner[2];
}

//-----------------------------------------------------------------------------
void vtkAMRBox::GetLoCorner(int *lo) const
{
  for (int q=0; q<this->Dimension; ++q)
    {
    lo[q]=this->LoCorner[q];
    }
}

//-----------------------------------------------------------------------------
void vtkAMRBox::GetHiCorner(int *hi) const
{
  for (int q=0; q<this->Dimension; ++q)
    {
    hi[q]=this->HiCorner[q];
    }
}

//-----------------------------------------------------------------------------
void vtkAMRBox::GetGridSpacing(double *dX) const
{
  for (int q=0; q<this->Dimension; ++q)
    {
    dX[q]=this->DX[q];
    }
}

//-----------------------------------------------------------------------------
void vtkAMRBox::SetGridSpacing(double dx)
{
  this->SetGridSpacing(dx,dx,dx);
}

//-----------------------------------------------------------------------------
void vtkAMRBox::SetGridSpacing(const double *dX)
{
  switch (this->Dimension)
    {
  case 2:
    this->SetGridSpacing(dX[0],dX[1],0.0);
    break;
  case 3:
    this->SetGridSpacing(dX[0],dX[1],dX[2]);
    break;
    }
}

//-----------------------------------------------------------------------------
void vtkAMRBox::SetGridSpacing(double dx, double dy, double dz)
{
  this->DX[0]=dx;
  this->DX[1]=dy;
  this->DX[2]=dz;
}

//-----------------------------------------------------------------------------
void vtkAMRBox::GetDataSetOrigin(double *x0) const
{
  for (int q=0; q<this->Dimension; ++q)
    {
    x0[q]=this->X0[q];
    }
}

//-----------------------------------------------------------------------------
void vtkAMRBox::SetDataSetOrigin(const double *x0)
{
  switch (this->Dimension)
    {
  case 1:
    this->SetDataSetOrigin(x0[0],0.0,0.0);
    break;
  case 2:
    this->SetDataSetOrigin(x0[0],x0[1],0.0);
    break;
  case 3:
    this->SetDataSetOrigin(x0[0],x0[1],x0[2]);
    break;
    }
}

//-----------------------------------------------------------------------------
void vtkAMRBox::SetDataSetOrigin(double x0, double y0, double z0)
{
  this->X0[0]=x0;
  this->X0[1]=y0;
  this->X0[2]=z0;
}

//-----------------------------------------------------------------------------
void vtkAMRBox::GetBoxOrigin(double *x0) const
{
  x0[0]=this->X0[0]+this->DX[0]*this->LoCorner[0];
  if (this->Dimension>=2)
    {
    x0[1]=this->X0[1]+this->DX[1]*this->LoCorner[1];
    }
  if (this->Dimension==3)
    {
    x0[2]=this->X0[2]+this->DX[2]*this->LoCorner[2];
    }
}

//-----------------------------------------------------------------------------
void vtkAMRBox::GetNumberOfCells(int *ext) const
{
  if (this->Empty())
    {
    ext[0]=ext[1]=0;
    if (this->Dimension>2){ ext[2]=0; }
    return;
    }

  ext[2]=1;
  for (int q=0; q<this->Dimension; ++q)
    {
    ext[q]=this->HiCorner[q]-this->LoCorner[q]+1;
    }
}

//-----------------------------------------------------------------------------
vtkIdType vtkAMRBox::GetNumberOfCells() const
{
  if (this->Empty())
    {
    return 0;
    }

  vtkIdType nCells=1;
  for (int q=0; q<this->Dimension; ++q)
    {
    nCells*=this->HiCorner[q]-this->LoCorner[q]+1;
    }
  return nCells;
}

//-----------------------------------------------------------------------------
void vtkAMRBox::GetNumberOfNodes(int *ext) const
{
  if (this->Empty())
    {
    ext[0]=ext[1]=0;
    if (this->Dimension>2){ ext[2]=0; }
    return;
    }

  ext[2]=1;
  for (int q=0; q<this->Dimension; ++q)
    {
    ext[q]=this->HiCorner[q]-this->LoCorner[q]+2;
    }
}

//-----------------------------------------------------------------------------
vtkIdType vtkAMRBox::GetNumberOfNodes() const
{
  if (this->Empty())
    {
    return 0;
    }

  vtkIdType nPoints=1;
  for (int q=0; q<this->Dimension; ++q)
    {
    nPoints*=this->HiCorner[q]-this->LoCorner[q]+2;
    }
  return nPoints;
}

//-----------------------------------------------------------------------------
void vtkAMRBox::Grow(int byN)
{
  if (this->Empty())
    {
    return;
    }
  int lo[3];
  int hi[3];
  for (int q=0; q<this->Dimension; ++q)
    {
    lo[q]=this->LoCorner[q]-byN;
    hi[q]=this->HiCorner[q]+byN;
    }
  this->SetDimensions(lo,hi);
}

//-----------------------------------------------------------------------------
void vtkAMRBox::Shrink(int byN)
{
  if (this->Empty())
    {
    return;
    }
  int lo[3];
  int hi[3];
  for (int q=0; q<this->Dimension; ++q)
    {
    lo[q]=this->LoCorner[q]+byN;
    hi[q]=this->HiCorner[q]-byN;
    }
  this->SetDimensions(lo,hi);
}

//-----------------------------------------------------------------------------
void vtkAMRBox::Shift(int i, int j)
{
  if (this->Empty())
    {
    return;
    }
  this->SetDimensions(
    this->LoCorner[0]+i, this->LoCorner[1]+j,0,
    this->HiCorner[0]+i, this->HiCorner[1]+j,0);
}

//-----------------------------------------------------------------------------
void vtkAMRBox::Shift(int i, int j, int k)
{
  if (this->Empty())
    {
    return;
    }
  this->SetDimensions(
    this->LoCorner[0]+i, this->LoCorner[1]+j,this->LoCorner[2]+k,
    this->HiCorner[0]+i, this->HiCorner[1]+j,this->HiCorner[2]+k);
}

//-----------------------------------------------------------------------------
void vtkAMRBox::Shift(const int *I)
{
  switch (this->Dimension)
    {
    case 2:
      this->Shift(I[0],I[1]);
      break;
    case 3:
      this->Shift(I[0],I[1],I[2]);
      break;
    }
}

//-----------------------------------------------------------------------------
bool vtkAMRBox::Empty() const
{
  for (int q=0; q<this->Dimension; ++q)
    {
    if (this->HiCorner[q]<this->LoCorner[q])
      {
      return true;
      }
    }
  return false;
}

//-----------------------------------------------------------------------------
bool vtkAMRBox::operator==(const vtkAMRBox &other)
{
  if ( this->Dimension!=other.Dimension)
    {
    return false;
    }

  switch (this->Dimension)
    {
    case 1:
      if ((this->Empty() && other.Empty())
          ||(this->LoCorner[0]==other.LoCorner[0]
             && this->HiCorner[0]==other.HiCorner[0]))
        {
        return true;
        }
      break;
    case 2:
      if ((this->Empty() && other.Empty())
          ||(this->LoCorner[0]==other.LoCorner[0]
             && this->LoCorner[1]==other.LoCorner[1]
             && this->HiCorner[0]==other.HiCorner[0]
             && this->HiCorner[1]==other.HiCorner[1]))
        {
        return true;
        }
      break;
    case 3:
      if ((this->Empty() && other.Empty())
          ||(this->LoCorner[0]==other.LoCorner[0]
             && this->LoCorner[1]==other.LoCorner[1]
             && this->LoCorner[2]==other.LoCorner[2]
             && this->HiCorner[0]==other.HiCorner[0]
             && this->HiCorner[1]==other.HiCorner[1]
             && this->HiCorner[2]==other.HiCorner[2]))
        {
        return true;
        }
      break;
    }
  return false;
}

//-----------------------------------------------------------------------------
void vtkAMRBox::operator&=(const vtkAMRBox &other)
{
  if (this->Dimension!=other.Dimension)
    {
    vtkGenericWarningMacro(
      "Can't operate on a " << this->Dimension
      << "D box with a " << other.Dimension << "D box.");
    return;
    }
  if (this->Empty())
    {
    return;
    }
  if (other.Empty())
    {
    this->Invalidate();
    return;
    }

  int otherLo[3];
  int otherHi[3];
  other.GetDimensions(otherLo,otherHi);
  int lo[3];
  int hi[3];
  for (int q=0; q<this->Dimension; ++q)
    {
    lo[q]=(this->LoCorner[q]>otherLo[q] ? this->LoCorner[q] : otherLo[q]);
    hi[q]=(this->HiCorner[q]<otherHi[q] ? this->HiCorner[q] : otherHi[q]);
    }
  this->SetDimensions(lo,hi);
}

//-----------------------------------------------------------------------------
bool vtkAMRBox::Contains(int i,int j,int k) const
{
  switch (this->Dimension)
    {
    case 1:
    if (!this->Empty()
      && this->LoCorner[0]<=i
      && this->HiCorner[0]>=i)
      {
      return true;
      }
    break;
    case 2:
    if (!this->Empty()
      && this->LoCorner[0]<=i
      && this->HiCorner[0]>=i
      && this->LoCorner[1]<=j
      && this->HiCorner[1]>=j)
      {
      return true;
      }
    break;
    case 3:
    if (!this->Empty()
      && this->LoCorner[0]<=i
      && this->HiCorner[0]>=i
      && this->LoCorner[1]<=j
      && this->HiCorner[1]>=j
      && this->LoCorner[2]<=k
      && this->HiCorner[2]>=k)
      {
      return true;
      }
    break;
    }
  return false;
}

//-----------------------------------------------------------------------------
bool vtkAMRBox::Contains(const int *I) const
{
  switch (this->Dimension)
    {
    case 1:
    return this->Contains(I[0],0,0);
    break;
    case 2:
    return this->Contains(I[0],I[1],0);
    break;
    case 3:
    return this->Contains(I[0],I[1],I[2]);
    break;
    }
  return false;
}

//-----------------------------------------------------------------------------
bool vtkAMRBox::Contains(const vtkAMRBox &other) const
{
  if (this->Dimension!=other.Dimension)
    {
    vtkGenericWarningMacro(
      "Can't operate on a " << this->Dimension
      << "D box with a " << other.Dimension << "D box.");
    return false;
    }
  const int *lo=other.GetLoCorner();
  const int *hi=other.GetHiCorner();

  return this->Contains(lo) && this->Contains(hi);
}

//-----------------------------------------------------------------------------
void vtkAMRBox::Refine(int r)
{
  if (this->Empty())
    {
    return;
    }

  int lo[3];
  int hi[3];
  for (int q=0; q<this->Dimension; ++q)
    {
    lo[q]=this->LoCorner[q]*r;
    hi[q]=(this->HiCorner[q]+1)*r-1;
    }
  this->SetDimensions(lo,hi);

  this->DX[0]/=r;
  this->DX[1]/=r;
  this->DX[2]/=r;
}

//-----------------------------------------------------------------------------
void vtkAMRBox::Coarsen(int r)
{
  if (this->Empty())
    {
    return;
    }

  // sanity check.
  int nCells[3];
  this->GetNumberOfCells(nCells);
  for (int q=0; q<this->Dimension; ++q)
    {
    if (nCells[q]%r)
      {
      vtkGenericWarningMacro("This box cannot be coarsened.");
      return;
      }
    }

  int lo[3];
  int hi[3];
  for (int q=0; q<this->Dimension; ++q)
    {
    lo[q]=(this->LoCorner[q]<0 ? -abs(this->LoCorner[q]+1)/r-1 : this->LoCorner[q]/r);
    hi[q]=(this->HiCorner[q]<0 ? -abs(this->HiCorner[q]+1)/r-1 : this->HiCorner[q]/r);
    }
  this->SetDimensions(lo,hi);

  this->DX[0]*=r;
  this->DX[1]*=r;
  this->DX[2]*=r;
}

//-----------------------------------------------------------------------------
ostream &vtkAMRBox::Print(ostream &os) const
{
  os << "("  << this->LoCorner[0]
     << ","  << this->LoCorner[1]
     << ","  << this->LoCorner[2]
     << ")(" << this->HiCorner[0]
     << ","  << this->HiCorner[1]
     << ","  << this->HiCorner[2]
     << ")(" << this->X0[0]
     << ","  << this->X0[1]
     << ","  << this->X0[2]
     << ")(" << this->DX[0]
     << ","  << this->DX[1]
     << ","  << this->DX[2]
     << ")";
  return os;
}





















//*****************************************************************************
void Split(
      const int N[3],
      const int minSide[3],
      std::vector<vtkAMRBox> &decomp)
{
  vector<vtkAMRBox> tDecomp; // Working array for resulting splits
  vector<vtkAMRBox> aDecomp; // and for atomic boxes.

  // For each coordinate direction attempt N splits.
  for (int cdir=0; cdir<3; ++cdir)
    {
    int n=N[cdir];
    while(n>0 && decomp.size())
      {
      size_t nBoxes=decomp.size();
      for (size_t bid=0; bid<nBoxes; ++bid)
        {
        vtkAMRBox original=decomp[bid];
        if (original.Empty())
          {
          // skip empty boxes.
          continue;
          }
        int lo[3];
        int hi[3];
        original.GetDimensions(lo,hi);
        // Don't split atomic boxes,do pass through.
        if (lo[cdir]==hi[cdir]
          || hi[cdir]-lo[cdir]<minSide[cdir] )
          {
          aDecomp.push_back(original);
          continue;
          }
        // Split evenly in the given direction.
        int mid=(lo[cdir]+hi[cdir])/2;
        //
        int sHi[3]={hi[0],hi[1],hi[2]};
        sHi[cdir]=mid;
        vtkAMRBox first(lo,sHi);
        tDecomp.push_back(first);
        //
        int sLo[3]={lo[0],lo[1],lo[2]};
        sLo[cdir]=mid+1;
        vtkAMRBox second(sLo,hi);
        tDecomp.push_back(second);
        }
      // Update the list we operate on, so the these splits
      // are subsequently split in the next pass.
      decomp.clear();
      decomp=tDecomp;
      tDecomp.clear();
      --n;
      }
    // Merge the atomic boxes back into the list.
    size_t nRemain=decomp.size();
    size_t nAtomic=aDecomp.size();
    decomp.resize(nRemain+nAtomic);
    copy(aDecomp.begin(),aDecomp.end(),decomp.begin()+nRemain);
    aDecomp.clear();
    }
}

//*****************************************************************************
void Split(
      const int minSide[3],
      std::vector<vtkAMRBox> &decomp)
{
  vector<vtkAMRBox> tDecomp; // Working array for resulting splits
  vector<vtkAMRBox> aDecomp; // and for atomic boxes.
  for (int cdir=0; cdir<3; ++cdir)
    {
    while(decomp.size())
      {
      size_t nBoxes=decomp.size();
      for (size_t bid=0; bid<nBoxes; ++bid)
        {
        vtkAMRBox original=decomp[bid];
        if (original.Empty())
          {
          // skip empty boxes.
          continue;
          }
        int lo[3];
        int hi[3];
        original.GetDimensions(lo,hi);
        // Don't split atomic boxes,do pass through.
        if (lo[cdir]==hi[cdir]
          || hi[cdir]-lo[cdir]<minSide[cdir] )
          {
          aDecomp.push_back(original);
          continue;
          }
        // Split evenly in the given direction.
        int mid=(lo[cdir]+hi[cdir])/2;
        //
        int sHi[3]={hi[0],hi[1],hi[2]};
        sHi[cdir]=mid;
        vtkAMRBox first(lo,sHi);
        tDecomp.push_back(first);
        //
        int sLo[3]={lo[0],lo[1],lo[2]};
        sLo[cdir]=mid+1;
        vtkAMRBox second(sLo,hi);
        tDecomp.push_back(second);
        }
      // Update the list we operate on, so the these splits
      // are subsequently split in the next pass.
      decomp.clear();
      decomp=tDecomp;
      tDecomp.clear();
      }
    // Merge the atomic boxes back into the list.
    size_t nRemain=decomp.size();
    size_t nAtomic=aDecomp.size();
    decomp.resize(nRemain+nAtomic);
    copy(aDecomp.begin(),aDecomp.end(),decomp.begin()+nRemain);
    aDecomp.clear();
    }
}















// TODO delete these
// These are legacy methods going away, do not use!
#ifndef VTK_LEGACY_REMOVE
int vtkAMRBox::DoesContainCell(int i, int j, int k)
{
  VTK_LEGACY_REPLACED_BODY(vtkAMRBox::DoesContainCell, "VTK 5.4",
                           vtkAMRBox::Contains);
  return this->Contains(i,j,k);
}

int vtkAMRBox::DoesContainBox(vtkAMRBox const & box) const
{
  VTK_LEGACY_REPLACED_BODY(vtkAMRBox::DoesContainBox, "VTK 5.4",
                           vtkAMRBox::Contains);
  return this->Contains(box);
}
#endif // #ifndef VTK_LEGACY_REMOVE

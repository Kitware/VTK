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
#include "vtkStructuredData.h"

#include <vector>
#include <algorithm>
#include <cstring>
#include <cassert>
#include <sstream>
#include <fstream>



//-----------------------------------------------------------------------------
vtkAMRBox::vtkAMRBox(int dim)
{
  this->Initialize();
  this->SetDimensionality(dim);
}

//-----------------------------------------------------------------------------
vtkAMRBox::vtkAMRBox(
    int ilo,int jlo,
    int ihi,int jhi)
{
  this->BuildAMRBox( ilo,jlo,0,ihi,jhi,0);
//  assert( "post: Dimension expected to <= 2" &&
//    ( (this->GetDimensionality()==2) || (this->GetDimensionality()==1) ) );
}

//-----------------------------------------------------------------------------
vtkAMRBox::vtkAMRBox(
    int ilo,int jlo,int klo,
    int ihi,int jhi,int khi)
{
  this->BuildAMRBox( ilo, jlo, klo, ihi,jhi,khi);
}

//-----------------------------------------------------------------------------
vtkAMRBox::vtkAMRBox(const int *lo, const int *hi)
{
  this->BuildAMRBox( lo[0],lo[1],lo[2],hi[0],hi[1],hi[2] );
}

//-----------------------------------------------------------------------------
vtkAMRBox::vtkAMRBox(int vtkNotUsed(dim), const int *lo, const int *hi)
{

  assert( "pre: lo corner buffer is NULL" && (lo != NULL) );
  assert( "pre: hi corner buffer is NULL" && (hi != NULL) );

  this->BuildAMRBox( lo[0], lo[1], lo[2], hi[0], hi[1], hi[2] );
  assert( "post: Dimension expected to be 2 or 3" &&
      ( (this->GetDimensionality()==2) || (this->GetDimensionality()==3) ) );
}

//-----------------------------------------------------------------------------
vtkAMRBox::vtkAMRBox(const int *dims)
{
  this->BuildAMRBox(dims[0],dims[2],dims[4],dims[1],dims[3],dims[5] );
  assert( "post: Dimension expected to be 2 or 3" &&
      ( (this->GetDimensionality()==2) || (this->GetDimensionality()==3) ) );

}

//-----------------------------------------------------------------------------
vtkAMRBox::vtkAMRBox(int vtkNotUsed(dim), const int *dims)
{
  this->BuildAMRBox( dims[0], dims[2],dims[4], dims[1],dims[3],dims[5] );
  assert( "post: Dimension expected to be 2 or 3" &&
      ( (this->GetDimensionality()==2) || (this->GetDimensionality()==3) ) );
}


//-----------------------------------------------------------------------------
void vtkAMRBox::BuildAMRBox(
    const int ilo, const int jlo, const int klo,
    const int ihi, const int jhi, const int khi  )
{
  this->Initialize();

// Dimension must be explicitely defined!
//  this->SetDimensionality(
//   this->DetectDimension(ilo, jlo, klo,ihi, jhi, khi ) );

  this->SetDimensions(ilo,jlo,klo,ihi,jhi,khi);
  this->X0[0]=this->X0[1]=this->X0[2]=0.0;
  this->DX[0]=this->DX[1]=this->DX[2]=1.0;
  this->RealExtent[0] = ilo;
  this->RealExtent[1] = ihi;
  this->RealExtent[2] = jlo;
  this->RealExtent[3] = jhi;
  this->RealExtent[4] = klo;
  this->RealExtent[5] = khi;
}

//-----------------------------------------------------------------------------
int vtkAMRBox::GetCellLinearIndex( const int i, const int j, const int k )
{
  // Sanity Check!
  assert( "pre: i-index out-of-bounds!" &&
   ( (i >= 0) && (i <= this->HiCorner[0]) ) );
  assert( "pre: j-index out-of-bounds!" &&
   ( (j >= 0) && (j <= this->HiCorner[1] ) ) );
  assert( "pre: k-index out-of-bounds!" &&
   ( (k >= 0) && (k <= this->HiCorner[2]) ) );

  // Get Cell dimensions.
  int ndim[3];
  this->GetNumberOfCells( ndim );

  // Convert to local numbering
  int ijk[3];
  ijk[0]=i-this->LoCorner[0];
  ijk[1]=j-this->LoCorner[1];
  ijk[2]=k-this->LoCorner[2];

  int N1=0,N2=0,idx=0;
  switch( this->Dimension )
    {
    case 1:
    case 3:
      N1  = ndim[0];
      N2  = ndim[1];
      idx = ijk[2]*N1*N2 + ijk[1]*N1 + ijk[0];
      break;
    case 2:
      switch( this->GridDescription )
        {
        case VTK_XY_PLANE:
          N1  = ndim[0];
          N2  = ndim[1];
          idx = ijk[2]*N1*N2 + ijk[1]*N1 + ijk[0];
          break;
        case VTK_XZ_PLANE:
          N1  = ndim[0];
          N2  = ndim[2];
          idx = ijk[1]*N1*N2 + ijk[2]*N1 + ijk[0];
          break;
        case VTK_YZ_PLANE:
          N1  = ndim[1];
          N2  = ndim[2];
          idx = ijk[0]*N1*N2 + ijk[2]*N1 + ijk[1];
          break;
        default:
          std::cerr << "Invalid 2-D topoly for AMR box!\n";
          std::cerr << "FILE: " << __FILE__ << std::endl;
          std::cerr << "LINE: " << __LINE__ << std::endl;
          std::cerr.flush();
        }
      break;
    default:
      std::cerr << "Invalid dimension for AMR box!\n";
      std::cerr << "FILE: " << __FILE__ << std::endl;
      std::cerr << "LINE: " << __LINE__ << std::endl;
      std::cerr.flush();
    }
  return( idx );
}


//-----------------------------------------------------------------------------
vtkAMRBox::vtkAMRBox(const vtkAMRBox &other)
{
  *this = other;
}

//-----------------------------------------------------------------------------
vtkAMRBox &vtkAMRBox::operator=(const vtkAMRBox &other)
{
  assert( "pre: AMR Box instance is invalid" && !other.IsInvalid() );

  if (this==&other) return *this;

  this->SetDimensionality(other.GetDimensionality());

  int lo[3];
  int hi[3];
  other.GetDimensions(lo,hi);
  this->SetDimensions(lo,hi);

  this->SetGridSpacing(other.GetGridSpacing());
  this->SetDataSetOrigin( other.GetDataSetOrigin() );

  this->GridDescription  = other.GridDescription;
  this->ProcessId        = other.ProcessId;
  this->BlockId          = other.BlockId;
  this->BlockLevel       = other.BlockLevel;
  for( int i=0; i < 6; ++i )
    {
    this->RealExtent[i] = other.RealExtent[i];
    this->NG[i]         = other.NG[i];
    }

  assert( "post: AMR Box instance is invalid" && !this->IsInvalid() );
  return *this;
}

//-----------------------------------------------------------------------------
void vtkAMRBox::Initialize( )
{
  this->Dimension       = 3;
  this->ProcessId       = 0;
  this->BlockId         = 0;
  this->BlockLevel      = 0;
  this->GridDescription = VTK_XYZ_GRID;

  for( int i=0; i < 3; ++i )
    {
    this->LoCorner[ i ] = 0;
    this->HiCorner[ i ] = 0;
    }

  this->X0[0] = this->X0[1] = this->X0[2] = 0.0;
  this->DX[0] = this->DX[1] = this->DX[2] = 1.0;
  for( int i=0; i < 6; ++i )
    {
    this->NG[i]         = 0;
    this->RealExtent[i] = 0;
    }

}

//-----------------------------------------------------------------------------
void vtkAMRBox::SetDimensionality(int dim)
{
  assert( "dimension is out-of-bounds" && ( (dim >=1) && (dim <= 3) ) );
  this->Dimension=dim;
}

//-----------------------------------------------------------------------------
void vtkAMRBox::SetGridDescription( const int desc )
{
  this->GridDescription = desc;
}

//-----------------------------------------------------------------------------
void vtkAMRBox::SetProcessId( const int pid )
{
  this->ProcessId = pid;
}

//-----------------------------------------------------------------------------
void vtkAMRBox::SetBlockId( const int blockIdx )
{
  assert( "pre: blockIdx >= 0" && (blockIdx >= 0) );
  this->BlockId = blockIdx;
}

//-----------------------------------------------------------------------------
void vtkAMRBox::SetLevel( const int level )
{
  assert( "pre: level >= 0" && (level >= 0) );
  this->BlockLevel = level;
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
    this->LoCorner[0]= ilo;
    this->LoCorner[1]= jlo;
    this->LoCorner[2]= klo;
    this->HiCorner[0]= ihi;
    this->HiCorner[1]= jhi;
    this->HiCorner[2]= khi;

    this->SetRealExtent( this->LoCorner, this->HiCorner );
    }
}

//-----------------------------------------------------------------------------
void vtkAMRBox::SetDimensions(const int *lo, const int *hi)
{
  this->SetDimensions(
      lo[0],lo[1],lo[2],
      hi[0],hi[1],hi[2] );
}

//-----------------------------------------------------------------------------
void vtkAMRBox::SetDimensions(const int *dims)
{
  this->SetDimensions(
      dims[0],dims[2],dims[4],
      dims[1],dims[3],dims[5]);
}

//-----------------------------------------------------------------------------
void vtkAMRBox::GetDimensions(int *lo, int *hi) const
{
  assert( "pre: AMR Box instance is invalid" && !this->IsInvalid() );
  for (int q=0; q < 3; ++q)
    {
    lo[q]=this->LoCorner[q];
    hi[q]=this->HiCorner[q];
    }
}

//-----------------------------------------------------------------------------
void vtkAMRBox::GetDimensions(int dims[6]) const
{
  assert( "pre: AMR Box instance is invalid" && !this->IsInvalid() );
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
  assert( "pre: AMR Box instance is invalid" && !this->IsInvalid() );
  for (int q=0; q < 3; ++q)
    {
    lo[q] = this->LoCorner[q];
    }
}

//-----------------------------------------------------------------------------
void vtkAMRBox::GetHiCorner(int *hi) const
{
  assert( "pre: AMR Box instance is invalid" && !this->IsInvalid() );
  for (int q=0; q < 3; ++q)
    {
    hi[q] = this->HiCorner[q];
    }
}

//-----------------------------------------------------------------------------
void vtkAMRBox::GetGridSpacing(double *dX) const
{
  assert( "pre: AMR Box instance is invalid" && !this->IsInvalid() );
  for (int q=0; q < 3; ++q)
    {
    dX[q] = this->DX[q];
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
  this->SetGridSpacing( dX[0], dX[1], dX[2] );
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
  assert( "pre: AMR Box instance is invalid" && !this->IsInvalid() );
  for (int q=0; q < 3; ++q)
    {
    x0[q]=this->X0[q];
    }
}

//-----------------------------------------------------------------------------
void vtkAMRBox::SetDataSetOrigin(const double *x0)
{
  this->SetDataSetOrigin( x0[0], x0[1], x0[2] );
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
  assert( "pre: input array is NULL" && (x0 != NULL) );
  x0[0] = x0[1] = x0[2] = 0.0;

  for( int i=0; i < 3; ++i )
    {
    x0[i] = this->X0[i]+this->LoCorner[i]*this->DX[i];
    }
}

//-----------------------------------------------------------------------------
void vtkAMRBox::GetNumberOfCells(int *ext) const
{
  // Sanity Check!
  assert( "pre: AMR Box instance is invalid" && !this->IsInvalid() );

  if( this->Empty( ) )
    {
    ext[0]=ext[1]=ext[2]=0;
    return;
    }

  switch( this->Dimension )
    {
    case 1:
      ext[1] = ext[2] = 0;
      ext[0] = this->HiCorner[0]-this->LoCorner[0]+1;
      break;
    case 2:
      switch( this->GridDescription )
        {
        case VTK_XY_PLANE:
          ext[2] = 0;
          ext[0] = this->HiCorner[0]-this->LoCorner[0]+1;
          ext[1] = this->HiCorner[1]-this->LoCorner[1]+1;
          break;
        case VTK_XZ_PLANE:
          ext[1] = 0;
          ext[0] = this->HiCorner[0]-this->LoCorner[0]+1;
          ext[2] = this->HiCorner[2]-this->LoCorner[2]+1;
          break;
        case VTK_YZ_PLANE:
          ext[0] = 0;
          ext[1] = this->HiCorner[1]-this->LoCorner[1]+1;
          ext[2] = this->HiCorner[2]-this->LoCorner[2]+1;
          break;
        default:
          std::cerr << "Invalid 2-D topoly for AMR box!\n";
          std::cerr << "FILE: " << __FILE__ << std::endl;
          std::cerr << "LINE: " << __LINE__ << std::endl;
          std::cerr.flush();
        }
      break;
    case 3:
      ext[0] = this->HiCorner[0]-this->LoCorner[0]+1;
      ext[1] = this->HiCorner[1]-this->LoCorner[1]+1;
      ext[2] = this->HiCorner[2]-this->LoCorner[2]+1;
      break;
    default:
      std::cerr << "Undefined dimension!\n";
      std::cerr << "FILE: " << __FILE__ << std::endl;
      std::cerr << "LINE: " << __LINE__ << std::endl;
      std::cerr.flush();
    }

}

//-----------------------------------------------------------------------------
vtkIdType vtkAMRBox::GetNumberOfCells() const
{
  assert( "pre: AMR Box instance is invalid" && !this->IsInvalid() );

  int cellExtent[3];
  this->GetNumberOfCells( cellExtent );
  int numCells = 0;
  for( int i=0; i < 3; ++i )
    {
    if( numCells == 0 )
      {
      numCells = cellExtent[i];
      }
    else if( cellExtent[i] != 0 )
      {
      numCells *= cellExtent[i];
      }
    }
  return( numCells );
}

//-----------------------------------------------------------------------------
void vtkAMRBox::GetNumberOfNodes(int *ext) const
{
  // Sanity Check!
  assert( "pre: AMR Box instance is invalid" && !this->IsInvalid() );

  if( this->Empty( ) )
    {
    ext[0]=ext[1]=ext[2]=0;
    return;
    }

  switch( this->Dimension )
    {
    case 1:
      ext[1] = ext[2] = 0;
      ext[0] = this->HiCorner[0]-this->LoCorner[0]+2;
      break;
    case 2:
      switch( this->GridDescription )
        {
        case VTK_XY_PLANE:
          ext[2] = 0;
          ext[0] = this->HiCorner[0]-this->LoCorner[0]+2;
          ext[1] = this->HiCorner[1]-this->LoCorner[1]+2;
          break;
        case VTK_XZ_PLANE:
          ext[1] = 0;
          ext[0] = this->HiCorner[0]-this->LoCorner[0]+2;
          ext[2] = this->HiCorner[2]-this->LoCorner[2]+2;
          break;
        case VTK_YZ_PLANE:
          ext[0] = 0;
          ext[1] = this->HiCorner[1]-this->LoCorner[1]+2;
          ext[2] = this->HiCorner[2]-this->LoCorner[2]+2;
          break;
        default:
          std::cerr << "Invalid 2-D topoly for AMR box!\n";
          std::cerr << "FILE: " << __FILE__ << std::endl;
          std::cerr << "LINE: " << __LINE__ << std::endl;
          std::cerr.flush();
        }
      break;
    case 3:
      ext[0] = this->HiCorner[0]-this->LoCorner[0]+2;
      ext[1] = this->HiCorner[1]-this->LoCorner[1]+2;
      ext[2] = this->HiCorner[2]-this->LoCorner[2]+2;
      break;
    default:
      std::cerr << "Undefined dimension!\n";
      std::cerr << "FILE: " << __FILE__ << std::endl;
      std::cerr << "LINE: " << __LINE__ << std::endl;
      std::cerr.flush();
    }

}

//-----------------------------------------------------------------------------
vtkIdType vtkAMRBox::GetNumberOfNodes() const
{
  assert( "pre: AMR Box instance is invalid" && !this->IsInvalid() );

  int ext[3];
  this->GetNumberOfNodes( ext );
  int numNodes = 0;
  for( int i=0; i < 3; ++i )
    {
    if( numNodes == 0 )
      {
      numNodes = ext[i];
      }
    else if( ext[i] != 0 )
      {
      numNodes *= ext[i];
      }
    }
  return( numNodes );
}

//-----------------------------------------------------------------------------
void vtkAMRBox::Grow(int byN)
{
  assert( "pre: AMR Box instance is invalid" && !this->IsInvalid() );

  if( this->Empty() )
    {
    std::cerr << "WARNING: tried growing an empty AMR box!\n";
    std::cerr << "FILE:" << __FILE__ << std::endl;
    std::cerr << "LINE:" << __LINE__ << std::endl;
    std::cerr.flush();
    return;
    }

  int lo[3];
  int hi[3];


  // TODO: One question here is, should we allow negative indices?
  //       Or should we otherwise, ensure that the box is grown with
  //       bounds.
  int q;
  switch( this->Dimension )
    {
    case 1:
      lo[1] = this->LoCorner[1];
      lo[2] = this->LoCorner[2];
      hi[1] = this->HiCorner[1];
      hi[2] = this->HiCorner[2];
      lo[0] = this->LoCorner[0]-byN;
      hi[0] = this->HiCorner[0]+byN;
      this->NG[0]++;
      this->NG[1]++;
      break;
    case 2:
      switch( this->GridDescription )
        {
        case VTK_XY_PLANE:
          lo[2] = this->LoCorner[2];
          hi[2] = this->HiCorner[2];
          for( q=0; q < 2; ++q )
            {
            lo[q] = this->LoCorner[q]-byN;
            hi[q] = this->HiCorner[q]+byN;
            this->NG[q*2]++;
            this->NG[q*2+1]++;
            }
          break;
        case VTK_XZ_PLANE:
          lo[1] = this->LoCorner[1];
          hi[1] = this->HiCorner[1];

          // grow along x
          lo[0] = this->LoCorner[0]-byN;
          hi[0] = this->HiCorner[0]+byN;
          this->NG[0]++;
          this->NG[1]++;

          // grow along z
          lo[2] = this->LoCorner[2]-byN;
          hi[2] = this->HiCorner[2]+byN;
          this->NG[4]++;
          this->NG[5]++;
          break;
        case VTK_YZ_PLANE:
          lo[0] = this->LoCorner[0];
          hi[0] = this->HiCorner[0];
          for( q=1; q < 3; ++q )
            {
            lo[q] = this->LoCorner[q]-byN;
            hi[q] = this->HiCorner[q]+byN;
            this->NG[q*2]++;
            this->NG[q*2+1]++;
            }
          break;
        default:
          std::cerr << "Invalid 2-D topoly for AMR box!\n";
          std::cerr << "FILE: " << __FILE__ << std::endl;
          std::cerr << "LINE: " << __LINE__ << std::endl;
          std::cerr.flush();
        }
      break;
    case 3:
      for( q=0; q < 3; ++q )
        {
        lo[q] = this->LoCorner[q]-byN;
        hi[q] = this->HiCorner[q]+byN;
        this->NG[q*2]++;
        this->NG[q*2+1]++;
        }
      break;
    default:
      // Code should not reach here!
      std::cerr << "Undefined dimension! @" << __FILE__ << __LINE__ << "\n";
      std::cerr.flush();
      assert( false );
    }
  this->SetDimensions(lo,hi);
  assert( "post: Grown AMR Box instance is invalid" && !this->IsInvalid() );
}

//-----------------------------------------------------------------------------
void vtkAMRBox::Shrink(int byN)
{
  assert( "pre: AMR Box instance is invalid" && !this->IsInvalid() );

  if( this->Empty() )
    {
    std::cerr << "WARNING: tried shrinking an empty AMR box!\n";
    std::cerr << "FILE:" << __FILE__ << std::endl;
    std::cerr << "LINE:" << __LINE__ << std::endl;
    std::cerr.flush();
    return;
    }

  int lo[3];
  int hi[3];

  // TODO: One question here is, should we allow negative indices?
  //       Or should we otherwise, ensure that the box is grown with
  //       bounds.
  int q;
  switch( this->Dimension )
    {
    case 1:
      lo[0] = this->LoCorner[0]+byN;
      hi[0] = this->HiCorner[0]-byN;
      break;
    case 2:
      switch( this->GridDescription )
        {
        case VTK_XY_PLANE:
          lo[2] = this->LoCorner[2];
          hi[2] = this->HiCorner[2];
          for( q=0; q < 2; ++q )
            {
            lo[q] = this->LoCorner[q]+byN;
            hi[q] = this->HiCorner[q]-byN;
            }
          break;
        case VTK_XZ_PLANE:
          lo[1] = this->LoCorner[1];
          hi[1] = this->HiCorner[1];
          // grow along x
          lo[0] = this->LoCorner[0]+byN;
          hi[0] = this->HiCorner[0]-byN;

          // grow along z
          lo[2] = this->LoCorner[2]+byN;
          hi[2] = this->HiCorner[2]-byN;
          break;
        case VTK_YZ_PLANE:
          lo[0] = this->LoCorner[0];
          hi[0] = this->HiCorner[0];
          for( q=1; q < 3; ++q )
            {
            lo[q] = this->LoCorner[q]+byN;
            hi[q] = this->HiCorner[q]-byN;
            }
          break;
        default:
          std::cerr << "Invalid 2-D topoly for AMR box!\n";
          std::cerr << "FILE: " << __FILE__ << std::endl;
          std::cerr << "LINE: " << __LINE__ << std::endl;
          std::cerr.flush();
        }
      break;
    case 3:
      for( q=0; q < 3; ++q )
        {
        lo[q] = this->LoCorner[q]+byN;
        hi[q] = this->HiCorner[q]-byN;
        }
      break;
    default:
      // Code should not reach here!
      std::cerr << "Undefined dimension! @" << __FILE__ << __LINE__ << "\n";
      std::cerr.flush();
      assert( false );
    }
  this->SetDimensions(lo,hi);
  assert( "post: Grown AMR Box instance is invalid" && !this->IsInvalid() );
}

//-----------------------------------------------------------------------------
void vtkAMRBox::Shift(int i, int j)
{
  assert( "pre: AMR Box instance is invalid" && !this->IsInvalid() );
  if( this->Empty() )
    {
    std::cerr << "WARNING: tried shrinking an empty AMR box!\n";
    std::cerr << "FILE:" << __FILE__ << std::endl;
    std::cerr << "LINE:" << __LINE__ << std::endl;
    std::cerr.flush();
    return;
    }

  // TODO: There is no bound checking here!
  // ijk's can be negative,
  this->SetDimensions(
    this->LoCorner[0]+i, this->LoCorner[1]+j,this->LoCorner[2],
    this->HiCorner[0]+i, this->HiCorner[1]+j,this->HiCorner[2] );
}

//-----------------------------------------------------------------------------
void vtkAMRBox::Shift(int i, int j, int k)
{
  assert( "pre: AMR Box instance is invalid" && !this->IsInvalid() );
  if( this->Empty() )
    {
    std::cerr << "WARNING: tried shrinking an empty AMR box!\n";
    std::cerr << "FILE:" << __FILE__ << std::endl;
    std::cerr << "LINE:" << __LINE__ << std::endl;
    std::cerr.flush();
    return;
    }

  int ijk[3]; ijk[0] = i; ijk[1] = j; ijk[2] = k;

  // TODO: One question here is, should we allow negative indices?
  //       Or should we otherwise, ensure that the box is grown with
  //       bounds.
  int q;
  switch( this->Dimension )
    {
    case 1:
      this->LoCorner[0] = this->LoCorner[0]+i;
      this->HiCorner[0] = this->HiCorner[0]+i;
      break;
    case 2:
      switch( this->GridDescription )
        {
        case VTK_XY_PLANE:
          for( q=0; q < 2; ++q )
            {
            this->LoCorner[q] = this->LoCorner[q]+ijk[q];
            this->HiCorner[q] = this->HiCorner[q]+ijk[q];
            }
          break;
        case VTK_XZ_PLANE:
          // shift in x
          this->LoCorner[0] = this->LoCorner[0]+i;
          this->HiCorner[0] = this->HiCorner[0]+i;

          // shift in z
          this->LoCorner[2] = this->LoCorner[2]+k;
          this->HiCorner[2] = this->HiCorner[2]+k;
          break;
        case VTK_YZ_PLANE:
          for( q=1; q < 3; ++q )
            {
            this->LoCorner[q] = this->LoCorner[q]+ijk[q];
            this->HiCorner[q] = this->HiCorner[q]+ijk[q];
            }
          break;
        default:
          std::cerr << "Invalid 2-D topoly for AMR box!\n";
          std::cerr << "FILE: " << __FILE__ << std::endl;
          std::cerr << "LINE: " << __LINE__ << std::endl;
          std::cerr.flush();
        }
      break;
    case 3:
      for( q=0; q < 3; ++q )
        {
        this->LoCorner[q] = this->LoCorner[q]+ijk[q];
        this->HiCorner[q] = this->HiCorner[q]+ijk[q];
        }
      break;
    default:
      // Code should not reach here!
      std::cerr << "Undefined dimension! @" << __FILE__ << __LINE__ << "\n";
      std::cerr.flush();
      assert( false );
    }
  assert( "post: Shifted AMR Box instance is invalid" && !this->IsInvalid() );
}

//-----------------------------------------------------------------------------
void vtkAMRBox::Shift(const int *I)
{
  this->Shift( I[0], I[1], I[2] );
}

//-----------------------------------------------------------------------------
double vtkAMRBox::GetMinX() const
{
  double pnt[3];
  this->GetMinBounds( pnt );
  return( pnt[0] );
}

//-----------------------------------------------------------------------------
double vtkAMRBox::GetMinY() const
{
  double pnt[3];
  this->GetMinBounds( pnt );
  return( pnt[1] );
}

//-----------------------------------------------------------------------------
double vtkAMRBox::GetMinZ() const
{
  double pnt[3];
  this->GetMinBounds( pnt );
  return( pnt[2] );
}

//-----------------------------------------------------------------------------
double vtkAMRBox::GetMaxX() const
{
  double pnt[3];
  this->GetMaxBounds( pnt );
  return( pnt[0] );
}

//-----------------------------------------------------------------------------
double vtkAMRBox::GetMaxY() const
{
  double pnt[3];
  this->GetMaxBounds( pnt );
  return( pnt[1] );
}

//-----------------------------------------------------------------------------
double vtkAMRBox::GetMaxZ() const
{
  double pnt[3];
  this->GetMaxBounds( pnt );
  return( pnt[2] );
}

//-----------------------------------------------------------------------------
void vtkAMRBox::GetMinBounds( double min[3] ) const
{
  min[0] = min[1] = min[2] = 0.0;
  for( int i=0; i < 3; ++i )
    {
    min[ i ] = this->X0[i]+this->LoCorner[i]*this->DX[i];
    }
}

//-----------------------------------------------------------------------------
void vtkAMRBox::GetMaxBounds( double max[3] ) const
{
  max[0] = max[1] = max[2] = 0.0;
  for( int i=0; i < 3; ++i )
    {
    max[ i ] =this->X0[i]+(this->HiCorner[i]+1)*this->DX[i];
    }
}

//-----------------------------------------------------------------------------
void vtkAMRBox::GetBounds( double bounds[6] ) const
{
  int i, j;
  for( i=0, j=0; i < 3; ++i )
    {
    bounds[ j++ ] = this->X0[i]+this->LoCorner[i]*this->DX[i];
    bounds[ j++ ] = this->X0[i]+(this->HiCorner[i]+1)*this->DX[i];
    }
}

//-----------------------------------------------------------------------------
bool vtkAMRBox::HasPoint( const double x, const double y, const double z )
{
  assert( "pre: AMR Box instance is invalid" && !this->IsInvalid() );

  double min[3];
  double max[3];
  this->GetMinBounds( min );
  this->GetMaxBounds( max );

  switch( this->Dimension )
    {
    case 1:
      if( x >= min[0] && x <= max[0]  )
        {
        return true;
        }
      break;
    case 2:
      switch( this->GridDescription )
        {
        case VTK_XY_PLANE:
          return(
           (x >= min[0] && x <= max[0] &&
            y >= min[1] && y <= max[1]) );
          break;
        case VTK_XZ_PLANE:
          return(
           (x >= min[0] && x <= max[0] &&
            z >= min[2] && z <= max[2]) );
          break;
        case VTK_YZ_PLANE:
          return(
           (y >= min[1] && y <= max[1] &&
            z >= min[2] && z <= max[2]) );
          break;
        default:
          std::cerr << "Invalid 2-D topoly for AMR box!\n";
          std::cerr << "FILE: " << __FILE__ << std::endl;
          std::cerr << "LINE: " << __LINE__ << std::endl;
          std::cerr.flush();
        }
      break;
    case 3:
      if( x >= min[0] && x <= max[0] &&
          y >= min[1] && y <= max[1] &&
          z >= min[2] && z <= max[2]   )
        {
        return true;
        }
      break;
    default:
      std::cerr << "Error code should not reach here!";
      std::cerr.flush();
      assert(false);
    }
  return false;

}

//-----------------------------------------------------------------------------
bool vtkAMRBox::operator==(const vtkAMRBox &other)
{
  // TODO: fix this to check for equality of meta-data as well?
  if ( this->Dimension != other.Dimension)
    {
    return false;
    }

  if( this->BlockLevel != other.BlockLevel )
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
bool vtkAMRBox::DoesBoxIntersectAlongDimension(
      const vtkAMRBox &other, const int q)
{
  int minVal = 0;
  int maxVal = 0;
  minVal = (this->LoCorner[q] < other.LoCorner[q]) ?
      other.LoCorner[q] : this->LoCorner[q];
  maxVal = (this->HiCorner[q] > other.HiCorner[q]) ?
      other.HiCorner[q] : this->HiCorner[q];

  if (minVal >= maxVal)
    {
    return false;
    }
  return true;
}

//-----------------------------------------------------------------------------
bool vtkAMRBox::DoesIntersect(const vtkAMRBox &other)
{
  assert( "pre: AMR Box instance is invalid" && !this->IsInvalid() );
  if (this->Dimension!=other.Dimension)
    {
    vtkGenericWarningMacro(
      "Can't operate on a " << this->Dimension
      << "D box with a " << other.Dimension << "D box.");
    return false;
    }
  if (this->Empty())
    {
    return false;
    }
  if (other.Empty())
    {
    return false;
    }

  // Compare each coordinate of the corners.  Stop if at
  // anytime the box becomes invalid - i.e. there is no intersection
  switch( this->Dimension )
    {
    case 2:
      switch( this->GridDescription )
        {
        case VTK_XY_PLANE:
          if( !this->DoesBoxIntersectAlongDimension(other,0) ||
              !this->DoesBoxIntersectAlongDimension(other,1) )
            {
            return false;
            }
          break;
        case VTK_XZ_PLANE:
          if( !this->DoesBoxIntersectAlongDimension(other,0) ||
              !this->DoesBoxIntersectAlongDimension(other,2) )
            {
            return false;
            }
          break;
        case VTK_YZ_PLANE:
          if( !this->DoesBoxIntersectAlongDimension(other,1) ||
              !this->DoesBoxIntersectAlongDimension(other,2) )
            {
            return false;
            }
          break;
        default:
          std::cerr << "Invalid 2-D topoly for AMR box!\n";
          std::cerr << "FILE: " << __FILE__ << std::endl;
          std::cerr << "LINE: " << __LINE__ << std::endl;
          std::cerr.flush();
        }
        break;
    case 3:
      if( !this->DoesBoxIntersectAlongDimension(other,0) ||
          !this->DoesBoxIntersectAlongDimension(other,1) ||
          !this->DoesBoxIntersectAlongDimension(other,2) )
        {
        return false;
        }
        break;
    default:
      vtkGenericWarningMacro( "Can't operate on a " << this->Dimension );
      return false;
    }
  return true;
}

//-----------------------------------------------------------------------------
bool vtkAMRBox::IntersectBoxAlongDimension(const vtkAMRBox &other, const int q)
{
  assert( "pre: dimension is out-of-bounds!" && (q >= 0) && (q <= 2) );
  if( this->LoCorner[q] <= other.LoCorner[q] )
    {
    this->LoCorner[q] = other.LoCorner[q];
    }
  if (this->HiCorner[q] >= other.HiCorner[q])
    {
    this->HiCorner[q] = other.HiCorner[q];
    }
  if (this->LoCorner[q] >= this->HiCorner[q])
    {
    return false;
    }
  return true;
}

//-----------------------------------------------------------------------------
bool vtkAMRBox::Intersect(const vtkAMRBox &other)
{
  assert( "pre: AMR Box instance is invalid" && !this->IsInvalid() );
  if (this->Dimension!=other.Dimension)
    {
    vtkGenericWarningMacro(
      "Can't operate on a " << this->Dimension
      << "D box with a " << other.Dimension << "D box.");
    return false;
    }
  if (this->Empty())
    {
    return false;
    }
  if (other.Empty())
    {
    this->Invalidate();
    return false;
    }

  // Compare each coordinate of the corners.  Stop if at
  // anytime the box becomes invalid - i.e. there is no intersection
  switch( this->Dimension )
    {
    case 2:
      switch( this->GridDescription )
        {
        case VTK_XY_PLANE:
          if( !this->IntersectBoxAlongDimension(other,0) ||
              !this->IntersectBoxAlongDimension(other,1) )
            {
            return false;
            }
          break;
        case VTK_XZ_PLANE:
          if( !this->IntersectBoxAlongDimension(other,0) ||
              !this->IntersectBoxAlongDimension(other,2) )
            {
            return false;
            }
          break;
        case VTK_YZ_PLANE:
          if( !this->IntersectBoxAlongDimension(other,1) ||
              !this->IntersectBoxAlongDimension(other,2) )
            {
            return false;
            }
          break;
        default:
          std::cerr << "Invalid 2-D topoly for AMR box!\n";
          std::cerr << "FILE: " << __FILE__ << std::endl;
          std::cerr << "LINE: " << __LINE__ << std::endl;
          std::cerr.flush();
        }
        break;
    case 3:
      if( !this->IntersectBoxAlongDimension(other,0) ||
          !this->IntersectBoxAlongDimension(other,1) ||
          !this->IntersectBoxAlongDimension(other,2) )
        {
        return false;
        }
        break;
    default:
      vtkGenericWarningMacro( "Can't operate on a " << this->Dimension );
      return false;
    }
  return true;
}

//-----------------------------------------------------------------------------
bool vtkAMRBox::Contains(int i,int j,int k) const
{
  assert( "pre: AMR Box instance is invalid" && !this->IsInvalid() );

  if( this->Empty() )
    {
    return false;
    }

   switch (this->Dimension)
    {
    case 1:
      if( this->LoCorner[0]<=i && this->HiCorner[0]>=i )
        {
        return true;
        }
      break;
    case 2:
      switch( this->GridDescription )
        {
        case VTK_XY_PLANE:
          return(
           ( this->LoCorner[0]<=i && this->HiCorner[0]>=i &&
             this->LoCorner[1]<=j && this->HiCorner[1]>=j) );
          break;
        case VTK_XZ_PLANE:
          return(
           ( this->LoCorner[0]<=i && this->HiCorner[0]>=i &&
             this->LoCorner[2]<=k && this->HiCorner[2]>=k) );
          break;
        case VTK_YZ_PLANE:
          return(
           ( this->LoCorner[1]<=j && this->HiCorner[1]>=j &&
             this->LoCorner[2]<=k && this->HiCorner[2]>=k) );
          break;
        default:
          std::cerr << "Invalid 2-D topoly for AMR box!\n";
          std::cerr << "FILE: " << __FILE__ << std::endl;
          std::cerr << "LINE: " << __LINE__ << std::endl;
          std::cerr.flush();
        }
      break;
    case 3:
      if( this->LoCorner[0]<=i && this->HiCorner[0]>=i &&
          this->LoCorner[1]<=j && this->HiCorner[1]>=j &&
          this->LoCorner[2]<=k && this->HiCorner[2]>=k  )
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
  return this->Contains( I[0],I[1],I[2]);
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
  assert( "pre: Input refinement ratio must be >= 2" && (r >= 2)  );
  assert( "pre: AMR Box instance is invalid" && !this->IsInvalid() );

  if( this->Empty() )
    {
    std::cerr << "WARNING: tried refining an empty AMR box!\n";
    std::cerr << "FILE:" << __FILE__ << std::endl;
    std::cerr << "LINE:" << __LINE__ << std::endl;
    std::cerr.flush();
    return;
    }

  int q;
 switch( this->Dimension )
   {
     case 1:
       this->LoCorner[0] *= r;
       this->HiCorner[0]  = (this->HiCorner[0]+1)*r-1;
       this->DX[0] /= r;
       break;
     case 2:
       switch( this->GridDescription )
         {
         case VTK_XY_PLANE:
          for( q=0; q < 2; ++q )
           {
           this->LoCorner[q]=this->LoCorner[q]*r;
           this->HiCorner[q]=(this->HiCorner[q]+1)*r-1;
           this->DX[q] /= r;
           }
           break;
         case VTK_XZ_PLANE:
           // Refine x
           this->LoCorner[0] *= r;
           this->HiCorner[0]  = (this->HiCorner[0]+1)*r-1;
           this->DX[0] /= r;

           // Refine z
           this->LoCorner[2] *= r;
           this->HiCorner[2]  = (this->HiCorner[2]+1)*r-1;
           this->DX[2] /= r;
           break;
         case VTK_YZ_PLANE:
          for( q=1; q < 3; ++q )
            {
            this->LoCorner[q]=this->LoCorner[q]*r;
            this->HiCorner[q]=(this->HiCorner[q]+1)*r-1;
            this->DX[q] /= r;
            }
           break;
         default:
           std::cerr << "Invalid 2-D topoly for AMR box!\n";
           std::cerr << "FILE: " << __FILE__ << std::endl;
           std::cerr << "LINE: " << __LINE__ << std::endl;
           std::cerr.flush();
         }
       break;
     case 3:
       for( q=0; q < 3; ++q )
         {
         this->LoCorner[q]=this->LoCorner[q]*r;
         this->HiCorner[q]=(this->HiCorner[q]+1)*r-1;
         this->DX[q] /= r;
         }
       break;
     default:
       // Code should not reach here!
       std::cerr << "Undefined dimension! @" << __FILE__ << __LINE__ << "\n";
       std::cerr.flush();
       assert( false );
   }
  assert( "post: Refined AMR box should not be empty!" && !this->Empty() );
  assert( "post: Refined AMR Box instance is invalid" && !this->IsInvalid() );

}

//-----------------------------------------------------------------------------
void vtkAMRBox::Coarsen(int r)
{
  assert( "pre: Input refinement ratio must be >= 2" && (r >= 2)  );
  assert( "pre: AMR Box instance is invalid" && !this->IsInvalid() );

  if( this->Empty() )
    {
    std::cerr << "WARNING: tried refining an empty AMR box!\n";
    std::cerr << "FILE:" << __FILE__ << std::endl;
    std::cerr << "LINE:" << __LINE__ << std::endl;
    std::cerr.flush();
    return;
    }


  int lo[3];
  int hi[3];
  int q;

  int nCells[3];
  this->GetNumberOfCells( nCells );

  switch( this->Dimension )
    {
    case 1:
      lo[1]=this->LoCorner[1];
      lo[2]=this->LoCorner[2];
      hi[1]=this->HiCorner[1];
      hi[2]=this->HiCorner[2];

      lo[0]=( (this->LoCorner[0]<0)?
         -abs(this->LoCorner[0]+1)/r-1 : this->LoCorner[0]/r);
      hi[0]=( (this->HiCorner[0]<0)?
         -abs(this->HiCorner[0]+1)/r-1 : this->HiCorner[0]/r );
      this->DX[0]*=r;
      break;
    case 2:
      switch( this->GridDescription )
        {
        case VTK_XY_PLANE:
          lo[2] = this->LoCorner[2];
          hi[2] = this->HiCorner[2];
          for( q=0; q < 2; ++q )
            {
            lo[q]=( (this->LoCorner[q] < 0)?
                    -abs(this->LoCorner[q]+1)/r-1 : this->LoCorner[q]/r);
            hi[q]=( this->HiCorner[q]<0 ?
                    -abs(this->HiCorner[q]+1)/r-1 : this->HiCorner[q]/r);
            this->DX[q]*=r;
            }
          break;
        case VTK_XZ_PLANE:
          // keep y fixed
          lo[1] = this->LoCorner[1];
          hi[1] = this->HiCorner[1];

          // coarsen x
          lo[0]=( (this->LoCorner[0]<0)?
             -abs(this->LoCorner[0]+1)/r-1 : this->LoCorner[0]/r);
          hi[0]=( (this->HiCorner[0]<0)?
             -abs(this->HiCorner[0]+1)/r-1 : this->HiCorner[0]/r );
          this->DX[0]*=r;

          // coarsen y
          lo[2]=( (this->LoCorner[2]<0)?
             -abs(this->LoCorner[2]+1)/r-1 : this->LoCorner[2]/r);
          hi[2]=( (this->HiCorner[2]<0)?
             -abs(this->HiCorner[2]+1)/r-1 : this->HiCorner[2]/r );
          this->DX[2]*=r;
          break;
        case VTK_YZ_PLANE:
          lo[0] = this->LoCorner[0];
          hi[0] = this->HiCorner[0];
          for( q=1; q < 3; ++q )
            {
            lo[q] = ( (this->LoCorner[q] < 0)?
               -abs(this->LoCorner[q]+1)/r-1 : this->LoCorner[q]/r);
            hi[q] = ( this->HiCorner[q]<0 ?
               -abs(this->HiCorner[q]+1)/r-1 : this->HiCorner[q]/r);
            this->DX[q]*=r;
            }
          break;
        default:
          std::cerr << "Invalid 2-D topoly for AMR box!\n";
          std::cerr << "FILE: " << __FILE__ << std::endl;
          std::cerr << "LINE: " << __LINE__ << std::endl;
          std::cerr.flush();
        }
      break;
    case 3:
      for( q=0; q < 3; ++q )
        {
        lo[q]=( (this->LoCorner[q] < 0)?
           -abs(this->LoCorner[q]+1)/r-1 : this->LoCorner[q]/r);
        hi[q]=( this->HiCorner[q]<0 ?
          -abs(this->HiCorner[q]+1)/r-1 : this->HiCorner[q]/r);
        this->DX[q]*=r;
        }
      break;
    default:
      std::cerr << "ERROR: Undefined dimension, code should not reach here!\n";
      std::cerr << "FILE: " << __FILE__ << std::endl;
      std::cerr << "LINE: " << __LINE__ << std::endl;
      assert( false );
    }
//  this->DX[0] *= r;
//  this->DX[1] *= r;
//  this->DX[2] *= r;
  this->SetDimensions( lo, hi );
  assert( "post: Coarsened AMR box should not be empty!" && !this->Empty() );
  assert( "post: Coarsened AMR Box instance is invalid" && !this->IsInvalid() );
}

//-----------------------------------------------------------------------------
ostream &vtkAMRBox::Print(ostream &os) const
{
  os << this->Dimension << "-D AMR box => "
     << "Low: ("  << this->LoCorner[0]
     << ","  << this->LoCorner[1]
     << ","  << this->LoCorner[2]
     << ") High: (" << this->HiCorner[0]
     << ","  << this->HiCorner[1]
     << ","  << this->HiCorner[2]
     << ") Origin: (" << this->X0[0]
     << ","  << this->X0[1]
     << ","  << this->X0[2]
     << ") Spacing: (" << this->DX[0]
     << ","  << this->DX[1]
     << ","  << this->DX[2]
     << ")";
  return os;
}

//-----------------------------------------------------------------------------
void vtkAMRBox::GetRealExtent( int realext[6] ) const
{
  realext[0] = this->RealExtent[0];
  realext[1] = this->RealExtent[1];
  realext[2] = this->RealExtent[2];
  realext[3] = this->RealExtent[3];
  realext[4] = this->RealExtent[4];
  realext[5] = this->RealExtent[5];
}

//-----------------------------------------------------------------------------
void vtkAMRBox::SetRealExtent( int realExtent[6] )
{
  this->RealExtent[0] = realExtent[0]; // imin
  this->RealExtent[1] = realExtent[1]; // imax
  this->RealExtent[2] = realExtent[2]; // jmin
  this->RealExtent[3] = realExtent[3]; // jmax
  this->RealExtent[4] = realExtent[4]; // kmin
  this->RealExtent[5] = realExtent[5]; // kmax
}

//-----------------------------------------------------------------------------
void vtkAMRBox::SetRealExtent( int min[3], int max[3] )
{
  this->RealExtent[0] = min[0]; // imin
  this->RealExtent[1] = max[0]; // imax
  this->RealExtent[2] = min[1]; // jmin
  this->RealExtent[3] = max[1]; // jmax
  this->RealExtent[4] = min[2]; // kmin
  this->RealExtent[5] = max[2]; // kmax
}

//-----------------------------------------------------------------------------
bool vtkAMRBox::IsGhostNode( const int i, const int j, const int k )
{
  bool status = false;
  switch( this->Dimension )
    {
    case 1:
      if( (i < this->RealExtent[0]) || (i > this->RealExtent[1])    )
        {
        status = true;
        }
      break;
    case 2:
      if( (i < this->RealExtent[0]) || (i > this->RealExtent[1]) ||
          (j < this->RealExtent[2]) || (j > this->RealExtent[3])    )
        {
        status = true;
        }
      break;
    case 3:
      if( (i < this->RealExtent[0]) || (i > this->RealExtent[1]) ||
          (j < this->RealExtent[2]) || (j > this->RealExtent[3]) ||
          (k < this->RealExtent[4]) || (k > this->RealExtent[5])    )
        {
        status = true;
        }
      break;
    default:
      // Code should not reach here!
      // TODO: Better error handling of this case!
      this->Invalidate();
    }
  return status;
}

//------------------------------------------------------------------------------
void vtkAMRBox::GetPoint( const int ijk[3], double pnt[3] ) const
{
  // Compiler should unroll this small loop!
  pnt[0] = 0.0; pnt[1] = 0.0; pnt[2] = 0.0;
  for( int i=0; i < 3; ++i )
    {
    // Sanity Check!
    assert( "pre: ijk index out-of-bounds" &&
        (ijk[i]>=this->LoCorner[i]) && (ijk[i]<=this->HiCorner[i]) );

    if( ijk[i] == 0 )
      {
      pnt[i] = this->X0[i];
      }
    else
      {
      pnt[i] = this->X0[i]+ijk[i]*this->DX[i];
      }
    }
}

//------------------------------------------------------------------------------
void vtkAMRBox::GetPoint(
    const int i, const int j, const int k, double pnt[3] ) const
{
  int ijk[3] = {i,j,k};
  this->GetPoint(ijk, pnt);
}

//-----------------------------------------------------------------------------
void vtkAMRBox::Serialize( unsigned char*& buffer, vtkIdType& bytesize)
{
  assert( "pre: input buffer is expected to be NULL" && (buffer==NULL) );

  bytesize       = vtkAMRBox::GetBytesize();
  buffer         = new unsigned char[ bytesize ];
  assert( buffer != NULL );

  // STEP 0: set pointer to traverse the buffer
  unsigned char* ptr = buffer;

  // STEP 1: serialize the dimension
  std::memcpy( ptr, &(this->Dimension), sizeof(int) );
  ptr += sizeof( int );

  // STEP 2: serialize the coordinates array
  std::memcpy( ptr, &(this->X0), (3*sizeof(double)) );
  ptr += 3*sizeof( double );

  // STEP 3: serialize the spacing array
  std::memcpy( ptr, &(this->DX), (3*sizeof(double) ) );
  ptr += 3*sizeof( double );

  // STEP 4: serialize the block ID
  std::memcpy( ptr, &(this->BlockId), sizeof(int) );
  ptr += sizeof( int );

  // STEP 5: serialize the process ID
  std::memcpy(ptr, &(this->ProcessId), sizeof(int) );
  ptr += sizeof( int );

  // STEP 6: serialize the block level
  std::memcpy(ptr, &(this->BlockLevel), sizeof(int) );
  ptr += sizeof( int );

  // STEP 7: serialize the low corner
  std::memcpy(ptr, &(this->LoCorner), 3*sizeof(int) );
  ptr += 3*sizeof( int );

  // STEP 8: serialize the high corner
  std::memcpy(ptr, &(this->HiCorner), 3*sizeof(int) );
  ptr += 3*sizeof( int );

  // STEP 9: serialize real-extent
  std::memcpy(ptr, &(this->RealExtent), 6*sizeof(int));
  ptr += 6*sizeof( int );

  // STEP 10: serialize Grid description
  std::memcpy(ptr, &(this->GridDescription), sizeof(int) );
  ptr += sizeof( int );
}

//-----------------------------------------------------------------------------
void vtkAMRBox::Deserialize(
    unsigned char* buffer, const vtkIdType& vtkNotUsed(bytesize) )
{
  assert( "pre: input buffer is NULL" && (buffer != NULL) );

  // STEP 0: set pointer to traverse the buffer
  unsigned char *ptr = buffer;

  // STEP 1: de-serialize the dimension
  std::memcpy( &(this->Dimension), ptr, sizeof(int) );
  ptr += sizeof( int );
  assert( ptr != NULL );

  // STEP 2: de-serialize the coordinates
  std::memcpy( &(this->X0), ptr, 3*sizeof(double) );
  ptr += 3*sizeof(double);
  assert( ptr != NULL );

  // STEP 3: de-serialize the spacing array
  std::memcpy( &(this->DX), ptr, 3*sizeof(double) );
  ptr += 3*sizeof(double);
  assert( ptr != NULL );

  // STEP 4: de-serialize the block ID
  std::memcpy( &(this->BlockId), ptr, sizeof(int) );
  ptr += sizeof( int );
  assert( ptr != NULL );

  // STEP 5: de-serialize the process ID
  std::memcpy( &(this->ProcessId), ptr, sizeof(int) );
  ptr += sizeof( int );
  assert( ptr != NULL );

  // STEP 6: de-serialize the block level
  std::memcpy(&(this->BlockLevel), ptr, sizeof(int) );
  ptr += sizeof( int );
  assert( ptr != NULL );

  // STEP 7: de-serialize the low corner
  std::memcpy( &(this->LoCorner), ptr, 3*sizeof(int) );
  ptr += 3*sizeof( int );
  assert( ptr != NULL );

  // STEP 8: de-serialize the high corner
  std::memcpy(&(this->HiCorner), ptr,  3*sizeof(int) );
  ptr += 3*sizeof( int );
  assert( ptr != NULL );

  // STEP 9: de-serialize the real extent
  std::memcpy(&(this->RealExtent), ptr, 6*sizeof(int) );
  ptr += 6*sizeof( int );
  assert( ptr != NULL );

  // STEP 10: de-serialize the grid description
  std::memcpy( &(this->GridDescription), ptr, sizeof(int) );
  ptr += sizeof(int);
  assert( ptr != NULL );
}

//-----------------------------------------------------------------------------
//int vtkAMRBox::DetectDimension(
//    const int ilo, const int jlo, const int klo,
//    const int ihi, const int jhi, const int khi )
//{
//
//  int dim = 3;
//  if( ilo == ihi )
//    {
//      --dim;
//    }
//  if( jlo == jhi )
//    {
//      --dim;
//    }
//  if( klo == khi )
//    {
//      --dim;
//    }
//
//  if( dim < 1 || dim > 3 )
//    {
//      std::cerr << "WARNING: Invalid dimension: " << dim << std::endl;
//      std::cerr << "FILE: " << __FILE__ << std::endl;
//      std::cerr << "LINE: " << __LINE__ << std::endl;
//    }
//  assert( "pre: Invalid dimension detected" &&
//          ( (dim >= 1) && (dim <= 3) ) );
//
//  return( dim );
//}

//-----------------------------------------------------------------------------
bool vtkAMRBox::Collides( vtkAMRBox &b1, vtkAMRBox &b2)
{
  double min1[3];
  double max1[3];
  double min2[3];
  double max2[3];

  // Sanity check
  assert( "pre: boxes must be of the same dimension" &&
          (b1.GetDimensionality() == b2.GetDimensionality()));

  b1.GetMinBounds( min1 );
  b1.GetMaxBounds( max1 );
  b2.GetMinBounds( min2 );
  b2.GetMaxBounds( max2 );

  for( int i=0; i < b1.GetDimensionality(); ++i )
    {
    if( (min1[i] >= min2[i]) && (min1[i] <= max2[i]) )
      {
      continue;
      }
    if( (min2[i] >= min1[i]) && (min2[i] <= max1[i]) )
      {
      continue;
      }
    if( (max1[i] >= min2[i]) && (max1[i] <= max2[i]) )
      {
      continue;
      }
    if( (max2[i] >= min1[i]) && (max2[i] <= max1[i]) )
      {
      continue;
      }
    return false;
    } // End for all directions
  return true;
}















//*****************************************************************************
void Split(
      const int N[3],
      const int minSide[3],
      std::vector<vtkAMRBox> &decomp)
{
  std::vector<vtkAMRBox> tDecomp; // Working array for resulting splits
  std::vector<vtkAMRBox> aDecomp; // and for atomic boxes.

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
  std::vector<vtkAMRBox> tDecomp; // Working array for resulting splits
  std::vector<vtkAMRBox> aDecomp; // and for atomic boxes.
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

//----------------------------------------------------------------------------
void vtkAMRBox::WriteBox( )
{
  this->WriteBox(
      this->GetMinX(),this->GetMinY(), this->GetMinZ(),
      this->GetMaxX(),this->GetMaxY(), this->GetMaxZ() );
}

//----------------------------------------------------------------------------
void vtkAMRBox::WriteBox(
      const double x, const double y, const double z,
      const double X, const double Y, const double Z )
{
  std::ostringstream oss;
  oss.str( "" ); oss.clear( );
  oss << "Box" << this->Dimension << "D_";
  oss << this->BlockId << "_Level_" << this->BlockLevel << ".vtk";

  std::ofstream ofs;
  ofs.open( oss.str().c_str( ) );
  assert( "Cannot open file" && ( ofs.is_open() ) );

  ofs << "# vtk DataFile Version 3.0\n";
  ofs << oss.str( ) << std::endl;
  ofs << "ASCII\n";
  ofs << "DATASET UNSTRUCTURED_GRID\n";
  ofs << "POINTS 8 double\n";
  ofs << x << " " << y << " " << z << std::endl;
  ofs << X << " " << y << " " << z << std::endl;
  ofs << X << " " << Y << " " << z << std::endl;
  ofs << x << " " << Y << " " << z << std::endl;
  ofs << x << " " << y << " " << Z << std::endl;
  ofs << X << " " << y << " " << Z << std::endl;
  ofs << X << " " << Y << " " << Z << std::endl;
  ofs << x << " " << Y << " " << Z << std::endl;
  ofs << "CELLS 1 9\n";
  ofs << "8 0 1 2 3 4 5 6 7\n";
  ofs << "CELL_TYPES 1\n";
  ofs << "12\n";

  ofs.close( );
}

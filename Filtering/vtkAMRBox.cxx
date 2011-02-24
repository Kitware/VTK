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
#include "vtkAssertUtils.hpp"

#include <vtkstd/vector>
#include <vtkstd/algorithm>
#include <cstring>
#include <cassert>
#include <sstream>
#include <fstream>

using vtkstd::vector;
using vtkstd::copy;


//-----------------------------------------------------------------------------
void vtkAMRBox::Invalidate()
{
  this->LoCorner[0]=this->LoCorner[1]=this->LoCorner[2]=0;
  this->HiCorner[0]=this->HiCorner[1]=this->HiCorner[2]=-1;
}

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
  this->Initialize( );
  this->SetDimensionality(2);
  this->SetDimensions(ilo,jlo,0,ihi,jhi,0);
  this->X0[0]=this->X0[1]=this->X0[2]=0.0;
  this->DX[0]=this->DX[1]=this->DX[2]=1.0;
  this->RealExtent[0] = ilo;
  this->RealExtent[1] = ihi;
  this->RealExtent[2] = jlo;
  this->RealExtent[3] = jhi;
  this->RealExtent[4] = 0;
  this->RealExtent[5] = 0;
}

//-----------------------------------------------------------------------------
vtkAMRBox::vtkAMRBox(
    int ilo,int jlo,int klo,
    int ihi,int jhi,int khi)
{
  this->Initialize();
  this->SetDimensionality(3);
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
vtkAMRBox::vtkAMRBox(const int *lo, const int *hi)
{
  this->Initialize();
  this->SetDimensionality(3);
  this->SetDimensions(lo,hi);
  this->X0[0]=this->X0[1]=this->X0[2]=0.0;
  this->DX[0]=this->DX[1]=this->DX[2]=1.0;
  this->RealExtent[0] = lo[0];
  this->RealExtent[1] = hi[0];
  this->RealExtent[2] = lo[1];
  this->RealExtent[3] = hi[1];
  this->RealExtent[4] = lo[2];
  this->RealExtent[5] = hi[2];
}

//-----------------------------------------------------------------------------
vtkAMRBox::vtkAMRBox(int dim, const int *lo, const int *hi)
{
  this->Initialize();
  this->SetDimensionality(dim);
  this->SetDimensions(lo,hi);
  this->X0[0]=this->X0[1]=this->X0[2]=0.0;
  this->DX[0]=this->DX[1]=this->DX[2]=1.0;
}

//-----------------------------------------------------------------------------
vtkAMRBox::vtkAMRBox(const int *dims)
{
  this->SetDimensionality(3);
  this->Initialize( );
  this->SetDimensions(dims);
  this->X0[0]=this->X0[1]=this->X0[2]=0.0;
  this->DX[0]=this->DX[1]=this->DX[2]=1.0;
}

//-----------------------------------------------------------------------------
vtkAMRBox::vtkAMRBox(int dim, const int *dims)
{
  this->Initialize();
  this->SetDimensionality(dim);
  this->SetDimensions(dims);
  this->X0[0]=this->X0[1]=this->X0[2]=0.0;
  this->DX[0]=this->DX[1]=this->DX[2]=1.0;
}

//-----------------------------------------------------------------------------
vtkAMRBox::vtkAMRBox(
      const double origin[3], const int dim, const int ndim[3],
      const double h[3], const int blockIdx, const int level,
      const int rank )
{
  // Sanity Check!
  vtkAssertUtils::assertInRange( dim, 2, 3, __FILE__, __LINE__ );
  vtkAssertUtils::assertTrue( (level>=0), __FILE__, __LINE__ );

  this->Dimension = dim;

  // low corner ijk is @ (0,0,0)
  this->LoCorner[ 0 ] = this->LoCorner[ 1 ] = this->LoCorner[ 2 ] = 0;

  for( int i=0; i < 3; ++i )
    {
      vtkAssertUtils::assertTrue( ndim[i]>=1, __FILE__, __LINE__ );
      this->X0[i]       = origin[i];
      this->DX[i]       = h[i];
      this->HiCorner[i] = ndim[i]-1; // we are indexing from 0!
      vtkAssertUtils::assertTrue( this->HiCorner[i]>=0, __FILE__, __LINE__ );
    }
  this->BlockId    = blockIdx;
  this->BlockLevel = level;
  this->ProcessId  = rank;
  for( int i=0; i < 6; ++i )
    {
      this->NG[ i ] = 0;
    }
  this->RealExtent[0] = this->LoCorner[0];
  this->RealExtent[1] = this->HiCorner[0];
  this->RealExtent[2] = this->LoCorner[1];
  this->RealExtent[3] = this->HiCorner[1];
  this->RealExtent[4] = this->LoCorner[2];
  this->RealExtent[5] = this->HiCorner[2];
}

//-----------------------------------------------------------------------------
vtkAMRBox::vtkAMRBox(
    const double origin[3], const int dimension,
    const int lo[3], const int hi[3],
    const double h[3],
    const int blockIdx,const int level,
    const int rank )
{
  // Sanity check!
  vtkAssertUtils::assertInRange( dimension, 2, 3, __FILE__, __LINE__ );
  vtkAssertUtils::assertTrue( (level>=0), __FILE__,__LINE__ );
  vtkAssertUtils::assertTrue( (hi[0]>=lo[0]),__FILE__,__LINE__ );
  vtkAssertUtils::assertTrue( (hi[1]>=lo[1]),__FILE__,__LINE__ );
  vtkAssertUtils::assertTrue( (hi[2]>=lo[2]),__FILE__,__LINE__ );
  vtkAssertUtils::assertTrue( (hi[0]>=0),__FILE__,__LINE__);
  vtkAssertUtils::assertTrue( (lo[0]>=0),__FILE__,__LINE__);
  vtkAssertUtils::assertTrue( (hi[1]>=0),__FILE__,__LINE__);
  vtkAssertUtils::assertTrue( (lo[1]>=0),__FILE__,__LINE__);
  vtkAssertUtils::assertTrue( (hi[2]>=0),__FILE__,__LINE__);
  vtkAssertUtils::assertTrue( (lo[2]>=0),__FILE__,__LINE__);

  this->Initialize();
  this->Dimension = dimension;

  this->LoCorner[0] = lo[0];
  this->LoCorner[1] = lo[1];
  this->LoCorner[2] = lo[2];

  this->HiCorner[0] = hi[0];
  this->HiCorner[1] = hi[1];
  this->HiCorner[2] = hi[2];

  for( int i=0; i < this->Dimension; ++i )
    {
      this->X0[ i ] = origin[ i ];
      this->DX[ i ] = h[ i ];
      vtkAssertUtils::assertTrue( this->HiCorner[i]>=0, __FILE__, __LINE__ );
    }
   this->BlockId    = blockIdx;
   this->BlockLevel = level;
   this->ProcessId  = rank;
   for( int i=0; i < 6; ++i )
     {
       this->NG[ i ] = 0;
     }
   this->RealExtent[0] = this->LoCorner[0];
   this->RealExtent[1] = this->HiCorner[0];
   this->RealExtent[2] = this->LoCorner[1];
   this->RealExtent[3] = this->HiCorner[1];
   this->RealExtent[4] = this->LoCorner[2];
   this->RealExtent[5] = this->HiCorner[2];
}

//-----------------------------------------------------------------------------
vtkAMRBox::vtkAMRBox(
    const double dataSetOrigin[3], const double boxOrigin[3],
    const int dimension,
    const double h[3],
    const int ndim[3],
    const int blockIdx,
    const int level,
    const int rank )
{
  this->Initialize();

  this->Dimension = dimension;
  for( int i=0; i < this->Dimension; ++i )
    {
      this->X0[i]       = dataSetOrigin[i];
      this->DX[i]       = h[i];
      this->LoCorner[i] = round( (boxOrigin[i]-dataSetOrigin[i])/h[i] );
      this->HiCorner[i] = this->LoCorner[i] + ( ndim[i]-1 );
      vtkAssertUtils::assertTrue( (this->LoCorner[i]>=0),__FILE__,__LINE__);
      vtkAssertUtils::assertTrue( (this->HiCorner[i]>=0),__FILE__,__LINE__);
    }

  this->BlockId    = blockIdx;
  this->BlockLevel = level;
  this->ProcessId  = rank;
  for( int i=0; i < 6; ++i )
    {
      this->NG[ i ] = 0;
    }

  this->RealExtent[0] = this->LoCorner[0];
  this->RealExtent[1] = this->HiCorner[0];
  this->RealExtent[2] = this->LoCorner[1];
  this->RealExtent[3] = this->HiCorner[1];
  this->RealExtent[4] = this->LoCorner[2];
  this->RealExtent[5] = this->HiCorner[2];
}

//-----------------------------------------------------------------------------
int vtkAMRBox::GetNodeLinearIndex( const int i,const int j,const int k )
{
  // Sanity Check!
  vtkAssertUtils::assertInRange(
   i,this->LoCorner[0],this->HiCorner[0],__FILE__, __LINE__ );
  vtkAssertUtils::assertInRange(
   j,this->LoCorner[1],this->HiCorner[1],__FILE__, __LINE__ );
  vtkAssertUtils::assertInRange(
   k,this->LoCorner[2],this->HiCorner[2],__FILE__, __LINE__ );

  int ndim[3];
  int ijk[3];
  ijk[0]=i; ijk[1]=j; ijk[2]=k;
  this->GetNumberOfNodes( ndim );
  int idx = vtkStructuredData::ComputePointId( ndim, ijk );
  return( idx );
}

//-----------------------------------------------------------------------------
int vtkAMRBox::GetCellLinearIndex( const int i, const int j, const int k )
{
  // Sanity Check!
  vtkAssertUtils::assertInRange(
   i,this->LoCorner[0],this->HiCorner[0]-1,__FILE__, __LINE__ );
  vtkAssertUtils::assertInRange(
   j,this->LoCorner[1],this->HiCorner[1]-1,__FILE__, __LINE__ );
  vtkAssertUtils::assertInRange(
   k,this->LoCorner[2],this->HiCorner[2]-1,__FILE__, __LINE__ );

  int ndim[3];
  int ijk[3];
  ijk[0]=i; ijk[1]=j; ijk[2]=k;
  this->GetNumberOfNodes( ndim );
  int idx = vtkStructuredData::ComputeCellId( ndim, ijk );
  return( idx );
}

//-----------------------------------------------------------------------------
void vtkAMRBox::WriteToVtkFile( const char *file )
{
  // Sanity Checks
  vtkAssertUtils::assertNotNull( file, __FILE__, __LINE__ );
//  vtkAssertUtils::assertTrue( std::strlen( file )>0, __FILE__, __LINE__ );

  std::ofstream ofs;
  ofs.open( file );
  vtkAssertUtils::assertTrue( ofs.is_open(), __FILE__, __LINE__ );

  ofs << "# vtk DataFile Version 3.0\n";
  ofs << "Grid:" << this->BlockId << " Level:" << this->BlockLevel << "\n";
  ofs << "ASCII\n";
  ofs << "DATASET STRUCTURED_GRID\n";

  int nodeExtent[3];
  this->GetNumberOfNodes( nodeExtent );
  ofs << "DIMENSIONS " << nodeExtent[0] << " " << nodeExtent[1];
  ofs << " " << nodeExtent[2 ] << std::endl;
  ofs << "POINTS " << this->GetNumberOfNodes() << " double\n";


  int *isghost = new int[ this->GetNumberOfNodes() ];
  vtkAssertUtils::assertNotNull( isghost, __FILE__, __LINE__ );

  int    ijk[3];
  double pnt[3];
  pnt[0] = pnt[1] = pnt[2] = 0;
  for( int k=this->LoCorner[2]; k <= this->HiCorner[2]; ++k )
    {
      for( int j=this->LoCorner[1]; j <= this->HiCorner[1]; ++j )
        {
          for( int i=this->LoCorner[0]; i <= this->HiCorner[0]; ++i )
            {

              ijk[0] = i;
              ijk[1] = j;
              ijk[2] = k;
              int pntIdx = this->GetNodeLinearIndex( i,j,k );
              this->GetPoint( ijk, pnt );
              ofs << pnt[ 0 ] << " " << pnt[ 1 ] << " " << pnt[ 2 ] << "\n";

              if( this->IsGhostNode( i,j,k ) )
                isghost[ pntIdx ] = 0;
              else
                isghost[ pntIdx ] = 1;

            } // END for all k
        } // END for all j
    } // END for all i

  // Attach Ghost status as point data
  ofs << "POINT_DATA " << this->GetNumberOfNodes() << std::endl;
  ofs << "SCALARS GHOSTFLAG int 1\n";
  ofs << "LOOKUP_TABLE default\n";
  for( int i=0; i < this->GetNumberOfNodes(); ++i )
   ofs << isghost[ i ] << std::endl;

  delete[] isghost;
  isghost = NULL;

  ofs.close( );
}

//-----------------------------------------------------------------------------
vtkAMRBox::vtkAMRBox(const vtkAMRBox &other)
{
  *this = other;
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
  this->SetDataSetOrigin( other.GetDataSetOrigin() );

  this->ProcessId  = other.ProcessId;
  this->BlockId    = other.BlockId;
  this->BlockLevel = other.BlockLevel;
  for( int i=0; i < 6; ++i )
    {
      this->RealExtent[i] = other.RealExtent[i];
      this->NG[i]         = other.NG[i];
    }

  return *this;
}

//-----------------------------------------------------------------------------
void vtkAMRBox::Initialize( )
{
  this->Dimension  = 3;
  this->ProcessId  = 0;
  this->BlockId    = 0;
  this->BlockLevel = 0;
  this->X0[0] = this->X0[1] = this->X0[2] = 0.0;
  this->DX[0] = this->DX[1] = this->DX[2] = 0.0;
  for( int i=0; i < 6; ++i )
    {
      this->NG[i]         = 0;
      this->RealExtent[i] = 0;
    }
  this->Invalidate();
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
void vtkAMRBox::SetProcessId( const int pid )
{
  vtkAssertUtils::assertTrue( (pid>=0), __FILE__, __LINE__ );
  this->ProcessId = pid;
}

//-----------------------------------------------------------------------------
void vtkAMRBox::SetBlockId( const int blockIdx )
{
  vtkAssertUtils::assertTrue( (blockIdx>=0), __FILE__, __LINE__ );
  this->BlockId = blockIdx;
}

//-----------------------------------------------------------------------------
void vtkAMRBox::SetLevel( const int level )
{
  vtkAssertUtils::assertTrue( (level>=0), __FILE__, __LINE__ );
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
  vtkAssertUtils::assertNotNull( x0,__FILE__,__LINE__);
  x0[0] = x0[1] = x0[2] = 0.0;

  for( int i=0; i < this->Dimension; ++i )
    {
      x0[i] = this->X0[i]+this->LoCorner[i]*this->DX[i];
    }
}

//-----------------------------------------------------------------------------
void vtkAMRBox::GetNumberOfCells(int *ext) const
{
  // Sanity!
 vtkAssertUtils::assertTrue(
  this->HiCorner[0]>=this->LoCorner[0],__FILE__,__LINE__ );
 vtkAssertUtils::assertTrue(
  this->HiCorner[1]>=this->LoCorner[1],__FILE__,__LINE__ );
 vtkAssertUtils::assertTrue(
  this->HiCorner[2]>=this->LoCorner[2],__FILE__,__LINE__ );

  ext[0]=this->HiCorner[0]-this->LoCorner[0];
  ext[1]=this->HiCorner[1]-this->LoCorner[1];
  ext[2]=this->HiCorner[2]-this->LoCorner[2];
// TODO: ?
//  if (this->Empty())
//    {
//    ext[0]=ext[1]=0;
//    if (this->Dimension>2){ ext[2]=0; }
//    return;
//    }
//
//  ext[2]=1;
//  for (int q=0; q<this->Dimension; ++q)
//    {
//    ext[q]=this->HiCorner[q]-this->LoCorner[q]+1;
//    }

}

//-----------------------------------------------------------------------------
vtkIdType vtkAMRBox::GetNumberOfCells() const
{
  int cellExtent[3];
  this->GetNumberOfCells( cellExtent );
  int numCells = 0;
  switch( this->Dimension )
    {
    case 1:
      numCells = cellExtent[0];
      break;
    case 2:
      numCells = cellExtent[0]*cellExtent[1];
      break;
    case 3:
      numCells = cellExtent[0]*cellExtent[1]*cellExtent[2];
      break;
    default:
      numCells = 0;
      // Code should not reach here
//      vtkWarningMacro( "Encountered a box with dimension > 3!" );
    }
  return( numCells );
// TODO: ?
//  if (this->Empty())
//    {
//    return 0;
//    }
//
//  vtkIdType nCells=1;
//  for (int q=0; q<this->Dimension; ++q)
//    {
//    nCells*=this->HiCorner[q]-this->LoCorner[q]+1;
//    }
//  return nCells;
}

//-----------------------------------------------------------------------------
void vtkAMRBox::GetNumberOfNodes(int *ext) const
{
  // Sanity!
  vtkAssertUtils::assertTrue(
   this->HiCorner[0]>=this->LoCorner[0], __FILE__, __LINE__ );
  vtkAssertUtils::assertTrue(
   this->HiCorner[1]>=this->LoCorner[1], __FILE__, __LINE__ );
  vtkAssertUtils::assertTrue(
   this->HiCorner[2]>=this->LoCorner[2], __FILE__, __LINE__ );

  ext[0] = (this->HiCorner[0]-this->LoCorner[0])+1;
  ext[1] = (this->HiCorner[1]-this->LoCorner[1])+1;
  ext[2] = (this->HiCorner[2]-this->LoCorner[2])+1;

// TODO: ?
//  if (this->Empty())
//    {
//    ext[0]=ext[1]=0;
//    if (this->Dimension>2){ ext[2]=0; }
//    return;
//    }
//
//  ext[2]=1;
//  for (int q=0; q<this->Dimension; ++q)
//    {
//    ext[q]=this->HiCorner[q]-this->LoCorner[q]+2;
//    }
}

//-----------------------------------------------------------------------------
vtkIdType vtkAMRBox::GetNumberOfNodes() const
{
  int ext[3];
  this->GetNumberOfNodes( ext );
  int numNodes = 0;
  switch( this->Dimension )
    {
    case 1:
      numNodes = ext[0];
      break;
    case 2:
      numNodes = ext[0]*ext[1];
      break;
    case 3:
      numNodes = ext[0]*ext[1]*ext[2];
      break;
    default:
      numNodes = 0;
      // Code should not reach here
//      vtkWarningMacro( "Encountered a box with dimension > 3!" );
    }
  return( numNodes );
// TODO: ?
//  if (this->Empty())
//    {
//    return 0;
//    }
//
//  vtkIdType nPoints=1;
//  for (int q=0; q<this->Dimension; ++q)
//    {
//    nPoints*=this->HiCorner[q]-this->LoCorner[q]+2;
//    }
//  return nPoints;
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

  // TODO: There is no bound checking here!
  // ijk's can be negative & grids can grow
  // outside the domain?
  for (int q=0; q<this->Dimension; ++q)
    {
     lo[q]=this->LoCorner[q]-byN;
     hi[q]=this->HiCorner[q]+byN;
     this->NG[q*2]++;
     this->NG[q*2+1]++;
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

  // TODO: There is no bound checking here!
  // ijk's can be negative, grids can shrink
  // to a plane(3-D) or point (2-D) or worse
  // get inverted
  for (int q=0; q<this->Dimension; ++q)
    {
    lo[q]=this->LoCorner[q]+byN;
    hi[q]=this->HiCorner[q]-byN;
    if( this->NG[q] > 0)
      this->NG[q*2]--;
    if( this->NG[q*2+1] > 0)
      this->NG[q*2+1]--;
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
  // TODO: There is no bound checking here!
  // ijk's can be negative,
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
  // TODO: There is no bound checking here!
  // ijk's can be negative,
  this->SetDimensions(
    this->LoCorner[0]+i, this->LoCorner[1]+j,this->LoCorner[2]+k,
    this->HiCorner[0]+i, this->HiCorner[1]+j,this->HiCorner[2]+k);
}

//-----------------------------------------------------------------------------
void vtkAMRBox::Shift(const int *I)
{
  // TODO: There is no bound checking here!
  // ijk's can be negative,
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
double vtkAMRBox::GetMinX() const
{
  vtkAssertUtils::assertTrue( this->Dimension>=1, __FILE__, __LINE__ );

  double pnt[3];
  this->GetBoxOrigin(pnt);

  return( pnt[0] );
}

//-----------------------------------------------------------------------------
double vtkAMRBox::GetMinY() const
{
  vtkAssertUtils::assertTrue( this->Dimension>=1, __FILE__, __LINE__ );

  double pnt[3];
  this->GetBoxOrigin(pnt);

  if( this->Dimension >= 2 )
      return( pnt[1] );
  return( 0 );
}

//-----------------------------------------------------------------------------
double vtkAMRBox::GetMinZ() const
{
  vtkAssertUtils::assertTrue( this->Dimension>=1, __FILE__, __LINE__ );

  double pnt[3];
  this->GetBoxOrigin(pnt);

  if( this->Dimension >= 3 )
    return( pnt[2] );
  return( 0 );
}

//-----------------------------------------------------------------------------
double vtkAMRBox::GetMaxX() const
{
  vtkAssertUtils::assertTrue( this->Dimension>=1, __FILE__, __LINE__ );

  double pnt[3];
  this->GetPoint( this->HiCorner, pnt );
  return( pnt[0] );
}

//-----------------------------------------------------------------------------
double vtkAMRBox::GetMaxY() const
{
  vtkAssertUtils::assertTrue( this->Dimension>=1, __FILE__, __LINE__ );

  double pnt[3];
  this->GetPoint( this->HiCorner, pnt );

  if( this->Dimension >= 2 )
    return( pnt[1] );
  return( 0 );
}

//-----------------------------------------------------------------------------
double vtkAMRBox::GetMaxZ() const
{
  vtkAssertUtils::assertTrue( this->Dimension>=1, __FILE__, __LINE__ );

  double pnt[3];
  this->GetPoint( this->HiCorner, pnt );

  if( this->Dimension == 3 )
   return( pnt[2] );
  return( 0 );
}

//-----------------------------------------------------------------------------
void vtkAMRBox::GetMinBounds( double min[3] ) const
{
  min[0] = min[1] = min[2] = 0.0;
  for( int i=0; i < this->Dimension; ++i )
    min[ i ] = this->X0[i]+this->LoCorner[i]*this->DX[i];
}

//-----------------------------------------------------------------------------
void vtkAMRBox::GetMaxBounds( double max[3] ) const
{
  max[0] = max[1] = max[2] = 0.0;
  for( int i=0; i < this->Dimension; ++i )
    max[ i ] =this->X0[i]+this->HiCorner[i]*this->DX[i];
}

//-----------------------------------------------------------------------------
bool vtkAMRBox::HasPoint( const double x, const double y, const double z )
{

  double min[3];
  double max[3];
  this->GetMinBounds( min );
  this->GetMaxBounds( max );

  switch( this->Dimension )
    {
      case 1:
        if( x >= min[0] && x <= max[0]  )
            return true;
        break;
      case 2:
        if( x >= min[0] && x <= max[0] &&
            y >= min[1] && y <= max[1]     )
            return true;
        break;
      case 3:
        if( x >= min[0] && x <= max[0] &&
            y >= min[1] && y <= max[1] &&
            z >= min[2] && z <= max[2]   )
            return true;
        break;
      default:
        // Code should not reach here!
        // TODO: add better error handling here!
        return false;
    }
  return false;

}

//-----------------------------------------------------------------------------
bool vtkAMRBox::operator==(const vtkAMRBox &other)
{
  // TODO: fix this to check for equality of meta-data as well
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
//void vtkAMRBox::operator&=(const vtkAMRBox &other)
//{
//  if (this->Dimension!=other.Dimension)
//    {
//    vtkGenericWarningMacro(
//      "Can't operate on a " << this->Dimension
//      << "D box with a " << other.Dimension << "D box.");
//    return;
//    }
//  if (this->Empty())
//    {
//    return;
//    }
//  if (other.Empty())
//    {
//    this->Invalidate();
//    return;
//    }
//
//  int otherLo[3];
//  int otherHi[3];
//  other.GetDimensions(otherLo,otherHi);
//  int lo[3];
//  int hi[3];
//  for (int q=0; q<this->Dimension; ++q)
//    {
//    lo[q]=(this->LoCorner[q]>otherLo[q] ? this->LoCorner[q] : otherLo[q]);
//    hi[q]=(this->HiCorner[q]<otherHi[q] ? this->HiCorner[q] : otherHi[q]);
//    }
//  this->SetDimensions(lo,hi);
//}

//-----------------------------------------------------------------------------
bool vtkAMRBox::Contains(int i,int j,int k) const
{
  vtkAssertUtils::assertFalse( this->Empty(), __FILE__, __LINE__ );
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
  vtkAssertUtils::assertFalse( this->Empty(),__FILE__,__LINE__);
  if (this->Empty())
    {
    return;
    }

//  std::cout << "=============================\n";
//  std::cout << "Before coarsening:\n";
//  this->Print( std::cout );
//  std::cout << std::endl;
//  std::cout.flush( );

  // sanity check.
//  int nCells[3];
//  this->GetNumberOfCells(nCells);
//  for (int q=0; q<this->Dimension; ++q)
//    {
//     if (nCells[q]%r)
//      {
//      vtkGenericWarningMacro( << "nCells[q]: " << nCells[q] <<
//        " r:" << r << " nCells[q]%r: " << nCells[q]%r );
//      vtkGenericWarningMacro("This box cannot be coarsened.");
//      return;
//      }
//    }

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

//  std::cout << "After coarsening:\n";
//  this->Print( std::cout );
//  std::cout << std::endl;
//  std::cout << "=============================\n";
//  std::cout.flush( );

}

//-----------------------------------------------------------------------------
ostream &vtkAMRBox::Print(ostream &os) const
{
  os << "Low: ("  << this->LoCorner[0]
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
  // Sanity Checks
  vtkAssertUtils::assertInRange(
   realExtent[0],this->LoCorner[0],this->HiCorner[0],__FILE__,__LINE__);
  vtkAssertUtils::assertInRange(
   realExtent[1],this->LoCorner[0],this->HiCorner[0],__FILE__,__LINE__);
  vtkAssertUtils::assertInRange(
   realExtent[2],this->LoCorner[1],this->HiCorner[1],__FILE__,__LINE__);
  vtkAssertUtils::assertInRange(
   realExtent[3],this->LoCorner[1],this->HiCorner[1],__FILE__,__LINE__);

 if( this->Dimension  == 3 )
   {
    vtkAssertUtils::assertInRange(
     realExtent[4],this->LoCorner[2],this->HiCorner[2],__FILE__,__LINE__);
    vtkAssertUtils::assertInRange(
     realExtent[5],this->LoCorner[2],this->HiCorner[2],__FILE__,__LINE__);
   }

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
  // Sanity Checks
  // Check i-range
  vtkAssertUtils::assertInRange(
    min[0],this->LoCorner[0],this->HiCorner[0],__FILE__,__LINE__);
  vtkAssertUtils::assertInRange(
    max[0],this->LoCorner[0],this->HiCorner[0],__FILE__,__LINE__);

  // Check j-range
  vtkAssertUtils::assertInRange(
    min[1],this->LoCorner[1],this->HiCorner[1],__FILE__,__LINE__);
  vtkAssertUtils::assertInRange(
    max[1],this->LoCorner[1],this->HiCorner[1],__FILE__,__LINE__);

  // Check k-range
  vtkAssertUtils::assertInRange(
    min[2],this->LoCorner[2],this->HiCorner[2],__FILE__,__LINE__);
  vtkAssertUtils::assertInRange(
    max[2],this->LoCorner[2],this->HiCorner[2],__FILE__,__LINE__);

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
        status = true;
      break;
    case 2:
      if( (i < this->RealExtent[0]) || (i > this->RealExtent[1]) ||
          (j < this->RealExtent[2]) || (j > this->RealExtent[3])    )
        status = true;
      break;
    case 3:
      if( (i < this->RealExtent[0]) || (i > this->RealExtent[1]) ||
          (j < this->RealExtent[2]) || (j > this->RealExtent[3]) ||
          (k < this->RealExtent[4]) || (k > this->RealExtent[5])    )
        status = true;
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
  for( int i=0; i < this->Dimension; ++i )
    {
      // Sanity Check!
      vtkAssertUtils::assertInRange(
       ijk[i],this->LoCorner[i],this->HiCorner[i], __FILE__, __LINE__ );

      if( ijk[i] == 0 )
        pnt[i] = this->X0[i];
      else
        pnt[i] = this->X0[i]+ijk[i]*this->DX[i];
    }

}

//------------------------------------------------------------------------------
void vtkAMRBox::GetPoint(
    const int i, const int j, const int k, double pnt[3] ) const
{
  int ijk[3] = {i,j,k};
  this->GetPoint( ijk, pnt );
}

//-----------------------------------------------------------------------------
void vtkAMRBox::Serialize( unsigned char*& buffer, size_t& vtkNotUsed(bytesize))
{

  vtkAssertUtils::assertNull( buffer, __FILE__, __LINE__ );

  size_t bufsize = vtkAMRBox::GetBytesize();
  buffer         = new unsigned char[ bufsize ];
  vtkAssertUtils::assertNotNull( buffer, __FILE__, __LINE__ );

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
}

//-----------------------------------------------------------------------------
void vtkAMRBox::Deserialize( unsigned char* buffer, const size_t &bytesize )
{

  vtkAssertUtils::assertNotNull( buffer, __FILE__, __LINE__ );
  vtkAssertUtils::assertTrue( (bytesize > 0), __FILE__, __LINE__ );

  // STEP 0: set pointer to traverse the buffer
  unsigned char *ptr = buffer;

  // STEP 1: de-serialize the dimension
  std::memcpy( &(this->Dimension), ptr, sizeof(int) );
  ptr += sizeof( int );
  vtkAssertUtils::assertNotNull( ptr, __FILE__, __LINE__ );

  // STEP 2: de-serialize the coordinates
  std::memcpy( &(this->X0), ptr, 3*sizeof(double) );
  ptr += 3*sizeof(double);
  vtkAssertUtils::assertNotNull( ptr, __FILE__, __LINE__ );

  // STEP 3: de-serialize the spacing array
  std::memcpy( &(this->DX), ptr, 3*sizeof(double) );
  ptr += 3*sizeof(double);
  vtkAssertUtils::assertNotNull( ptr, __FILE__, __LINE__ );

  // STEP 4: de-serialize the block ID
  std::memcpy( &(this->BlockId), ptr, sizeof(int) );
  ptr += sizeof( int );
  vtkAssertUtils::assertNotNull( ptr, __FILE__, __LINE__ );

  // STEP 5: de-serialize the process ID
  std::memcpy( &(this->ProcessId), ptr, sizeof(int) );
  ptr += sizeof( int );
  vtkAssertUtils::assertNotNull( ptr, __FILE__, __LINE__ );

  // STEP 6: serialize the block level
  std::memcpy(&(this->BlockLevel), ptr, sizeof(int) );
  ptr += sizeof( int );
  vtkAssertUtils::assertNotNull( ptr, __FILE__, __LINE__ );

  // STEP 7: serialize the low corner
  std::memcpy( &(this->LoCorner), ptr, 3*sizeof(int) );
  ptr += 3*sizeof( int );
  vtkAssertUtils::assertNotNull( ptr, __FILE__, __LINE__ );

  // STEP 8: serialize the high corner
  std::memcpy(&(this->HiCorner), ptr,  3*sizeof(int) );
  ptr += 3*sizeof( int );
}

//-----------------------------------------------------------------------------
bool vtkAMRBox::Collides( const vtkAMRBox &b1, const vtkAMRBox &b2)
{
  double min1[3];
  double max1[3];
  double min2[3];
  double max2[3];

  b1.GetMinBounds( min1 );
  b1.GetMaxBounds( max1 );
  b2.GetMinBounds( min2 );
  b2.GetMaxBounds( max2 );

  for( int i=0; i < 3; ++i )
    {

      if( (min1[i] >= min2[i]) && (min1[i] <= max2[i]) )
       continue;
      if( (min2[i] >= min1[i]) && (min2[i] <= max1[i]) )
        continue;
      if( (max1[i] >= min2[i]) && (max1[i] <= max2[i]) )
        continue;
      if( (max2[i] >= min1[i]) && (max2[i] <= max1[i]) )
        continue;

      return false;
    } // End for all directions

  return true;

}















//*****************************************************************************
void Split(
      const int N[3],
      const int minSide[3],
      vtkstd::vector<vtkAMRBox> &decomp)
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
      vtkstd::vector<vtkAMRBox> &decomp)
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
  vtkAssertUtils::assertTrue( ofs.is_open(), __FILE__, __LINE__ );

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

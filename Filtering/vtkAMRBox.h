/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAMRBox.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAMRBox - Encloses a rectangular region of voxel like cells.
//
// .SECTION Description
// vtkAMRBox represents a collection of voxel like cells that fill
// a rectangular region. It's purpose is to facilitate manipulation
// of cell data arrays and the blocks that they are defined on.
//
// The vtkAMRBox can be either 2D or 3D. For 2D grids, pointers
// passed in are assumed to be 2 units. The default is 3D.
//
// vtkAMRBox is used in vtkHierarchicalBoxDataSet to compute cell visibilty.
//
// .SECTION See Also
// vtkHierarachicalBoxDataSet, vtkAMRBoxUtilities.hxx

#ifndef __vtkAMRBox_h
#define __vtkAMRBox_h

#include "vtkObject.h"
#include "vtkSerializable.h"
#include "vtkType.h" //For utility functions.
#include <vtkstd/vector> // STL Header

class VTK_FILTERING_EXPORT vtkAMRBox : public vtkSerializable
{

public:

  // Description:
  // Construct the empty box.
  vtkAMRBox(int dim=3);

  // Description:
  // Construct a specific 3D box.
  vtkAMRBox(
      int ilo,int jlo,int klo,
      int ihi,int jhi,int khi);

  // Description:
  // Construct a specific 2D box.
  vtkAMRBox(
      int ilo,int jlo,
      int ihi,int jhi);

  // Description:
  // Construct a specific box. (ilo,jlo,klo,)(ihi,jhi,khi)
  vtkAMRBox(int dim, const int lo[3], const int hi[3]);
  vtkAMRBox(const int lo[3], const int hi[3]);

  // Description:
  // Construct a specific box. (ilo,ihi, jlo,jhi, klo,khi)
  vtkAMRBox(int dim, const int dims[6]);
  vtkAMRBox(const int dims[6]);

  // Description:
  // Construct a specific box & set all of its meta-data
  // origin   -- 3-tuple consisting of the x,y,z world origin of the Dataset
  // dim      -- The dimension of the corresponding block, i.e., 2 or 3
  // ndim     -- 3-tuple of the number of points along each dimension
  //             Note, this constructor assumes loCorner to be 0
  // h        -- 3-tuple consisting of the mesh spacing at each dimension
  // blockIdx -- the ID of the block corresponding to this instance
  // level    -- the level of this instance
  // rank     -- the corresponding process rank that owns this block
  //
  // .SECTION: Note the origin is the origin of the entire data-set and not
  // the origin of the box. To get the origin of the box use
  vtkAMRBox(
      const double origin[3], const int dim, const int ndim[3],
      const double h[3], const int blockIdx, const int level,
      const int rank );


   // Description:
   // Construct a specific box & set all of its meta-data
   // origin   -- 3-tuple consisting of the x,y,z world origin of the Dataset
   // dim      -- The dimension of the corresponding block, i.e., 2 or 3
   // lo       -- 3-tuple of the lowest corner of the box
   // hi       -- 3-tuple of the highest corner of the box
   //             Note, this constructor assumes loCorner to be 0
   // h        -- 3-tuple consisting of the mesh spacing at each dimension
   // blockIdx -- the ID of the block corresponding to this instance
   // level    -- the level of this instance
   // rank     -- the corresponding process rank that owns this block
   //
   // .SECTION: Note the origin is the origin of the entire data-set and not
   // the origin of the box. To get the origin of the box use
  vtkAMRBox(
      const double origin[3], const int dimension,
      const int lo[3], const int hi[3],
      const double h[3], const int blockIdx, const int level,
      const int rank );


  // Description:
  // Consturcts a specific box with given dataSet origin & box origin.
  // dataSetOrigin -- x,y,z world origin of the entire dataset.
  // boxOrigin     -- x,y,z box origin.
  // dimension     -- the dimension of the corresponding block, i.e., 2 or 3.
  // h             -- mesh spacing along each direction.
  // ndims         -- Number of points along each dimension
  // blockIdx      -- the ID of the block corresponding to this instance.
  // level         -- the level of this instance.
  // rank          -- the corresponding process rank that owns this block.
  vtkAMRBox(
      const double dataSetOrigin[3], const double boxOrigin[3],
      const int dimension,
      const double h[3],
      const int   ndim[3],
      const int blockIdx,
      const int level,
      const int rank );

  // Description:
  // Copy construct this box from another.
  vtkAMRBox(const vtkAMRBox &other);

  // Description:
  // Copy the other box to this box.
  vtkAMRBox &operator=(const vtkAMRBox &other);

  // Description:
  // Determines if two AMR boxes collide.
  static bool Collides( const vtkAMRBox &b1, const vtkAMRBox &b2 );

  // Description:
  // Set the box to null;
  void Invalidate();

  // Description:
  // Returns the number of ghost layes that have been extruded along
  // each dimension.
  void GetNumberOfGhosts( int *ng );

  // Description:
  // Get the minimum coordinates of this box instance.
  double GetMinX() const;
  double GetMinY() const;
  double GetMinZ() const;
  void GetMinBounds( double min[3] ) const;

  // Description:
  // Get the maximum coordinates of this box instance.
  double GetMaxX() const;
  double GetMaxY() const;
  double GetMaxZ() const;
  void GetMaxBounds( double max[3] ) const;

  // Description:
  // Get/Set the spatial dimension of the box. Only 2 and 3 
  // are valid.
  int GetDimensionality() const { return this->Dimension; }
  void SetDimensionality(int dim);

  // Description:
  // Get/Set the process ID of the process that owns the box.
  int GetProcessId() const { return this->ProcessId; }
  void SetProcessId( const int pid );

  // Description:
  // Get/Set the Block ID of the AMR grid corresponding to this AMR box.
  int GetBlockId() const { return this->BlockId; }
  void SetBlockId( const int blockIdx );

  // Description:
  // Get/Set the Block level of the AMR grid corresponding to this AMR box.
  int GetLevel() const { return this->BlockLevel; }
  void SetLevel( const int level );

  // Description:
  // Returns the real extent of this AMR Box instance.
  void GetRealExtent( int realExtent[6] ) const;

  // Description:
  // Sets the real extent of this AMR box instance.
  void SetRealExtent( int realExtent[6] );
  void SetRealExtent( int minijk[3], int maxijk[3] );

  // Description:
  // Checks if the point is inside this AMRBox instance.
  // x,y,z the world point
  bool HasPoint( const double x, const double y, const double z );

  // Description:
  // Set the dimensions of the box. ilo,jlo,klo,ihi,jhi,khi
  void SetDimensions(
      int ilo, int jlo, int klo,
      int ihi, int jhi, int khi);

  // Description:
  // Set the dimensions of the box. (ilo,jlo,klo),(ihi,jhi,khi)
  void SetDimensions(const int lo[3], const int hi[3]);

  // Description:
  // Set the dimensions of the box. (ilo,ihi,jlo,jhi,klo,khi)
  void SetDimensions(const int dims[6]);

  // Description:
  // Get the dimensions of this box. (ilo,jlo,jhi),(ihi,jhi,khi)
  void GetDimensions(int lo[3], int hi[3]) const;

  // Description:
  // Get the dimensions of this box. (ilo,ihi, jlo,jhi, klo,khi)
  void GetDimensions(int dims[6]) const;

  // Description:
  // Get the low corner index.
  void GetLoCorner(int lo[3]) const;
  const int *GetLoCorner() const { return this->LoCorner; }

  // Description:
  // Copy the high corner index.
  void GetHiCorner(int hi[3]) const;
  const int *GetHiCorner() const { return this->HiCorner; }

  // Description:
  // Gets the number of cells enclosed by the box.
  void GetNumberOfCells(int ext[3]) const;
  vtkIdType GetNumberOfCells() const;

  // Description:
  // Gets the number of nodes required to construct
  // a physical representation of the box.
  void GetNumberOfNodes(int ext[3]) const;
  vtkIdType GetNumberOfNodes() const;

  // Description:
  // Set/Get grid spacing. Refine/coarsen operations update
  // the grid spacing.
  const double *GetGridSpacing() const { return this->DX; }
  void GetGridSpacing(double dX[3]) const;
  void SetGridSpacing(double dx);
  void SetGridSpacing(const double dX[3]);
  void SetGridSpacing(double dx, double dy, double dz);

  // Description:
  // Set/Get world space origin of the data set, The origin
  // is the location of the low corner cell's low corner node
  // of the data set. Which is not necessarilly the origin of
  // this box! For that use GetBoxOrigin().
  const double *GetDataSetOrigin() const { return this->X0; }
  void GetDataSetOrigin(double X0[3]) const;
  void SetDataSetOrigin(const double X0[3]);
  void SetDataSetOrigin(double x0, double y0, double z0);

  // Description:
  // Get the world space origin of this box. The origin is the
  // location of the lower corner cell's lower corner node,
  // which is not necessarilly the orgin of the data set! For
  // that use GetDataSetOrigin(). The value is computed each time,
  // so that operations on the box are are appropriately reflected.
  void GetBoxOrigin(double x0[3]) const;

  // Description:
  // Grows the box in all directions.
  void Grow(int byN);

  // Description:
  // Shrinks the box in all directions.
  void Shrink(int byN);

  // Description:
  // Shifts the box in index space.
  void Shift(int i, int j);
  void Shift(int i, int j, int k);
  void Shift(const int I[3]);

  // Description:
  // Test if this box is empty/valid.
  bool Empty() const;

  // Description:
  // Test if this box has the same dimensions as another.
  bool operator==(const vtkAMRBox &other);

  // Description:
  // Intersect this box with another box in place.
  //void operator&=(const vtkAMRBox &rhs);

  // Description:
  // Test to see if a given cell index is inside this box.
  bool Contains(int i,int j,int k) const;
  bool Contains(const int I[3]) const;

  // Description:
  // Test to see if a given box is inside this box.
  bool Contains(const vtkAMRBox &other) const;

  // Description:
  // Refine the box.
  void Refine(int r);

  // Description:
  // Coarsen the box.
  void Coarsen(int r);

  // Description:
  // Returns the linear index of the given node structured coordinates
  int GetNodeLinearIndex( const int i, const int j, const int k );

  // Description:
  // Returns the linear index of the given cell structured coordinates
  int GetCellLinearIndex( const int i, const int j, const int k );

  // Description:
  // Checks to see if the node corresponding to the given
  // i-j-k coordinates is a ghost node.
  bool IsGhostNode( const int i, const int j, const int k );

  // Description:
  // Writes this instance of AMR box as a uniform grid in a VTK file
  // that can be visualized with ParaView.
  void WriteToVtkFile( const char *file );

  // Description:
  // Gets the real coordinates of the point within the virtual
  // AMR box at the given ijk coordinates.
  // ijk -- the computational coordinate of the point in query (in)
  // pnt -- the physical (real) coordinate of the point (out)
  void GetPoint( const int ijk[3], double pnt[3] ) const;
  void GetPoint( const int i, const int j, const int k, double pnt[3] ) const;

  // Description:
  // Send the box to a stream. "(ilo,jlo,jhi),(ihi,jhi,khi)"
  ostream &Print(ostream &os) const;

  // Description:
  // Serializes this object instance into a byte-stream.
  // buffer   -- user-supplied pointer where the serialized object is stored.
  // bytesize -- number of bytes, i.e., the size of the buffer.
  // NOTE: buffer is allocated internally by this method.
  // Pre-conditions:
  //   buffer == NULL
  // Post-conditions:
  //   buffer   != NULL
  //   bytesize != 0
  void Serialize( unsigned char*& buffer, size_t &bytesize );

  // Description:
  // Deserializes this object instance from the given byte-stream.
  // Pre-conditions:
  //   buffer != NULL
  //   bytesize != 0
  void Deserialize( unsigned char* buffer, const size_t &bytesize );

  // Description:
  // Returns the number of bytes allocated by this instance. In addition,
  // this number of bytes corresponds to the buffer size required to serialize
  // any vtkAMRBox instance.
  static size_t GetBytesize(){return (10*sizeof(int)+6*sizeof(double)); };

  //BTX
  // @deprecated Replaced by Contains() as of VTK 5.4.
  // Do not use! See Contains().
  VTK_LEGACY(int DoesContainCell(int i, int j, int k));
  // @deprecated Replaced by Contains() as of VTK 5.4.
  // Do not use! See Contains().
  VTK_LEGACY(int DoesContainBox(vtkAMRBox const & box) const);
  //ETX

public:

  // Description:
  // These are public for backward compatibility only. If your
  // code uses these, it will break in the future when this class
  // is fixed. Use the above Set/Get'ers. See Get/SetDimensions, 
  // Get/SetXCorner, and the many constructors above.
  int LoCorner[3]; // lo corner cell id.
  int HiCorner[3]; // hi corner cell id.

protected:

  // Description:
  // Initializes this box instance.
  void Initialize( );

private:
  int Dimension;      // 2 or 3
  int BlockId;        // The ID of the corresponding block
  int ProcessId;      // The process ID that owns this block
  int BlockLevel;     // The level of this AMR box instance
  int NG[6];          // Number of ghosts along each dimension
  double X0[3];       // Dataset origin (not box origin)
  double DX[3];       // grid spacing
  int RealExtent[6];  // Extent of the all the real nodes, i.e., not the ghosts



  // Description:
  // A simple method to write a box with the given min/max
  // coordindates for debugging.
  void WriteBox(
      const double x, const double y, const double z,
      const double X, const double Y, const double Z );


};

// NOTE 2008-11-10
// Favor the set'ers above to this helper, where ever possible.
// Helper to unroll the loop
template<int dimension>
struct vtkAMRBoxInitializeHelp;
template<int dimension>
void vtkAMRBoxInitialize(
        int *LoCorner,
        int *HiCorner, // member
        const int *loCorner,
        const int *hiCorner, // local
        vtkAMRBoxInitializeHelp<dimension>* = 0) // dummy parameter for vs6
  {
  for(int i=0; i<dimension; ++i)
    {
    LoCorner[i] = loCorner[i];
    HiCorner[i] = hiCorner[i];
    }
  for(int i=dimension; i<(3-dimension); ++i)
    {
    LoCorner[i] = 0;
    HiCorner[i] = 0;
    }
  }

//*****************************************************************************
// Description:
// Fill the region of "pArray" enclosed by "destRegion" with "fillValue"
// "pArray" is defined on "arrayRegion".
template <typename T>
void FillRegion(
        T *pArray,
        const vtkAMRBox &arrayRegion,
        const vtkAMRBox &destRegion,
        T fillValue)
{
  // Convert regions to array index space. VTK arrays
  // always start with 0,0,0.
  int ofs[3];
  arrayRegion.GetLoCorner(ofs);
  ofs[0]=-ofs[0];
  ofs[1]=-ofs[1];
  ofs[2]=-ofs[2];
  vtkAMRBox arrayDims(arrayRegion);
  arrayDims.Shift(ofs);
  vtkAMRBox destDims(destRegion);
  destDims.Shift(ofs);
  // Quick sanity check.
  if (!arrayRegion.Contains(destRegion))
    {
    vtkGenericWarningMacro(
         << "ERROR: Array must enclose the destination region. "
         << "Aborting the fill.");
    }
  // Get the bounds of the indices we fill.
  int destLo[3];
  destDims.GetLoCorner(destLo);
  int destHi[3];
  destDims.GetHiCorner(destHi);
  // Get the array dimensions.
  int arrayHi[3];
  arrayDims.GetNumberOfCells(arrayHi);
  // Fill.
  for (int k=destLo[2]; k<=destHi[2]; ++k)
    {
    vtkIdType kOfs=k*arrayHi[0]*arrayHi[1];
    for (int j=destLo[1]; j<=destHi[1]; ++j) 
      {
      vtkIdType idx=kOfs+j*arrayHi[0]+destLo[0];
      for (int i=destLo[0]; i<=destHi[0]; ++i)
        {
        pArray[idx]=fillValue;
        ++idx;
        }
      }
    }
}

// Description:
// Split the boxes passed in N times in the i,j and k directions.
// Once a box is split down to a single cell, or the given minimum side length
// it won't be split anymore, but it will propagate through the operation.
void Split(const int N[3], const int minSide[3], vtkstd::vector<vtkAMRBox> &decomp);

// Description:
// Split the boxes passed in in the i,j and k directions, until splitting
// operation would result boxes with side lengths less than the specified
// minimum or the box is split down to a single cell..
void Split(const int minSide[3], vtkstd::vector<vtkAMRBox> &decomp);

#endif

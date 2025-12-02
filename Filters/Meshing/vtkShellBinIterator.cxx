// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkShellBinIterator.h"
#include "vtkStaticPointLocatorPrivate.h"

VTK_ABI_NAMESPACE_BEGIN

namespace // anonymous
{

//------------------------------------------------------------------------------
// Iterate over a *specified face* of a block of bins. Iteration occurs by
// looping around the face center point at increasing face levels. Looping
// continues until the face level is equal to the block level. This struct is
// used in conjunction with BlockIterator to visit bins closer to a
// generating point, and to ping-pong iterate over -/+ face points. The block
// size, and therefore the sizes of the x, y, z faces, is specified by
// blockLevel.
struct BlockFaceIterator
{
  // The current state of iteration.
  int BlockLevel;    // the current block over which iteration is occuring
  int FaceCenter[3]; // the center of iteration of the current block face
  int FaceNum;       // the face being iterated over: 0-xface,1-yface,2-zface
  int FaceLevel;     // the current face level over which we are iterating
  int Span;          // the topological width of the block at the current face level
  int EdgeNum;       // four edges of each face form a loop, the current edge
  int NumIncs;       // the number of iteration steps over the current edge
  int IncNum;        // the current position along a block face edge
  int Inc0[3];       // the index increment in the face "i" direction
  int Inc1[3];       // the index increment in the face "j" direction
  int IJK[3];        // the current IJK index of iteration

  // Constructor requires block face type: i, j, or k.
  BlockFaceIterator(int faceNum)
  {
    // The face being iterated over: -i, -j, or -k. Ping-ponging
    // is used to iterate over the +i, +j, or +k block faces.
    this->FaceNum = faceNum;

    // Specify the increments to move to the next bin during face looping.
    if (this->FaceNum == 0) // -i face
    {
      this->Inc0[0] = 0;
      this->Inc0[1] = 1;
      this->Inc0[2] = 0;
      this->Inc1[0] = 0;
      this->Inc1[1] = 0;
      this->Inc1[2] = 1;
    }
    else if (this->FaceNum == 1) // -j face
    {
      this->Inc0[0] = 1;
      this->Inc0[1] = 0;
      this->Inc0[2] = 0;
      this->Inc1[0] = 0;
      this->Inc1[1] = 0;
      this->Inc1[2] = 1;
    }
    else // if ( this->FaceNum == 0 ) // -k face
    {
      this->Inc0[0] = 1;
      this->Inc0[1] = 0;
      this->Inc0[2] = 0;
      this->Inc1[0] = 0;
      this->Inc1[1] = 1;
      this->Inc1[2] = 0;
    }
  }

  // Update the IJK index.
  void GetIJK(int ijk[3])
  {
    ijk[0] = this->IJK[0];
    ijk[1] = this->IJK[1];
    ijk[2] = this->IJK[2];
  }

  // The following methods are used for looping around the face center point in
  // a counterclockwise direction.
  void MoveRight()
  {
    this->IJK[0] += this->Inc0[0];
    this->IJK[1] += this->Inc0[1];
    this->IJK[2] += this->Inc0[2];
  }
  void MoveUp()
  {
    this->IJK[0] += this->Inc1[0];
    this->IJK[1] += this->Inc1[1];
    this->IJK[2] += this->Inc1[2];
  }
  void MoveLeft()
  {
    this->IJK[0] -= this->Inc0[0];
    this->IJK[1] -= this->Inc0[1];
    this->IJK[2] -= this->Inc0[2];
  }
  void MoveDown()
  {
    this->IJK[0] -= this->Inc1[0];
    this->IJK[1] -= this->Inc1[1];
    this->IJK[2] -= this->Inc1[2];
  }

  // The i,j,k faces of the block overlap along edges and at vertices. To
  // prevent retrieval of bins more than once, we make sure that potentially
  // overlapping vertices are only retrieved once.
  bool IsValid()
  {
    // The i-face always produces a valid IJK. Any i,j,k face loops not
    // touching the block boundary edges are always valid as well.
    if (this->FaceNum == 0 || this->FaceLevel < this->BlockLevel)
    {
      return true;
    }

    // The j-face may revisit some edge bins (due to overlap with the
    // +/- i face edges). So check the +/- i extremes for overlap.
    else if (this->FaceNum == 1)
    {
      if (this->IJK[0] == (this->FaceCenter[0] - this->FaceLevel) ||
        this->IJK[0] == (this->FaceCenter[0] + this->FaceLevel))
      {
        return false;
      }
    }

    // The k-face may revisit some edge bins (due to overlap with the
    // +/- i and +/- j face edges). Note: this code should not be
    // visited since during iteration we prevent processing the outer
    // k-face loop altogether, it's included for debugging purposes.
    else // if ( this->FaceNum == 2 )
    {
      if (this->IJK[0] == (this->FaceCenter[0] - this->FaceLevel) ||
        this->IJK[0] == (this->FaceCenter[0] + this->FaceLevel) ||
        this->IJK[1] == (this->FaceCenter[1] - this->FaceLevel) ||
        this->IJK[1] == (this->FaceCenter[1] + this->FaceLevel))
      {
        return false;
      }
    }

    return true;
  }

  // Initialize the block face iteration process. Indicate the
  // center of the block, and the block level. This is used to
  // specify the face center of iteration.
  void Initialize(int blockLevel, int blockCenter[3])
  {
    this->BlockLevel = blockLevel;
    if (this->FaceNum == 0) // -i face
    {
      this->FaceCenter[0] = blockCenter[0] - blockLevel;
      this->FaceCenter[1] = blockCenter[1];
      this->FaceCenter[2] = blockCenter[2];
    }
    else if (this->FaceNum == 1) // -j face
    {
      this->FaceCenter[0] = blockCenter[0];
      this->FaceCenter[1] = blockCenter[1] - blockLevel;
      this->FaceCenter[2] = blockCenter[2];
    }
    else // if ( this->FaceNum == 0 ) // -k face
    {
      this->FaceCenter[0] = blockCenter[0];
      this->FaceCenter[1] = blockCenter[1];
      this->FaceCenter[2] = blockCenter[2] - blockLevel;
      ;
    }
  }

  // Internal methods for the face iteration process follow below. These just
  // visit the bins on the face of a block without considering topological
  // validity.  The iteration starts at the face center (level==0), and then
  // grows outward one level at a time until the all bins associated with the
  // face are visited. This method updates IJK[3] which is the current bin.
  void BeginBin(int faceLevel)
  {
    // The current level of face iteration
    this->FaceLevel = faceLevel;

    // Special case: starting at the center of the current face.
    if (faceLevel == 0)
    {
      this->IJK[0] = this->FaceCenter[0];
      this->IJK[1] = this->FaceCenter[1];
      this->IJK[2] = this->FaceCenter[2];
      return;
    }

    // General case: iterate over the four edges of the face (basically
    // traverse loops around the face center). We start at the lower left of the
    // current level, moving horizontally NumIncs steps before turning vertically
    // along the next edge.
    this->Span = 2 * faceLevel + 1;

    // Specify various iteration parameters, and the initial starting position
    this->EdgeNum = 0;
    this->IncNum = 0;
    this->NumIncs = this->Span - 1;

    if (this->FaceNum == 0) // -i face
    {
      this->IJK[0] = this->FaceCenter[0];
      this->IJK[1] = this->FaceCenter[1] - faceLevel + 1;
      this->IJK[2] = this->FaceCenter[2] - faceLevel;
    }
    else if (this->FaceNum == 1) // -j face
    {
      this->IJK[0] = this->FaceCenter[0] - faceLevel + 1;
      this->IJK[1] = this->FaceCenter[1];
      this->IJK[2] = this->FaceCenter[2] - faceLevel;
    }
    else // if ( this->FaceNum == 2 ) // -k face
    {
      this->IJK[0] = this->FaceCenter[0] - faceLevel + 1;
      this->IJK[1] = this->FaceCenter[1] - faceLevel;
      this->IJK[2] = this->FaceCenter[2];
    }
  }

  // Return false when iteration over the current faceLevel loop is complete.
  bool NextBin()
  {
    // Special case for face level == 0.
    if (this->FaceLevel == 0)
    {
      return false;
    }

    // The k-face special case for skipping the whole outer loop on
    // the k block face.
    if (this->FaceNum == 2 && this->FaceLevel == this->BlockLevel)
    {
      return false;
    }

    // Now for the general case. We traverse the four edges that
    // form a loop of bins around the face center point.
    this->IncNum++; // advance along current edge
    switch (this->EdgeNum)
    {
      case 0: // lower edge
        if (this->IncNum < this->NumIncs)
        {
          this->MoveRight();
        }
        else // move to next edge
        {
          this->EdgeNum = 1;
          this->IncNum = 0;
          this->MoveUp();
        }
        break;
      case 1: // right edge
        if (this->IncNum < this->NumIncs)
        {
          this->MoveUp();
        }
        else // move to next edge
        {
          this->EdgeNum = 2;
          this->IncNum = 0;
          this->MoveLeft();
        }
        break;
      case 2: // upper edge
        if (this->IncNum < this->NumIncs)
        {
          this->MoveLeft();
        }
        else // move to next edge
        {
          this->EdgeNum = 3;
          this->IncNum = 0;
          this->MoveDown();
        }
        break;
      case 3: // left edge
        if (this->IncNum < this->NumIncs)
        {
          this->MoveDown();
        }
        else // move to next edge
        {
          return false;
        }
        break;
      default: // terminate loop
        return false;
    }

    return true;
  }

  // Basically just sets the iteration starting point for the face loops
  // at the i face level==0.
  void Begin(int ijk[3])
  {
    this->BeginBin(0);
    this->GetIJK(ijk);
  }

  // Return false when iteration over the entirety of the current face
  // completes.  Rectangular rings of nested block face loops are processed
  // until all bins on the face are visited. When all face loops have been
  // processed, then return false;
  bool Next(int ijk[3])
  {
    // Loop until valid IJK is produced
    while (this->NextBin()) // returns false when level is complete
    {
      if (this->IsValid())
      {
        this->GetIJK(ijk);
        return true;
      }
    }

    // If here, the current loop at face level has been exhausted. Start the next
    // face loop at the next level, or check and see if all face loops have been
    // visited.
    if (this->FaceLevel < this->BlockLevel)
    {
      this->BeginBin(this->FaceLevel + 1);
      this->GetIJK(ijk);
      return true;
    }
    else
    {
      return false;
    }
  } // Next()
};  // BlockFaceIterator

//------------------------------------------------------------------------------
// Hard coded block traversals for lower levels. Note the ping-pong
// order (traversal of opposite bins).
const int Level1[26][3] = { { -1, 0, 0 }, { 1, 0, 0 }, { 0, -1, 0 }, { 0, 1, 0 }, { 0, 0, -1 },
  { 0, 0, 1 },                                                // six face centers
  { 0, -1, -1 }, { 0, 1, 1 }, { 0, 1, -1 }, { 0, -1, 1 },     // centers of x-edges
  { -1, 0, -1 }, { 1, 0, 1 }, { 1, 0, -1 }, { -1, 0, 1 },     // centers of y-edges
  { -1, -1, 0 }, { 1, 1, 0 }, { 1, -1, 0 }, { -1, 1, 0 },     // centers of z-edges
  { -1, -1, -1 }, { 1, 1, 1 }, { 1, -1, -1 }, { -1, 1, 1 },   // eight corners
  { -1, 1, -1 }, { 1, -1, 1 }, { 1, 1, -1 }, { -1, -1, 1 } }; // Level1

// Face centers, spiraling out with ping-pong traversal.
const int Level2[98][3] = { { -2, 0, 0 }, { 2, 0, 0 }, { 0, -2, 0 }, { 0, 2, 0 }, { 0, 0, -2 },
  { 0, 0, 2 }, { -2, 0, -1 }, { 2, 0, -1 }, { 0, -2, -1 }, { 0, 2, -1 }, { 0, -1, -2 },
  { 0, -1, 2 }, { -2, 1, -1 }, { 2, 1, -1 }, { 1, -2, -1 }, { 1, 2, -1 }, { 1, -1, -2 },
  { 1, -1, 2 }, { -2, 1, 0 }, { 2, 1, 0 }, { 1, -2, 0 }, { 1, 2, 0 }, { 1, 0, -2 }, { 1, 0, 2 },
  { -2, 1, 1 }, { 2, 1, 1 }, { 1, -2, 1 }, { 1, 2, 1 }, { 1, 1, -2 }, { 1, 1, 2 }, { -2, 0, 1 },
  { 2, 0, 1 }, { 0, -2, 1 }, { 0, 2, 1 }, { 0, 1, -2 }, { 0, 1, 2 }, { -2, -1, 1 }, { 2, -1, 1 },
  { -1, -2, 1 }, { -1, 2, 1 }, { -1, 1, -2 }, { -1, 1, 2 }, { -2, -1, 0 }, { 2, -1, 0 },
  { -1, -2, 0 }, { -1, 2, 0 }, { -1, 0, -2 }, { -1, 0, 2 }, { -2, -1, -1 }, { 2, -1, -1 },
  { -1, -2, -1 }, { -1, 2, -1 }, { -1, -1, -2 }, { -1, -1, 2 }, { -1, -2, -2 }, { -1, 2, -2 },
  { -2, -1, -2 }, { 2, -1, -2 }, { 0, -2, -2 }, { 0, 2, -2 }, { -2, 0, -2 }, { 2, 0, -2 },
  { 1, -2, -2 }, { 1, 2, -2 }, { -2, 1, -2 }, { 2, 1, -2 }, { 1, -2, 2 }, { 1, 2, 2 },
  { -2, 2, -2 }, { 2, 2, -2 }, { 0, -2, 2 }, { 0, 2, 2 }, { -2, 2, -1 }, { 2, 2, -1 },
  { -1, -2, 2 }, { -1, 2, 2 }, { -2, 2, 0 }, { 2, 2, 0 }, { -2, 2, 1 }, { 2, 2, 1 }, { -2, 2, 2 },
  { 2, 2, 2 }, { -2, 1, 2 }, { 2, 1, 2 }, { -2, 0, 2 }, { 2, 0, 2 }, { -2, -1, 2 }, { 2, -1, 2 },
  { -2, -2, 2 }, { 2, -2, 2 }, { -2, -2, 1 }, { 2, -2, 1 }, { -2, -2, 0 }, { 2, -2, 0 },
  { -2, -2, -1 }, { 2, -2, -1 }, { -2, -2, -2 }, { 2, -2, -2 } }; // Level2

const int Level3[218][3] = { { -3, 0, 0 }, { 3, 0, 0 }, { 0, -3, 0 }, { 0, 3, 0 }, { 0, 0, -3 },
  { 0, 0, 3 }, { -3, 0, -1 }, { 3, 0, -1 }, { 0, -3, -1 }, { 0, 3, -1 }, { 0, -1, -3 },
  { 0, -1, 3 }, { -3, 1, -1 }, { 3, 1, -1 }, { 1, -3, -1 }, { 1, 3, -1 }, { 1, -1, -3 },
  { 1, -1, 3 }, { -3, 1, 0 }, { 3, 1, 0 }, { 1, -3, 0 }, { 1, 3, 0 }, { 1, 0, -3 }, { 1, 0, 3 },
  { -3, 1, 1 }, { 3, 1, 1 }, { 1, -3, 1 }, { 1, 3, 1 }, { 1, 1, -3 }, { 1, 1, 3 }, { -3, 0, 1 },
  { 3, 0, 1 }, { 0, -3, 1 }, { 0, 3, 1 }, { 0, 1, -3 }, { 0, 1, 3 }, { -3, -1, 1 }, { 3, -1, 1 },
  { -1, -3, 1 }, { -1, 3, 1 }, { -1, 1, -3 }, { -1, 1, 3 }, { -3, -1, 0 }, { 3, -1, 0 },
  { -1, -3, 0 }, { -1, 3, 0 }, { -1, 0, -3 }, { -1, 0, 3 }, { -3, -1, -1 }, { 3, -1, -1 },
  { -1, -3, -1 }, { -1, 3, -1 }, { -1, -1, -3 }, { -1, -1, 3 }, { -3, -1, -2 }, { 3, -1, -2 },
  { -1, -3, -2 }, { -1, 3, -2 }, { -1, -2, -3 }, { -1, -2, 3 }, { -3, 0, -2 }, { 3, 0, -2 },
  { 0, -3, -2 }, { 0, 3, -2 }, { 0, -2, -3 }, { 0, -2, 3 }, { -3, 1, -2 }, { 3, 1, -2 },
  { 1, -3, -2 }, { 1, 3, -2 }, { 1, -2, -3 }, { 1, -2, 3 }, { -3, 2, -2 }, { 3, 2, -2 },
  { 2, -3, -2 }, { 2, 3, -2 }, { 2, -2, -3 }, { 2, -2, 3 }, { -3, 2, -1 }, { 3, 2, -1 },
  { 2, -3, -1 }, { 2, 3, -1 }, { 2, -1, -3 }, { 2, -1, 3 }, { -3, 2, 0 }, { 3, 2, 0 }, { 2, -3, 0 },
  { 2, 3, 0 }, { 2, 0, -3 }, { 2, 0, 3 }, { -3, 2, 1 }, { 3, 2, 1 }, { 2, -3, 1 }, { 2, 3, 1 },
  { 2, 1, -3 }, { 2, 1, 3 }, { -3, 2, 2 }, { 3, 2, 2 }, { 2, -3, 2 }, { 2, 3, 2 }, { 2, 2, -3 },
  { 2, 2, 3 }, { -3, 1, 2 }, { 3, 1, 2 }, { 1, -3, 2 }, { 1, 3, 2 }, { 1, 2, -3 }, { 1, 2, 3 },
  { -3, 0, 2 }, { 3, 0, 2 }, { 0, -3, 2 }, { 0, 3, 2 }, { 0, 2, -3 }, { 0, 2, 3 }, { -3, -1, 2 },
  { 3, -1, 2 }, { -1, -3, 2 }, { -1, 3, 2 }, { -1, 2, -3 }, { -1, 2, 3 }, { -3, -2, 2 },
  { 3, -2, 2 }, { -2, -3, 2 }, { -2, 3, 2 }, { -2, 2, -3 }, { -2, 2, 3 }, { -3, -2, 1 },
  { 3, -2, 1 }, { -2, -3, 1 }, { -2, 3, 1 }, { -2, 1, -3 }, { -2, 1, 3 }, { -3, -2, 0 },
  { 3, -2, 0 }, { -2, -3, 0 }, { -2, 3, 0 }, { -2, 0, -3 }, { -2, 0, 3 }, { -3, -2, -1 },
  { 3, -2, -1 }, { -2, -3, -1 }, { -2, 3, -1 }, { -2, -1, -3 }, { -2, -1, 3 }, { -3, -2, -2 },
  { 3, -2, -2 }, { -2, -3, -2 }, { -2, 3, -2 }, { -2, -2, -3 }, { -2, -2, 3 }, { -2, -3, -3 },
  { -2, 3, -3 }, { -3, -2, -3 }, { 3, -2, -3 }, { -1, -3, -3 }, { -1, 3, -3 }, { -3, -1, -3 },
  { 3, -1, -3 }, { 0, -3, -3 }, { 0, 3, -3 }, { -3, 0, -3 }, { 3, 0, -3 }, { 1, -3, -3 },
  { 1, 3, -3 }, { -3, 1, -3 }, { 3, 1, -3 }, { 2, -3, -3 }, { 2, 3, -3 }, { -3, 2, -3 },
  { 3, 2, -3 }, { 2, -3, 3 }, { 2, 3, 3 }, { -3, 3, -3 }, { 3, 3, -3 }, { 1, -3, 3 }, { 1, 3, 3 },
  { -3, 3, -2 }, { 3, 3, -2 }, { 0, -3, 3 }, { 0, 3, 3 }, { -3, 3, -1 }, { 3, 3, -1 },
  { -1, -3, 3 }, { -1, 3, 3 }, { -3, 3, 0 }, { 3, 3, 0 }, { -2, -3, 3 }, { -2, 3, 3 }, { -3, 3, 1 },
  { 3, 3, 1 }, { -3, 3, 2 }, { 3, 3, 2 }, { -3, 3, 3 }, { 3, 3, 3 }, { -3, 2, 3 }, { 3, 2, 3 },
  { -3, 1, 3 }, { 3, 1, 3 }, { -3, 0, 3 }, { 3, 0, 3 }, { -3, -1, 3 }, { 3, -1, 3 }, { -3, -2, 3 },
  { 3, -2, 3 }, { -3, -3, 3 }, { 3, -3, 3 }, { -3, -3, 2 }, { 3, -3, 2 }, { -3, -3, 1 },
  { 3, -3, 1 }, { -3, -3, 0 }, { 3, -3, 0 }, { -3, -3, -1 }, { 3, -3, -1 }, { -3, -3, -2 },
  { 3, -3, -2 }, { -3, -3, -3 }, { 3, -3, -3 } }; // Level3

//------------------------------------------------------------------------------
// Iterate over the boundary bins of a block of bins. The block is centered
// at Center, with width/height/depth of 2*Level+1. Note that this iterator
// deals strictly with topological concerns (iteration, and inclusion in
// topological space). For performance, block levels==0 and ==1 are hard
// coded to return the bins in near-optimal order (i.e., bins closer to the
// block center are returned first). The general iteration case (level>1) is
// performed using the BlockFaceIterator.
struct BlockIterator
{
  int Divs[3];        // the topology of a regular (locator) binning
  vtkIdType BinSlice; // the size of a slice of bins
  int MaxLevel;       // maximum level of iteration for the block+center

  // The current state of iteration.
  int Level;     // the current level over which we are iterating
  int Center[3]; // the center of iteration of the block
  int Span;      // the topological width of the block at current level
  int NumBins;   // total number of bins to iterate over at current level
  int BinNum;    // the current iteration position
  int Interior;  // indicate whether (current bin+level) is interior to the block
  int IJK[3];    // the current IJK index of iteration

  // Traversal info to support level>1 general iteration over bins on the
  // faces of the block. We need only the -i, -j, and -k block faces; using
  // ping-ponging (to opposite face) we visit the +i, +j, and +k faces as well.
  int BinNumRange[4];
  BlockFaceIterator IFace, JFace, KFace;

  // Constructor defines three face types (-i, -j, -k).
  BlockIterator()
    : IFace(0)
    , JFace(1)
    , KFace(2)
  {
  }

  // Convenience method to get the current bin id from the current bin IJK.
  vtkIdType GetBinId()
  {
    return (this->IJK[0] + this->IJK[1] * this->Divs[0] + this->IJK[2] * this->BinSlice);
  }

  // Determine if the current bin is within the locator binning.
  bool IsValid()
  {
    if (this->Interior)
    {
      return true;
    }

    if (this->IJK[0] < 0 || this->IJK[0] >= this->Divs[0] || this->IJK[1] < 0 ||
      this->IJK[1] >= this->Divs[1] || this->IJK[2] < 0 || this->IJK[2] >= this->Divs[2])
    {
      return false;
    }
    return true;
  }

  // Initialize the iterator. It is required that the center is
  // within the topological binning specified by divs[3].
  void Initialize(int divs[3], int maxLevel)
  {
    this->Divs[0] = divs[0];
    this->Divs[1] = divs[1];
    this->Divs[2] = divs[2];
    this->BinSlice = divs[0] * divs[1];
    this->MaxLevel = maxLevel;
  }

  // Internal methods for the iteration process follow below. These just
  // visit the surface of the block without considering topological validity.
  // Begin processing the block at the given level. Bins are returned in
  // this->IJK[3]. The level must be >=0, and the center within the specified
  // Divs[3].
  void BeginBin(int level, int center[3])
  {
    // The level being iterated over
    this->Level = level;

    // The center of iteration in topological coordinates.
    this->Center[0] = center[0];
    this->Center[1] = center[1];
    this->Center[2] = center[2];

    // Starting iteration bin is always 0
    this->BinNum = 0;

    // Special case for level 0.
    if (level == 0)
    {
      this->NumBins = 1;
      this->IJK[0] = this->Center[0];
      this->IJK[1] = this->Center[1];
      this->IJK[2] = this->Center[2];
      return;
    }

    // The span. This comes into play later to process edge and face bins
    // when the level>1.
    this->Span = 2 * level + 1;

    // The total number of bins to process
    this->NumBins = (level == 0 ? 1
                                : (this->Span * this->Span * this->Span) -
          ((2 * (level - 1) + 1) * (2 * (level - 1) + 1) * (2 * (level - 1) + 1)));

    // Determine whether the iteration region is inside of the locator. If
    // the iteration region overlaps the boundary (i.e., is not interior),
    // then bin validity checks have to be performed.
    this->Interior = true;
    if ((this->Center[0] - level) < 0 || (this->Center[0] + level) >= this->Divs[0] ||
      (this->Center[1] - level) < 0 || (this->Center[1] + level) >= this->Divs[1] ||
      (this->Center[2] - level) < 0 || (this->Center[2] + level) >= this->Divs[2])
    {
      this->Interior = false;
    }

    // Special case for levels 1-3. The bins are traversed in ping-pong order
    // (-/+ face centers, edges, and then corner points).
    if (level == 1)
    {
      // Iteration always begins at the center of the (-i)-face.
      this->IJK[0] = this->Center[0] + Level1[0][0];
      this->IJK[1] = this->Center[1] + Level1[0][1];
      this->IJK[2] = this->Center[2] + Level1[0][2];
      return;
    }
    else if (level == 2)
    {
      // Iteration always begins at the center of the (-i)-face.
      this->IJK[0] = this->Center[0] + Level2[0][0];
      this->IJK[1] = this->Center[1] + Level2[0][1];
      this->IJK[2] = this->Center[2] + Level2[0][2];
      return;
    }
    else if (level == 3)
    {
      // Iteration always begins at the center of the (-i)-face.
      this->IJK[0] = this->Center[0] + Level3[0][0];
      this->IJK[1] = this->Center[1] + Level3[0][1];
      this->IJK[2] = this->Center[2] + Level3[0][2];
      return;
    }

    // Otherwise general case for level>3. Process the three i, j, k boundary
    // bins of the current cuboid iteration sublock using ping-pong
    // traversal.  Begin by setting up some iteration ranges for each of the
    // i, j, k boundary planes.
    this->BinNumRange[0] = 0;
    this->BinNumRange[1] = 2 * (this->Span * this->Span);
    this->BinNumRange[2] = this->BinNumRange[1] + (2 * this->Span * (this->Span - 2));
    this->BinNumRange[3] = this->BinNumRange[2] + (2 * (this->Span - 2) * (this->Span - 2));
    assert(this->BinNumRange[3] == this->NumBins);

    // Initialize the block face iteration process (for the general case).
    this->IFace.Initialize(level, center);
    this->JFace.Initialize(level, center);
    this->KFace.Initialize(level, center);

    // Start in the center of the i-face.
    this->IFace.Begin(this->IJK);
  } // BeginBin

  // Return false when iteration over the entire block is complete. We do not worry
  // at this point if the returned IJK is valid, this will be checked by the calling
  // Next() method.
  bool NextBin()
  {
    // Advance the current bin number
    this->BinNum++;

    // Special case for level 0, or if visited all bins in the current level
    int level = this->Level;
    if (level == 0 || this->BinNum >= this->NumBins || level > this->MaxLevel)
    {
      return false;
    }

    // Special case for levels 1-3
    if (level == 1)
    {
      this->IJK[0] = this->Center[0] + Level1[this->BinNum][0];
      this->IJK[1] = this->Center[1] + Level1[this->BinNum][1];
      this->IJK[2] = this->Center[2] + Level1[this->BinNum][2];
      return true;
    }
    else if (level == 2)
    {
      this->IJK[0] = this->Center[0] + Level2[this->BinNum][0];
      this->IJK[1] = this->Center[1] + Level2[this->BinNum][1];
      this->IJK[2] = this->Center[2] + Level2[this->BinNum][2];
      return true;
    }
    else if (level == 3)
    {
      this->IJK[0] = this->Center[0] + Level3[this->BinNum][0];
      this->IJK[1] = this->Center[1] + Level3[this->BinNum][1];
      this->IJK[2] = this->Center[2] + Level3[this->BinNum][2];
      return true;
    }

    // Otherwise The general case with level>3: process the points on the
    // boundary of the block.  We visit the -/+i, -/+j, -/+k planes in "round
    // robin" order, at the same time ping ponging from bins on the "-" plane
    // to opposite bins on the "+" plane.

    // The total number of bins on i+j+k faces; and the i+j faces.
    vtkIdType nijk = 3 * (this->BinNumRange[3] - this->BinNumRange[2]);
    vtkIdType nij = 2 * (2 * this->BinNumRange[2] - this->BinNumRange[1] - this->BinNumRange[3]);

    // Process the interior face bins on the +/- i,j, and k faces.
    // These are bins strictly interior to the i,j,k faces.
    const int binNum = this->BinNum;
    if (binNum < nijk)
    {
      const int idx = binNum % 6;
      switch (idx)
      {
        case 0: // The Begin() method has initialized IFace traversal
          this->IFace.Next(this->IJK);
          break;
        case 1:
          this->IJK[0] = this->Center[0] + level; // i-pong
          break;
        case 2:
          if (binNum == 2)
          {
            this->JFace.Begin(this->IJK); // Initialize JFace traversal
          }
          else
          {
            this->JFace.Next(this->IJK);
          }
          break;
        case 3:
          this->IJK[1] = this->Center[1] + level; // j-pong
          break;
        case 4:
          if (binNum == 4)
          {
            this->KFace.Begin(this->IJK); // Initialize KFace traversal
          }
          else
          {
            this->KFace.Next(this->IJK);
          }
          break;
        case 5:
          this->IJK[2] = this->Center[2] + level; // k-pong
          break;
      }
    }

    // Process the remaining bins on +/- i and j faces. These
    // remaining faces are along i,j face edges.
    else if (binNum < (nij + nijk))
    {
      const int idx = binNum % 4;
      switch (idx)
      {
        case 0:
          this->IFace.Next(this->IJK);
          break;
        case 1:
          this->IJK[0] = this->Center[0] + level; // i-pong
          break;
        case 2:
          this->JFace.Next(this->IJK);
          break;
        case 3:
          this->IJK[1] = this->Center[1] + level; // j-pong
          break;
      }
    }

    // Process the remaining bins on +/- i faces. These remaining bins
    // are along i face edges.
    else // if ( this->BinNum < (ni+nij+nijk) )
    {
      if (!(binNum % 2))
      {
        this->IFace.Next(this->IJK);
      }
      else
      {
        this->IJK[0] = this->Center[0] + level; // i-pong
      }
    }
    return true;
  } // NextBin

  // Return a valid binId at the iteration starting point.
  vtkIdType Begin(int blockLevel, int blockCenter[3])
  {
    // Now iterate over this block
    this->BeginBin(blockLevel, blockCenter);
    while (!this->IsValid())
    {
      this->NextBin();
    }
    return this->GetBinId();
  }

  // Return a valid binId, or a -1 if the iteration for this level is
  // complete. Note that the iteration occurs in a prescribed order, so
  // intermediate information is computed incrementally as needed.
  vtkIdType Next()
  {
    // Get the next bin and check whether it is valid. If so,
    // return it. Otherwise, get the next valid bin.
    while (this->BinNum < this->NumBins)
    {
      if (!this->NextBin())
      {
        return (-1);
      }
      if (this->IsValid())
      {
        return this->GetBinId();
      }
    }
    return (-1); // end of bins at current level
  }

}; // BlockIterator

} // anonymous namespace

// Support the templated dispatch process.
struct InternalShellBinIterator
{
  // Stuff that doesn't change once the locator is built
  vtkStaticPointLocator* Locator; // the locator being iterated over
  vtkDataSet* DataSet;            // access dataset points
  double* FastPoints;             // fast path for points access
  int Divs[3];                    // locator bin divisions

  // Stuff that changes over the course of the iteration
  int Level;            // the current level of iteration
  double X[3];          // the center of the iterator in physical space
  int Center[3];        // the center of the iterator in index space
  vtkIdType NumSpheres; // the number of inclusive spheres
  double* Spheres;      // the spheres, 4 tuples with (x,y,z,r2)
  double MinD2;         // minimum distance of the current level to the query point

  // Use to enable / disable bin culling - it's not worth it for low levels.
  int LEVEL_QUERY_THRESHOLD = 3;

  // The core class that performs iteration over blocks of different levels.
  BlockIterator BIter;

  // Fast path for double points.
  double* GetPoint(vtkIdType ptId) { return (this->FastPoints + 3 * ptId); }

  // Get the current bin/bucket id
  vtkIdType GetBinId() { return this->BIter.GetBinId(); }

  // Get the current bin/bucket IJK position
  void GetBin(int ijk[3])
  {
    ijk[0] = this->BIter.IJK[0];
    ijk[1] = this->BIter.IJK[1];
    ijk[2] = this->BIter.IJK[2];
  }
};

namespace // anonymous
{

//------------------------------------------------------------------------------
// Coordinate the iteration process.
template <typename TIds>
struct ShellBinIterator : public InternalShellBinIterator
{
  // Stuff that doesn't change once the locator is built
  BucketList<TIds>* Bins; // templated data buckets

  ShellBinIterator(vtkStaticPointLocator* locator)
  {
    this->Locator = locator;
    this->Bins = static_cast<BucketList<TIds>*>(locator->GetBuckets());
    this->DataSet = locator->GetDataSet();
    this->FastPoints = this->Bins->FastPoints;
    this->Divs[0] = this->Bins->Divisions[0];
    this->Divs[1] = this->Bins->Divisions[1];
    this->Divs[2] = this->Bins->Divisions[2];
    this->BIter.Initialize(this->Divs, this->Bins->MaxLevel);
  }

  // Compute the minimum distance of the block of bins to the center of iteration.
  void ComputeMinD2(int level)
  {
    this->MinD2 = VTK_FLOAT_MAX;
    double d, d2;
    for (auto i = 0; i < 3; ++i)
    {
      if ((this->Center[i] - level) >= 0)
      {
        d = this->X[i] -
          (this->Bins->Bounds[2 * i] + (this->Center[i] - level + 1) * this->Bins->H[i]);
        d2 = d * d;
        this->MinD2 = std::min(d2, this->MinD2);
      }
      if ((this->Center[i] + level) < this->Divs[i])
      {
        d = (this->Bins->Bounds[2 * i] + (this->Center[i] + level) * this->Bins->H[i]) - this->X[i];
        d2 = d * d;
        this->MinD2 = std::min(d2, this->MinD2);
      }
    }
  }

  // Begin iterating over bins, starting at level==0.
  bool Begin(vtkIdType pid, double x[3], vtkDist2TupleArray& results)
  {
    // Clear out any previous results.
    results.clear();

    // Initialize starting values
    this->Level = 0;
    this->X[0] = x[0];
    this->X[1] = x[1];
    this->X[2] = x[2];
    this->MinD2 = 0.0;

    // Find the bucket/bin the point is in. This is the center of the request
    // footprint.
    this->Bins->GetBucketIndices(x, this->Center);
    vtkIdType binIdx = this->BIter.Begin(this->Level, this->Center);

    // Prepare the points for processing. Add all points at level==0.
    vtkIdType numIds = this->Bins->GetNumberOfIds(binIdx);
    const vtkLocatorTuple<TIds>* ids = this->Bins->GetIds(binIdx);
    double* pt;
    for (vtkIdType i = 0; i < numIds; ++i)
    {
      vtkIdType ptId = ids[i].PtId;
      if (pid != ptId)
      {
        pt = this->GetPoint(ptId);
        double d2 = vtkMath::Distance2BetweenPoints(this->X, pt);
        results.emplace_back(ptId, d2);
      }
    }

    // Initial call at level==0 it's best to sort the points
    std::sort(results.begin(), results.end());

    return true;
  }

  bool Next(double rad2, vtkDoubleArray* spheres, vtkDist2TupleArray& results)
  {
    // Grab points in the bin. Make sure there is something useful to
    // return.
    vtkIdType binId, numIds;
    bool foundBin = false;
    while (!foundBin)
    {
      if ((binId = this->BIter.Next()) < 0)
      {
        if (++this->Level >= this->BIter.MaxLevel)
        {
          return false; // completed iteration
        }
        // Increasing to the next level
        this->ComputeMinD2(this->Level);
        if (rad2 < this->MinD2)
        {
          return false; // completed iteration
        }

        // Move on to next level
        binId = this->BIter.Begin(this->Level, this->Center);
      } // if have a valid binId

      // Make sure there are some points in the bin. Otherwise skip.
      if ((numIds = this->Bins->GetNumberOfIds(binId)) <= 0)
      {
        continue;
      }

      // See whether the bin can be culled with the Voronoi flower or circumflower.
      // Culling is most effective at higher levels of block iteration.
      if (this->Level >= this->LEVEL_QUERY_THRESHOLD)
      {
        // See if the bin is outside of the Circumflower / radius of security
        double min[3], max[3];
        this->Bins->GetBucketBounds(BIter.IJK[0], BIter.IJK[1], BIter.IJK[2], min, max);
        if (!vtkBoundingBox::IntersectsSphere(min, max, this->X, rad2))
        {
          continue;
        }
        // In Voronoi flower
        if (spheres)
        {
          vtkIdType sNum, numSpheres = spheres->GetNumberOfTuples();
          const double* sphere = spheres->GetPointer(0);
          for (sNum = 0; sNum < numSpheres; ++sNum, sphere += 4)
          {
            if (vtkBoundingBox::IntersectsSphere(min, max, sphere, sphere[3]))
            {
              break;
            }
          }                       // See if the bin falls into any one of the petals
          if (sNum >= numSpheres) // no intersection
          {
            continue;
          }
        }
      } // if level is large enough to warrant in culling.

      // At this point we can load data from the current bin.
      results.clear();
      const vtkLocatorTuple<TIds>* ids = this->Bins->GetIds(binId);
      double* pt;

      if (numIds == 1)
      {
        vtkIdType ptId = ids[0].PtId;
        pt = this->GetPoint(ptId);
        double d2 = vtkMath::Distance2BetweenPoints(this->X, pt);
        if (d2 <= rad2)
        {
          results.emplace_back(ptId, d2);
        }
      } // adding a single point

      // Find the single point closest to the generator point. This will
      // insert it before the others in the bin, avoiding a few hull clips.
      else // processing more than one point
      {
        double minR2 = VTK_FLOAT_MAX;
        size_t pos = 0;
        for (vtkIdType i = 0; i < numIds; ++i)
        {
          vtkIdType ptId = ids[i].PtId;
          pt = this->GetPoint(ptId);
          double d2 = vtkMath::Distance2BetweenPoints(this->X, pt);
          if (d2 <= rad2)
          {
            results.emplace_back(ptId, d2);
            if (d2 < minR2)
            {
              pos = results.size() - 1;
              minR2 = d2;
            }
          }
        }
        // Swap the closest point in the bin to first position.
        if (results.size() > 1 && pos != 0)
        {
          std::swap(results[0], results[pos]);
        }
      } // potentially insert more than 1 point

      // Make sure there is something to return
      if (!results.empty())
      {
        foundBin = true;
      }
    } // while haven't found non-culled bin with points

    return true;
  }
}; // ShellBinIterator

} // anonymous namespace

//==============================================================================
// Define various dispatch methods.
//------------------------------------------------------------------------------
void vtkShellBinIteratorDispatch::Initialize(vtkStaticPointLocator* locator)
{
  this->LargeIds = locator->GetLargeIds();
  if (this->LargeIds)
  {
    this->Iterator = new ShellBinIterator<vtkIdType>(locator);
  }
  else
  {
    this->Iterator = new ShellBinIterator<int>(locator);
  }
}

//------------------------------------------------------------------------------
bool vtkShellBinIteratorDispatch::Begin(vtkIdType pid, double x[3], vtkDist2TupleArray& results)
{
  if (this->LargeIds)
  {
    return static_cast<ShellBinIterator<vtkIdType>*>(this->Iterator)->Begin(pid, x, results);
  }
  else
  {
    return static_cast<ShellBinIterator<int>*>(this->Iterator)->Begin(pid, x, results);
  }
}

//------------------------------------------------------------------------------
bool vtkShellBinIteratorDispatch::Next(
  double radius2, vtkDoubleArray* spheres, vtkDist2TupleArray& results)
{
  if (this->LargeIds)
  {
    return static_cast<ShellBinIterator<vtkIdType>*>(this->Iterator)
      ->Next(radius2, spheres, results);
  }
  else
  {
    return static_cast<ShellBinIterator<int>*>(this->Iterator)->Next(radius2, spheres, results);
  }
}

//------------------------------------------------------------------------------
vtkIdType vtkShellBinIteratorDispatch::GetBinId()
{
  return this->Iterator->GetBinId();
}

//------------------------------------------------------------------------------
void vtkShellBinIteratorDispatch::GetBin(int IJK[3])
{
  this->Iterator->GetBin(IJK);
}

//------------------------------------------------------------------------------
double vtkShellBinIteratorDispatch::GetMinD2()
{
  return this->Iterator->MinD2;
}

VTK_ABI_NAMESPACE_END

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPixelExtenth.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPixelExtent.h"

using std::deque;
using std::ostream;

//-----------------------------------------------------------------------------
vtkPixelExtent vtkPixelExtent::Grow(
      const vtkPixelExtent &inputExt,
      const vtkPixelExtent &problemDomain,
      int n)
{
  vtkPixelExtent outputExt = vtkPixelExtent::Grow(inputExt, n);
  outputExt &= problemDomain;
  return outputExt;
}

//-----------------------------------------------------------------------------
vtkPixelExtent vtkPixelExtent::Grow(
      const vtkPixelExtent &inputExt,
      int n)
{
  vtkPixelExtent outputExt(inputExt);
  outputExt.Grow(0, n);
  outputExt.Grow(1, n);
  return outputExt;
}

//-----------------------------------------------------------------------------
vtkPixelExtent vtkPixelExtent::GrowLow(
      const vtkPixelExtent &inputExt,
      int q,
      int n)
{
  vtkPixelExtent outputExt(inputExt);
  outputExt[2*q] -= n;
  return outputExt;
}

//-----------------------------------------------------------------------------
vtkPixelExtent vtkPixelExtent::GrowHigh(
      const vtkPixelExtent &inputExt,
      int q,
      int n)
{
  vtkPixelExtent outputExt(inputExt);
  outputExt[2*q+1] += n;
  return outputExt;
}

//-----------------------------------------------------------------------------
vtkPixelExtent vtkPixelExtent::Shrink(
      const vtkPixelExtent &inputExt,
      int n)
{
  return vtkPixelExtent::Grow(inputExt, -n);
}

//-----------------------------------------------------------------------------
vtkPixelExtent vtkPixelExtent::Shrink(
      const vtkPixelExtent &inputExt,
      const vtkPixelExtent &problemDomain,
      int n)
{
  vtkPixelExtent outputExt(inputExt);

  outputExt.Grow(0, -n);
  outputExt.Grow(1, -n);

  // don't shrink at the problem domain
  // because you don't grow outside the problem domain.
  for (int i=0; i<4; ++i)
    {
    if (inputExt[i] == problemDomain[i])
      {
      outputExt[i] = problemDomain[i];
      }
    }

  return outputExt;
}

//-----------------------------------------------------------------------------
vtkPixelExtent vtkPixelExtent::CellToNode(
      const vtkPixelExtent &inputExt)
{
  vtkPixelExtent outputExt(inputExt);
  ++outputExt[1];
  ++outputExt[3];
  return outputExt;
}

//-----------------------------------------------------------------------------
vtkPixelExtent vtkPixelExtent::NodeToCell(const vtkPixelExtent &inputExt)
{
  vtkPixelExtent outputExt(inputExt);
  --outputExt[1];
  --outputExt[3];
  return outputExt;
}

//-----------------------------------------------------------------------------
void vtkPixelExtent::Shift(
      int *ij,
      int n)
{
  ij[0] += n;
  ij[1] += n;
}

//-----------------------------------------------------------------------------
void vtkPixelExtent::Shift(
      int *ij,
      int *n)
{
  ij[0] += n[0];
  ij[1] += n[1];
}

//-----------------------------------------------------------------------------
void vtkPixelExtent::Split(
      int i1,
      int j1,
      const vtkPixelExtent &ext,
      deque<vtkPixelExtent> &newExts)
{
  // cell is inside, split results in as many as
  // 4 new extents. check for each one.
  int i0 = i1 - 1;
  int j0 = j1 - 1;

  int outside = 1;

  // lower left
  if (ext.Contains(i0, j0))
    {
    newExts.push_back(vtkPixelExtent(ext[0], i0, ext[2], j0));
    outside = 0;
    }
  // lower right
  if (ext.Contains(i1, j0))
    {
    newExts.push_back(vtkPixelExtent(i1, ext[1], ext[2], j0));
    outside = 0;
    }
  // upper left
  if (ext.Contains(i0, j1))
    {
    newExts.push_back(vtkPixelExtent(ext[0], i0, j1, ext[3]));
    outside = 0;
    }
  // upper right
  if (ext.Contains(i1, j1))
    {
    newExts.push_back(vtkPixelExtent(i1, ext[1], j1, ext[3]));
    outside = 0;
    }

  // split cell is outside, pass through
  if (outside)
    {
    newExts.push_back(ext);
    return;
    }
}

//-----------------------------------------------------------------------------
void vtkPixelExtent::Subtract(
      const vtkPixelExtent &A,
      vtkPixelExtent B,
      deque<vtkPixelExtent> &C)
{
  // split method requires split point inside the extent
  vtkPixelExtent I(A);
  I &= B;

  if (I.Empty())
    {
    // do nothing if disjoint
    C.push_back(A);
    return;
    }
  if (B.Contains(A))
    {
    // if A is covered by B then remove A
    return;
    }

  // split left and bellow this cells
  I.CellToNode();

  deque<vtkPixelExtent> tmpA0;
  tmpA0.push_back(A);
  for (int q=0; q<4; ++q)
    {
    const int ids[8] = {0,2, 1,2, 1,3, 0,3};
    int qq = 2*q;
    int i = I[ids[qq]];
    int j = I[ids[qq+1]];
    deque<vtkPixelExtent> tmpA1;
    while ( !tmpA0.empty() )
      {
      vtkPixelExtent ext = tmpA0.back();
      tmpA0.pop_back();
      vtkPixelExtent::Split(i,j, ext, tmpA1);
      }
    tmpA0 = tmpA1;
    }

  // remove anything covered by B
  size_t n = tmpA0.size();
  for (size_t q=0; q<n; ++q)
    {
    const vtkPixelExtent &ext = tmpA0[q];
    if (!B.Contains(ext))
      {
      C.push_back(ext);
      }
    }
}

// ----------------------------------------------------------------------------
void vtkPixelExtent::Merge(deque<vtkPixelExtent> &exts)
{
  size_t ne = exts.size();

  // working in point space simplifies things because
  // points overlap in adjacent extents while cells do not
  deque<vtkPixelExtent> tmpExts(ne);
  for (size_t t=0; t<ne; ++t)
    {
    vtkPixelExtent ext(exts[t]);
    ext.CellToNode();
    tmpExts[t] = ext;
    }

  // one pass for each direction
  for (int q=0; q<2; ++q)
    {
    int qq = 2*q;
    // consider each extent as a target to be merged
    for (size_t t=0; t<ne; ++t)
      {
      // if a merger occurs the merged extent is added
      // as a new target with the constituents marked empty
      // and the current pass is terminated early
      int nextPass = 0;

      // current target
      vtkPixelExtent &ext0 = tmpExts[t];
      if (ext0.Empty())
        {
        // was merged in preceeding pass
        continue;
        }

      for (size_t c=0; c<ne; ++c)
        {
        if (c == t)
          {
          // don't attempt merge with self
          continue;
          }

        // candidate
        vtkPixelExtent &ext1 = tmpExts[c];
        if (ext1.Empty())
          {
          // was merged in preceeding pass
          continue;
          }

        // must be same size and coordinate in merge dir
        if ( (ext0[qq] == ext1[qq]) && (ext0[qq+1] == ext1[qq+1]) )
          {
          // must overlap overlap
          vtkPixelExtent ext2(ext0);
          ext2 &= ext1;
          if (!ext2.Empty())
            {
            // merge and add as new target
            // in a later pass
            vtkPixelExtent ext3(ext0);
            ext3 |= ext1;
            tmpExts.push_back(ext3);
            ++ne;

            // mark the merged extents empty
            ext0.Clear();
            ext1.Clear();

            // move to next target
            nextPass = 1;
            break;
            }
          }
        if (nextPass)
          {
          break;
          }
        }
      }
    }
  // discard merged targets copy to output
  exts.clear();
  for (size_t t=0; t<ne; ++t)
    {
    vtkPixelExtent &ext = tmpExts[t];
    if (!ext.Empty())
      {
      ext.NodeToCell();
      exts.push_back(ext);
      }
    }
}

//*****************************************************************************
ostream &operator<<(ostream &os, const vtkPixelExtent &ext)
{
  if (ext.Empty())
    {
    os << "(empty)";
    }
  else
    {
    os
      << "("
      << ext[0] << ", "
      << ext[1] << ", "
      << ext[2] << ", "
      << ext[3] << ")";
    }
  return os;
}

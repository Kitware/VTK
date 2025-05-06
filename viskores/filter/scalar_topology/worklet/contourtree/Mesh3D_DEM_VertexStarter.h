//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
//  Copyright (c) 2016, Los Alamos National Security, LLC
//  All rights reserved.
//
//  Copyright 2016. Los Alamos National Security, LLC.
//  This software was produced under U.S. Government contract DE-AC52-06NA25396
//  for Los Alamos National Laboratory (LANL), which is operated by
//  Los Alamos National Security, LLC for the U.S. Department of Energy.
//  The U.S. Government has rights to use, reproduce, and distribute this
//  software.  NEITHER THE GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY, LLC
//  MAKES ANY WARRANTY, EXPRESS OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE
//  USE OF THIS SOFTWARE.  If software is modified to produce derivative works,
//  such modified software should be clearly marked, so as not to confuse it
//  with the version available from LANL.
//
//  Additionally, redistribution and use in source and binary forms, with or
//  without modification, are permitted provided that the following conditions
//  are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//  3. Neither the name of Los Alamos National Security, LLC, Los Alamos
//     National Laboratory, LANL, the U.S. Government, nor the names of its
//     contributors may be used to endorse or promote products derived from
//     this software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY LOS ALAMOS NATIONAL SECURITY, LLC AND
//  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
//  BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
//  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL LOS ALAMOS
//  NATIONAL SECURITY, LLC OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
//  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
//  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
//  USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
//  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//============================================================================

//  This code is based on the algorithm presented in the paper:
//  “Parallel Peak Pruning for Scalable SMP Contour Tree Computation.”
//  Hamish Carr, Gunther Weber, Christopher Sewell, and James Ahrens.
//  Proceedings of the IEEE Symposium on Large Data Analysis and Visualization
//  (LDAV), October 2016, Baltimore, Maryland.

//=======================================================================================
//
// COMMENTS:
//
// This functor replaces a parallel loop examining neighbours - again, for arbitrary
// meshes, it needs to be a reduction, but for regular meshes, it's faster this way.
//
// Any vector needed by the functor for lookup purposes will be passed as a parameter to
// the constructor and saved, with the actual function call being the operator ()
//
// Vectors marked I/O are intrinsically risky unless there is an algorithmic guarantee
// that the read/writes are completely independent - which for our case actually occurs
// The I/O vectors should therefore be justified in comments both here & in caller
//
//=======================================================================================

#ifndef viskores_worklet_contourtree_mesh3d_dem_vertex_starter_h
#define viskores_worklet_contourtree_mesh3d_dem_vertex_starter_h

#include <viskores/filter/scalar_topology/worklet/contourtree/Mesh3D_DEM_Triangulation_Macros.h>
#include <viskores/filter/scalar_topology/worklet/contourtree/VertexValueComparator.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace worklet
{
namespace contourtree
{

// Worklet for setting initial chain maximum value
template <typename T>
class Mesh3D_DEM_VertexStarter : public viskores::worklet::WorkletMapField
{
public:
  using TagType = viskores::List<T>;

  using ControlSignature = void(FieldIn vertex,      // (input) index of vertex
                                WholeArrayIn values, // (input) values within mesh
                                FieldOut chain,      // (output) modify the chains
                                FieldOut linkMask);  // (output) modify the mask
  using ExecutionSignature = void(_1, _2, _3, _4);
  using InputDomain = _1;

  viskores::Id nRows;   // (input) number of rows in 3D
  viskores::Id nCols;   // (input) number of cols in 3D
  viskores::Id nSlices; // (input) number of cols in 3D
  bool ascending;       // ascending or descending (join or split tree)

  // Constructor
  VISKORES_EXEC_CONT
  Mesh3D_DEM_VertexStarter(viskores::Id NRows,
                           viskores::Id NCols,
                           viskores::Id NSlices,
                           bool Ascending)
    : nRows(NRows)
    , nCols(NCols)
    , nSlices(NSlices)
    , ascending(Ascending)
  {
  }

  // Locate the next vertex in direction indicated
  template <typename InFieldPortalType>
  VISKORES_EXEC void operator()(const viskores::Id& vertex,
                                const InFieldPortalType& values,
                                viskores::Id& chain,
                                viskores::Id& linkMask) const
  {
    VertexValueComparator<InFieldPortalType> lessThan(values);
    viskores::Id row = VERTEX_ROW_3D(vertex, nRows, nCols);
    viskores::Id col = VERTEX_COL_3D(vertex, nRows, nCols);
    viskores::Id slice = VERTEX_SLICE_3D(vertex, nRows, nCols);

    viskores::Id destination = vertex;
    viskores::Id mask = 0;

    bool isLeft = (col == 0);
    bool isRight = (col == nCols - 1);
    bool isTop = (row == 0);
    bool isBottom = (row == nRows - 1);
    bool isFront = (slice == 0);
    bool isBack = (slice == nSlices - 1);

    // This order of processing must be maintained to match the LinkComponentCaseTables
    // and to return the correct destination extremum
    for (viskores::Id edgeNo = (N_INCIDENT_EDGES_3D - 1); edgeNo >= 0; edgeNo--)
    {
      viskores::Id nbr;

      switch (edgeNo)
      {
        ////////////////////////////////////////////////////////
        case 13: // down right back
          if (isBack || isRight || isBottom)
            break;
          nbr = vertex + (nRows * nCols) + nCols + 1;
          if (lessThan(vertex, nbr, ascending))
            break;
          mask |= 0x2000;
          destination = nbr;
          break;

        case 12: // down       back
          if (isBack || isBottom)
            break;
          nbr = vertex + (nRows * nCols) + nCols;
          if (lessThan(vertex, nbr, ascending))
            break;
          mask |= 0x1000;
          destination = nbr;
          break;

        case 11: //      right back
          if (isBack || isRight)
            break;
          nbr = vertex + (nRows * nCols) + 1;
          if (lessThan(vertex, nbr, ascending))
            break;
          mask |= 0x800;
          destination = nbr;
          break;

        case 10: //            back
          if (isBack)
            break;
          nbr = vertex + (nRows * nCols);
          if (lessThan(vertex, nbr, ascending))
            break;
          mask |= 0x400;
          destination = nbr;
          break;

        case 9: // down right
          if (isBottom || isRight)
            break;
          nbr = vertex + nCols + 1;
          if (lessThan(vertex, nbr, ascending))
            break;
          mask |= 0x200;
          destination = nbr;
          break;

        case 8: // down
          if (isBottom)
            break;
          nbr = vertex + nCols;
          if (lessThan(vertex, nbr, ascending))
            break;
          mask |= 0x100;
          destination = nbr;
          break;

        case 7: //      right
          if (isRight)
            break;
          nbr = vertex + 1;
          if (lessThan(vertex, nbr, ascending))
            break;
          mask |= 0x80;
          destination = nbr;
          break;

        case 6: // up left
          if (isLeft || isTop)
            break;
          nbr = vertex - nCols - 1;
          if (lessThan(vertex, nbr, ascending))
            break;
          mask |= 0x40;
          destination = nbr;
          break;

        case 5: //    left
          if (isLeft)
            break;
          nbr = vertex - 1;
          if (lessThan(vertex, nbr, ascending))
            break;
          mask |= 0x20;
          destination = nbr;
          break;

        case 4: //    left front
          if (isLeft || isFront)
            break;
          nbr = vertex - (nRows * nCols) - 1;
          if (lessThan(vertex, nbr, ascending))
            break;
          mask |= 0x10;
          destination = nbr;
          break;

        case 3: //         front
          if (isFront)
            break;
          nbr = vertex - (nRows * nCols);
          if (lessThan(vertex, nbr, ascending))
            break;
          mask |= 0x08;
          destination = nbr;
          break;

        case 2: // up      front
          if (isTop || isFront)
            break;
          nbr = vertex - (nRows * nCols) - nCols;
          if (lessThan(vertex, nbr, ascending))
            break;
          mask |= 0x04;
          destination = nbr;
          break;

        case 1: // up
          if (isTop)
            break;
          nbr = vertex - nCols;
          if (lessThan(vertex, nbr, ascending))
            break;
          mask |= 0x02;
          destination = nbr;
          break;

        case 0: // up left front
          if (isTop || isLeft || isFront)
            break;
          nbr = vertex - (nRows * nCols) - nCols - 1;
          if (lessThan(vertex, nbr, ascending))
            break;
          mask |= 0x01;
          destination = nbr;
          break;
      } // switch on edgeNo
    }   // per edge

    linkMask = mask;
    chain = destination;
  } // operator()
};  // Mesh3D_DEM_VertexStarter
}
}
}

#endif

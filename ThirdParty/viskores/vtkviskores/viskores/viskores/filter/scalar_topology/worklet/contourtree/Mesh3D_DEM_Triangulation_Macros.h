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

#ifndef viskores_worklet_contourtree_mesh3d_dem_triangulation_macros_h
#define viskores_worklet_contourtree_mesh3d_dem_triangulation_macros_h

// macro definitions
#define N_EDGE_TYPES 3
#define EDGE_TYPE_HORIZONTAL 0
#define EDGE_TYPE_VERTICAL 1
#define EDGE_TYPE_DIAGONAL 2

#define N_INCIDENT_EDGES_3D 14
#define MAX_OUTDEGREE_3D 6

// vertex row
#define VERTEX_ROW_3D(V, NROWS, NCOLS) (((V) % (NROWS * NCOLS)) / NCOLS)

// vertex column
#define VERTEX_COL_3D(V, NROWS, NCOLS) ((V) % (NCOLS))

// vertex slice
#define VERTEX_SLICE_3D(V, NROWS, NCOLS) ((V) / (NROWS * NCOLS))

// vertex ID - row * ncols + col
#define VERTEX_ID_3D(S, R, C, NROWS, NCOLS) (((S)*NROWS + (R)) * (NCOLS) + (C))

// edge row - edge / (ncols * nEdgeTypes)
#define EDGE_ROW(E, NCOLS) ((E) / ((NCOLS) * (N_EDGE_TYPES)))
// edge col - (edge / nEdgeTypes) % nCols
#define EDGE_COL(E, NCOLS) (((E) / (N_EDGE_TYPES)) % (NCOLS))
// edge which - edge % nEdgeTypes
#define EDGE_WHICH(E) ((E) % (N_EDGE_TYPES))
// edge ID - (row * ncols + col) * nEdgeTypes + which
#define EDGE_ID(R, C, W, NCOLS) ((((R) * (NCOLS) + (C)) * (N_EDGE_TYPES)) + (W))
// edge from - vertex with same row & col
#define EDGE_FROM(E, NCOLS) VERTEX_ID(EDGE_ROW(E, NCOLS), EDGE_COL(E, NCOLS), NCOLS)
// edge to - edge from +1 col if not vertical, +1 row if not horizontal
#define EDGE_TO(E, NCOLS)                                                           \
  VERTEX_ID(EDGE_ROW(E, NCOLS) + ((EDGE_WHICH(E) == EDGE_TYPE_HORIZONTAL) ? 0 : 1), \
            EDGE_COL(E, NCOLS) + ((EDGE_WHICH(E) == EDGE_TYPE_VERTICAL) ? 0 : 1),   \
            NCOLS)

#endif

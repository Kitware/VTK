##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

set(headers
  ActiveEdgeTransferrer.h
  ChainDoubler.h
  ChainGraph.h
  ContourTree.h
  CopyJoinSplit.h
  CopyNeighbors.h
  CopySupernodes.h
  DegreeDelta.h
  DegreeSubrangeOffset.h
  EdgePeakComparator.h
  FillSupernodes.h
  FindLeaves.h
  GoverningSaddleFinder.h
  JoinArcConnector.h
  JoinSuperArcFinder.h
  JoinTreeTransferrer.h
  LinkComponentCaseTable2D.h
  LinkComponentCaseTable3D.h
  MergeTree.h
  Mesh2D_DEM_SaddleStarter.h
  Mesh2D_DEM_Triangulation.h
  Mesh2D_DEM_Triangulation_Macros.h
  Mesh2D_DEM_VertexOutdegreeStarter.h
  Mesh2D_DEM_VertexStarter.h
  Mesh3D_DEM_SaddleStarter.h
  Mesh3D_DEM_Triangulation.h
  Mesh3D_DEM_Triangulation_Macros.h
  Mesh3D_DEM_VertexOutdegreeStarter.h
  Mesh3D_DEM_VertexStarter.h
  PrintVectors.h
  RegularPointTransferrer.h
  RegularToCandidate.h
  RegularToCriticalDown.h
  RegularToCriticalUp.h
  ResetDegrees.h
  SaddleAscentFunctor.h
  SaddleAscentTransferrer.h
  SetJoinAndSplitArcs.h
  SetSupernodeInward.h
  SkipVertex.h
  SubrangeOffset.h
  TrunkBuilder.h
  Types.h
  UpdateOutbound.h
  VertexDegreeUpdater.h
  VertexMergeComparator.h
  VertexValueComparator.h
  )

#-----------------------------------------------------------------------------
viskores_declare_headers(${headers})

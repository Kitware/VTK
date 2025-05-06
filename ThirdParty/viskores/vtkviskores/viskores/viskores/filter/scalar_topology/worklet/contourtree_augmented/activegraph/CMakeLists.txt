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
  BuildChainsWorklet.h
  BuildTrunkWorklet.h
  CompactActiveEdgesComputeNewVertexOutdegree.h
  CompactActiveEdgesTransferActiveEdges.h
  EdgePeakComparator.h
  FindGoverningSaddlesWorklet.h
  FindSuperAndHyperNodesWorklet.h
  HyperArcSuperNodeComparator.h
  InitializeActiveEdges.h
  InitializeActiveGraphVertices.h
  InitializeEdgeFarFromActiveIndices.h
  InitializeHyperarcsFromActiveIndices.h
  InitializeNeighbourhoodMasksAndOutDegrees.h
  SetArcsConnectNodes.h
  SetArcsSlideVertices.h
  SetArcsSetSuperAndHypernodeArcs.h
  SetHyperArcsWorklet.h
  SetSuperArcsSetTreeHyperparents.h
  SetSuperArcsSetTreeSuperarcs.h
  SuperArcNodeComparator.h
  TransferRegularPointsWorklet.h
  TransferSaddleStartsResetEdgeFar.h
  TransferSaddleStartsSetNewOutdegreeForSaddles.h
  TransferSaddleStartsUpdateEdgeSorter.h
  )

#-----------------------------------------------------------------------------
viskores_declare_headers(${headers})

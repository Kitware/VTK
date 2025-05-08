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
  Predicates.h
  TopVolumeBranchData.h
  AboveThresholdWorklet.h
  AssignValueWorklet.h
  BinarySearchWorklet.h
  BranchSaddleIsKnownWorklet.h
  ClarifyBranchEndSupernodeTypeWorklet.h
  CollectOuterSaddleWorklet.h
  UpdateOuterSaddleWorklet.h
  UpdateInfoByBranchDirectionWorklet.h
  GetParentBranchWorklet.h
  GetBranchHierarchyWorklet.h
  GetBranchVolumeWorklet.h
  BranchParentComparator.h
  BranchVolumeComparator.h
  BranchDecompositionTreeMaker.h
)
#-----------------------------------------------------------------------------

viskores_declare_headers(${headers})

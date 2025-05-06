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
  BoundaryTree.h
  BoundaryTreeMaker.h
  CombineHyperSweepBlockFunctor.h
  ComputeDistributedContourTreeFunctor.h
  ContourTreeBlockData.h
  DistributedContourTreeBlockData.h
  HierarchicalAugmenter.h
  HierarchicalAugmenterFunctor.h
  HierarchicalContourTree.h
  HierarchicalHyperSweeper.h
  HyperSweepBlock.h
  InteriorForest.h
  MergeBlockFunctor.h
  MultiBlockContourTreeHelper.h
  PrintGraph.h
  TreeCompiler.h
  TreeGrafter.h
  BranchCompiler.h
)

viskores_declare_headers(${headers})

add_subdirectory(boundary_tree_maker)
add_subdirectory(tree_grafter)
add_subdirectory(hierarchical_contour_tree)
add_subdirectory(hierarchical_hyper_sweeper)
add_subdirectory(hierarchical_augmenter)

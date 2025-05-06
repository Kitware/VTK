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
  ContourTreeUniform.h
  ContourTreeUniformAugmented.h
  )

viskores_declare_headers(${headers})

add_subdirectory(branch_decomposition)
add_subdirectory(select_top_volume_branches)
add_subdirectory(extract_top_volume_contours)
add_subdirectory(contourtree)
add_subdirectory(contourtree_augmented)
add_subdirectory(contourtree_distributed)

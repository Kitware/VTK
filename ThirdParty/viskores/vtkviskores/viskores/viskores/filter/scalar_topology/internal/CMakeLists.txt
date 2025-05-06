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
  BranchDecompositionBlock.h
  SelectTopVolumeBranchesBlock.h
  ExtractTopVolumeContoursBlock.h
  ComputeBlockIndices.h
  ComputeDistributedBranchDecompositionFunctor.h
  UpdateParentBranchFunctor.h
  SelectTopVolumeBranchesFunctor.h
  ExchangeBranchEndsFunctor.h
  )
#-----------------------------------------------------------------------------

# Note: The C++ source files BranchDecompositionBlock.cxx and
# ComputeDistributedBranchDecompositionFunctor.cxx are added to the scalar
# topology library in the CMakeLists.txt in our parent directory.

viskores_declare_headers(${headers})

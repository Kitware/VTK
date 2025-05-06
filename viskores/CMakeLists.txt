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

# Configure version file. (Other configuration in internal/Configure.h)
viskores_get_kit_name(kit_name kit_dir)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Version.h.in
  ${Viskores_BINARY_INCLUDE_DIR}/${kit_dir}/Version.h
  @ONLY)
viskores_install_headers(
  viskores ${Viskores_BINARY_INCLUDE_DIR}/${kit_dir}/Version.h)

set(headers
  Assert.h
  Atomic.h
  BinaryPredicates.h
  BinaryOperators.h
  Bitset.h
  Bounds.h
  CellClassification.h
  CellShape.h
  CellTraits.h
  Deprecated.h
  ErrorCode.h
  Flags.h
  Geometry.h
  Hash.h
  ImplicitFunction.h
  List.h
  LowerBound.h
  Math.h
  Matrix.h
  NewtonsMethod.h
  Pair.h
  Particle.h
  Range.h
  RangeId.h
  RangeId2.h
  RangeId3.h
  StaticAssert.h
  Swap.h
  TopologyElementTag.h
  Transform3D.h
  Tuple.h
  TypeList.h
  Types.h
  TypeTraits.h
  VecAxisAlignedPointCoordinates.h
  VecFromPortal.h
  VecFromPortalPermute.h
  VectorAnalysis.h
  VecFlat.h
  VecTraits.h
  VecVariable.h
  UnaryPredicates.h
  UpperBound.h
)

set(template_sources
  Geometry.hxx
)

viskores_pyexpander_generated_file(Math.h)
viskores_pyexpander_generated_file(Tuple.h)

viskores_declare_headers(
  ${headers}
  ${template_sources}
)

# Declare testing headers to install. (Needs to be special because they are in a different dir.)
set(testing_headers
  testing/Testing.h
  testing/VecTraitsTests.h
  )
viskores_install_headers(${kit_dir}/testing ${testing_headers})

add_subdirectory(internal)

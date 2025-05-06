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
#ifndef viskores_TypeList_h
#define viskores_TypeList_h

#include <viskores/List.h>
#include <viskores/Types.h>

namespace viskores
{

/// A list containing the type viskores::Id.
///
using TypeListId = viskores::List<viskores::Id>;

/// A list containing the type viskores::Id2.
///
using TypeListId2 = viskores::List<viskores::Id2>;

/// A list containing the type viskores::Id3.
///
using TypeListId3 = viskores::List<viskores::Id3>;

/// A list containing the type viskores::Id4.
///
using TypeListId4 = viskores::List<viskores::Id4>;

/// A list containing the type viskores::IdComponent
///
using TypeListIdComponent = viskores::List<viskores::IdComponent>;

/// A list containing types used to index arrays. Contains viskores::Id, viskores::Id2,
/// and viskores::Id3.
///
using TypeListIndex = viskores::List<viskores::Id, viskores::Id2, viskores::Id3>;

/// A list containing types used for scalar fields. Specifically, contains
/// floating point numbers of different widths (i.e. viskores::Float32 and
/// viskores::Float64).
using TypeListFieldScalar = viskores::List<viskores::Float32, viskores::Float64>;

/// A list containing types for values for fields with two dimensional
/// vectors.
///
using TypeListFieldVec2 = viskores::List<viskores::Vec2f_32, viskores::Vec2f_64>;

/// A list containing types for values for fields with three dimensional
/// vectors.
///
using TypeListFieldVec3 = viskores::List<viskores::Vec3f_32, viskores::Vec3f_64>;

/// A list containing types for values for fields with four dimensional
/// vectors.
///
using TypeListFieldVec4 = viskores::List<viskores::Vec4f_32, viskores::Vec4f_64>;

/// A list containing common types for floating-point vectors. Specifically contains
/// floating point vectors of size 2, 3, and 4 with floating point components.
/// Scalars are not included.
///
using TypeListFloatVec = viskores::List<viskores::Vec2f_32,
                                        viskores::Vec2f_64,
                                        viskores::Vec3f_32,
                                        viskores::Vec3f_64,
                                        viskores::Vec4f_32,
                                        viskores::Vec4f_64>;

/// A list containing common types for values in fields. Specifically contains
/// floating point scalars and vectors of size 2, 3, and 4 with floating point
/// components.
///
using TypeListField = viskores::List<viskores::Float32,
                                     viskores::Float64,
                                     viskores::Vec2f_32,
                                     viskores::Vec2f_64,
                                     viskores::Vec3f_32,
                                     viskores::Vec3f_64,
                                     viskores::Vec4f_32,
                                     viskores::Vec4f_64>;

/// A list of all scalars defined in viskores/Types.h. A scalar is a type that
/// holds a single number. This should containing all true variations of
/// scalars, but there might be some arithmetic C types not included. For
/// example, this list contains `signed char`, and `unsigned char`, but not
/// `char` as one of those types will behave the same as it. Two of the three
/// types behave the same, but be aware that template resolution will treat
/// them differently.
///
using TypeListScalarAll = viskores::List<viskores::Int8,
                                         viskores::UInt8,
                                         viskores::Int16,
                                         viskores::UInt16,
                                         viskores::Int32,
                                         viskores::UInt32,
                                         viskores::Int64,
                                         viskores::UInt64,
                                         viskores::Float32,
                                         viskores::Float64>;

// A list that containes all the base arithmetric C types (i.e. char, int, float, etc.).
// The list contains C types that are functionally equivalent but considered different
// types (e.g. it contains both `char` and `signed char`).
using TypeListBaseC = viskores::ListAppend<
  viskores::TypeListScalarAll,
  // Other base C types that are the same as above but
  // recognized as different by the compiler
  viskores::List<bool, char, signed VISKORES_UNUSED_INT_TYPE, unsigned VISKORES_UNUSED_INT_TYPE>>;

/// A list of the most commonly use Vec classes. Specifically, these are
/// vectors of size 2, 3, or 4 containing either unsigned bytes, signed
/// integers of 32 or 64 bits, or floating point values of 32 or 64 bits.
///
using TypeListVecCommon = viskores::List<viskores::Vec2ui_8,
                                         viskores::Vec2i_32,
                                         viskores::Vec2i_64,
                                         viskores::Vec2f_32,
                                         viskores::Vec2f_64,
                                         viskores::Vec3ui_8,
                                         viskores::Vec3i_32,
                                         viskores::Vec3i_64,
                                         viskores::Vec3f_32,
                                         viskores::Vec3f_64,
                                         viskores::Vec4ui_8,
                                         viskores::Vec4i_32,
                                         viskores::Vec4i_64,
                                         viskores::Vec4f_32,
                                         viskores::Vec4f_64>;

namespace internal
{

/// A list of uncommon Vec classes with length up to 4. This is not much
/// use in general, but is used when joined with \c TypeListVecCommon
/// to get a list of all vectors up to size 4.
///
using TypeListVecUncommon = viskores::List<viskores::Vec2i_8,
                                           viskores::Vec2i_16,
                                           viskores::Vec2ui_16,
                                           viskores::Vec2ui_32,
                                           viskores::Vec2ui_64,
                                           viskores::Vec3i_8,
                                           viskores::Vec3i_16,
                                           viskores::Vec3ui_16,
                                           viskores::Vec3ui_32,
                                           viskores::Vec3ui_64,
                                           viskores::Vec4i_8,
                                           viskores::Vec4i_16,
                                           viskores::Vec4ui_16,
                                           viskores::Vec4ui_32,
                                           viskores::Vec4ui_64>;

} // namespace internal

/// A list of all vector classes with standard types as components and
/// lengths between 2 and 4.
///
using TypeListVecAll =
  viskores::ListAppend<viskores::TypeListVecCommon, viskores::internal::TypeListVecUncommon>;

/// A list of all basic types listed in viskores/Types.h. Does not include all
/// possible Viskores types like arbitrarily typed and sized Vecs (only up to
/// length 4) or math types like matrices.
///
using TypeListAll = viskores::ListAppend<viskores::TypeListScalarAll, viskores::TypeListVecAll>;

/// A list of the most commonly used types across multiple domains. Includes
/// integers, floating points, and 3 dimensional vectors of floating points.
///
using TypeListCommon = viskores::List<viskores::UInt8,
                                      viskores::Int32,
                                      viskores::Int64,
                                      viskores::Float32,
                                      viskores::Float64,
                                      viskores::Vec3f_32,
                                      viskores::Vec3f_64>;

} // namespace viskores

#endif //viskores_TypeList_h

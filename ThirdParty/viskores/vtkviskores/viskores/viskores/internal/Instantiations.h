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

#ifndef viskores_internal_Instantiations_h
#define viskores_internal_Instantiations_h
///
/// The following empty macros are instantiation delimiters used by
/// `vtk_add_instantiations` at CMake/ViskoresWrappers.cmake to generate transient
/// instantiation files at the build directory.
///
/// # Example #
///
/// ```cpp
/// VISKORES_INSTANTIATION_BEGIN
/// extern template viskores::cont::ArrayHandle<viskores::Vec<viskores::Vec3f_32, 3>>
/// viskores::worklet::CellGradient::Run(
///   const viskores::cont::UnknownCellSet&,
///   const viskores::cont::CoordinateSystem&,
///   const viskores::cont::ArrayHandle<viskores::Vec3f_32, viskores::cont::StorageTagSOA>&,
///   GradientOutputFields<viskores::Vec3f_32>&);
/// VISKORES_INSTANTIATION_END
/// ```
///
/// # KNOWN ISSUES #
///
/// Abstain to use the following constructors in the code section between
/// the VISKORES_INSTANTIATION_BEGIN/END directives:
///
/// - The word extern other than for extern template.
/// - The word _TEMPLATE_EXPORT other then for the EXPORT macro.
/// - Comments that use the '$' symbol.
/// - Symbols for functions or methods that are inline. This includes methods
///   with implementation defined in the class/struct definition.
///
/// # See Also #
///
/// See the documentation for the `viskores_add_instantiations` function in
/// CMake/ViskoresWrappers.cmake for more information.
///
#define VISKORES_INSTANTIATION_BEGIN
#define VISKORES_INSTANTIATION_END

#endif //viskores_internal_Instantiations_h

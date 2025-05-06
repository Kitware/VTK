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
#ifndef viskores_cont_arg_Transport_h
#define viskores_cont_arg_Transport_h

namespace viskores
{
namespace cont
{
namespace arg
{

/// \brief Class for transporting from the control to the execution environment.
///
/// The \c Transport class is used to transport data of a certain type from the
/// control environment to the execution environment. It is used internally in
/// Viskores's dispatch mechanism.
///
/// \c Transport is a templated class with three arguments. The first argument
/// is a tag declaring the mechanism of transport. The second argument is the
/// type of data to transport. The third argument is device adapter tag for
/// the device to move the data to.
///
/// There is no generic implementation of \c Transport. There are partial
/// specializations of \c Transport for each mechanism supported. If you get a
/// compiler error about an incomplete type for \c Transport, it means you used
/// an invalid \c TransportTag or it is an invalid combination of data type or
/// device adapter.
///
template <typename TransportTag, typename ContObjectType, typename DeviceAdapterTag>
struct Transport
#ifdef VISKORES_DOXYGEN_ONLY
{
  /// \brief The type used in the execution environment.
  ///
  /// All \c Transport specializations are expected to declare a type named \c
  /// ExecObjectType that is the object type used in the execution environment.
  /// For example, for an \c ArrayHandle, the \c ExecObjectType is the portal
  /// used in the execution environment.
  ///
  using ExecObjectType = typename ContObjectType::ReadPortalType;

  /// \brief Send data to the execution environment.
  ///
  /// All \c Transport specializations are expected to have a constant
  /// parenthesis operator that takes the data in the control environment and
  /// returns an object that is accessible in the execution environment. The
  /// second argument of the operator is a reference to the input domain
  /// argument. This might be useful for checking the sizes of input data. The
  /// third argument is the size of the output domain, which can be used, for
  /// example, to allocate data for an output array. The transport might ignore
  /// either or both of the second two arguments.
  ///
  template <typename InputDomainType>
  VISKORES_CONT ExecObjectType
  operator()(const ContObjectType contData,
             const InputDomainType& inputDomain viskores::Id outputSize) const;
};
#else  // VISKORES_DOXYGEN_ONLY
  ;
#endif // VISKORES_DOXYGEN_ONLY
}
}
} // namespace viskores::cont::arg

#endif //viskores_cont_arg_Transport_h

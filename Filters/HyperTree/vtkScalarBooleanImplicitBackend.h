// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @struct vtkScalarBooleanImplicitBackend
 * @brief A utility structure serving as a backend for boolean implicit arrays
 *
 * This backend unpacks a bool array to an array of type `ValueType`,
 * reducing the memory footprint of the array by a factor of 8 * 8 if `ValueType` is `double`
 * while still guaranteeing fast element access using static dispatch.
 */

#ifndef vtkScalarBooleanImplicitBackend_h
#define vtkScalarBooleanImplicitBackend_h

template <typename ValueType>
struct vtkScalarBooleanImplicitBackend
{
  /**
   * Build the implicit array using a bit vector to be unpacked.
   *
   * @param values Lookup vector to use
   */
  vtkScalarBooleanImplicitBackend(const std::vector<bool>& values)
    : Values(values)
  {
  }

  /**
   * Templated method called for element access
   *
   * @param _index: Array element id
   * \return Array element in the templated type
   */
  ValueType operator()(const int _index) const
  {
    return static_cast<ValueType>(this->Values[_index]);
  }

  const std::vector<bool> Values;
};

#endif // vtkScalarBooleanImplicitBackend_h

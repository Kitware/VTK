// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkArrayComponents_h
#define vtkArrayComponents_h

#include "vtkCommonCoreModule.h" // For export macros
#include "vtkCompiler.h"         // For VTK_ALWAYS_EXPORT
#include "vtkDataArray.h"        // For API
#include "vtkSmartPointer.h"     // For API

#include <string>

VTK_ABI_NAMESPACE_BEGIN

class vtkAbstractArray;
class vtkDataArray;

/**
 * An enumeration for specifying components of a vtkAbstractArray's tuples.
 *
 * Values from this enumeration are passed to methods which expect a component index
 * when you wish to indicate either (1) the entire tuple be considered instead of a
 * single component or (2) a norm or other scalar function computed from the entire
 * tuple be considered as a "virtual" component.
 *
 * Values in this enumeration are accepted by the vtkAlgorithm::SetInputArrayToProcess()
 * and vtkAlgorithm::GetInputArray() methods.
 *
 * The Requested enumerant is used to indicate that the component specified by
 * vtkAlgorithm::SetInputArrayToProcess() should be used rather than overriding
 * it by a component-id passed to vtkAlgorithm::GetInputArray().
 */
enum vtkArrayComponents : int
{
  L1Norm = -1,          // Take the L₁ norm of all components and treat it as a virtual component.
  L2Norm = -2,          // Take the L₂ norm of all components and treat it as a virtual component.
  LInfNorm = -99,       // Take the L∞ norm of all components and treat it as a virtual component.
  AllComponents = -100, // Do not isolate a single component; use the entire tuple value.
  Requested = -101      // Use whatever component(s) a filter was requested to process.
};

VTK_ABI_NAMESPACE_END

namespace vtk
{

VTK_ABI_NAMESPACE_BEGIN

/// Given a string, return an "array component" enumerant.
int VTKCOMMONCORE_EXPORT ArrayComponents(const std::string& enumerantStr);

/// Given an enumerant, return a human-presentable string with its value.
///
/// The returned string is encoded as UTF-8 unicode data.
std::string VTKCOMMONCORE_EXPORT to_string(vtkArrayComponents enumerant);

/// A generic template to convert strings to enumerant values.
template <typename T>
T VTK_ALWAYS_EXPORT to_enumerant(const std::string&);

/// A specialization of the generic template above.
template <>
inline vtkArrayComponents to_enumerant<vtkArrayComponents>(const std::string& enumerantStr)
{
  return static_cast<vtkArrayComponents>(ArrayComponents(enumerantStr));
}

///@{
/**\brief Return a new array with a single component whose tuple-values
 *       are either a component of the input array or a norm of each input tuple.
 *
 * The \a compOrNorm parameter must be either a non-negative integer (a component
 * index) or a value from the vtkArrayComponents enumeration.
 *
 * When L₁ or L₂ norms are requested for integer-valued input arrays, the implicit
 * array returned to you will have `double`-precision values.
 * All other norm- and component-selections will return implicit arrays whose type
 * matches the input array type. This was done since the L₁ and L₂ norms may run
 * into overflow and/or precision issues if the storage type of component values
 * is used to hold norm values.
 *
 * If you ask for a non-existent component, a null array will be returned.
 *
 * The resulting array name will either exactly match the input array or – if
 * a component or norm was extracted – have an underscore and the component (or
 * norm) appended to it.
 */
vtkSmartPointer<vtkAbstractArray> VTKCOMMONCORE_EXPORT ComponentOrNormAsArray(
  vtkAbstractArray* array, int compOrNorm);

inline vtkSmartPointer<vtkDataArray> ComponentOrNormAsDataArray(vtkDataArray* array, int compOrNorm)
{
  // This variant only handles input data arrays, but returns a vtkDataArray
  // rather than forcing you to cast it afterward.
  auto intermediate = ComponentOrNormAsArray(array, compOrNorm);
  vtkSmartPointer<vtkDataArray> result = vtkDataArray::SafeDownCast(intermediate.GetPointer());
  return result;
}
///@}

VTK_ABI_NAMESPACE_END

} // vtk namespace

#endif // vtkArrayComponents_h
// VTK-HeaderTest-Exclude: vtkArrayComponents.h

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
#ifndef viskores_source_OscillatorSource_h
#define viskores_source_OscillatorSource_h

#include <viskores/source/Source.h>

namespace viskores
{
namespace source
{

/**\brief An analytical, time-varying uniform dataset with a point based array
 *
 * The Execute method creates a complete structured dataset that have a
 * point field names 'oscillating'
 *
 * This array is based on the coordinates and evaluates to a sum of time-varying
 * Gaussian exponentials specified in its configuration.
 */
class VISKORES_SOURCE_EXPORT Oscillator final : public viskores::source::Source
{
public:
  /// \brief Constructs an oscillator source with default parameters.
  ///
  /// The default point dimensions are 3 by 3 by 3. Oscillators can be added
  /// with \c AddPeriodic, \c AddDamped, and \c AddDecaying.
  VISKORES_CONT Oscillator();

  VISKORES_CONT VISKORES_DEPRECATED(
    2.0,
    "Use SetCellDimensions or SetPointDimensions.") explicit Oscillator(viskores::Id3 dims);

  // We can not declare default destructor here since compiler does not know how
  // to create one for the Worklet at this point yet. However, the implementation
  // in Oscillator.cxx does have ~Oscillator() = default;
  VISKORES_CONT ~Oscillator() override;

  /// \brief Sets the number of points in each dimension.
  ///
  /// The dimensions must be greater than 1 in each direction to generate cells.
  VISKORES_CONT void SetPointDimensions(viskores::Id3 pointDimensions);

  /// \brief Gets the number of points in each dimension.
  VISKORES_CONT viskores::Id3 GetPointDimensions() const;

  /// \brief Sets the number of cells in each dimension.
  ///
  /// The point dimensions are set to one more than the given cell dimensions in
  /// each direction.
  VISKORES_CONT void SetCellDimensions(viskores::Id3 pointDimensions);

  /// \brief Gets the number of cells in each dimension.
  ///
  /// This value is computed from the point dimensions by subtracting 1 in each
  /// direction.
  VISKORES_CONT viskores::Id3 GetCellDimensions() const;

  /// \brief Sets the time used to evaluate the oscillator field.
  VISKORES_CONT
  void SetTime(viskores::FloatDefault time);

  /// \brief Adds a periodic oscillator contribution.
  ///
  /// The first three parameters specify the oscillator center. The radius
  /// controls the Gaussian spatial falloff. The omega and zeta parameters
  /// control the oscillator's temporal behavior.
  VISKORES_CONT
  void AddPeriodic(viskores::FloatDefault x,
                   viskores::FloatDefault y,
                   viskores::FloatDefault z,
                   viskores::FloatDefault radius,
                   viskores::FloatDefault omega,
                   viskores::FloatDefault zeta);

  /// \brief Adds a damped oscillator contribution.
  ///
  /// The first three parameters specify the oscillator center. The radius
  /// controls the Gaussian spatial falloff. The omega and zeta parameters
  /// control the oscillator's temporal behavior.
  VISKORES_CONT
  void AddDamped(viskores::FloatDefault x,
                 viskores::FloatDefault y,
                 viskores::FloatDefault z,
                 viskores::FloatDefault radius,
                 viskores::FloatDefault omega,
                 viskores::FloatDefault zeta);

  /// \brief Adds a decaying oscillator contribution.
  ///
  /// The first three parameters specify the oscillator center. The radius
  /// controls the Gaussian spatial falloff. The omega and zeta parameters
  /// control the oscillator's temporal behavior.
  VISKORES_CONT
  void AddDecaying(viskores::FloatDefault x,
                   viskores::FloatDefault y,
                   viskores::FloatDefault z,
                   viskores::FloatDefault radius,
                   viskores::FloatDefault omega,
                   viskores::FloatDefault zeta);

private:
  VISKORES_CONT viskores::cont::DataSet DoExecute() const override;

  struct InternalStruct;
  std::unique_ptr<InternalStruct> Internals;
};
}
}

#endif // viskores_source_OscillatorSource_h

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
#ifndef viskores_cont_internal_PointLocatorBase_h
#define viskores_cont_internal_PointLocatorBase_h

#include <viskores/cont/viskores_cont_export.h>

#include <viskores/Types.h>
#include <viskores/cont/CoordinateSystem.h>
#include <viskores/cont/ExecutionObjectBase.h>

namespace viskores
{
namespace cont
{

/// @brief Base class for all `PointLocator` classes.
///
/// `PointLocatorBase` subclasses must implement the pure virtual `Build()` method.
/// They also must provide a `PrepareForExecution()` method to satisfy the
/// `ExecutionObjectBase` superclass.
///
/// If a derived class changes its state in a way that invalidates its internal search
/// structure, it should call the protected `SetModified()` method. This will alert the
/// base class to rebuild the structure on the next call to `Update()`.
class VISKORES_CONT_EXPORT PointLocatorBase : public viskores::cont::ExecutionObjectBase
{
public:
  virtual ~PointLocatorBase() = default;

  /// @brief Specify the `CoordinateSystem` defining the location of the cells.
  ///
  /// This is often retrieved from the `viskores::cont::DataSet::GetCoordinateSystem()` method,
  /// but it can be any array of size 3 `Vec`s.
  viskores::cont::CoordinateSystem GetCoordinates() const { return this->Coords; }
  /// @copydoc GetCoordinates
  void SetCoordinates(const viskores::cont::CoordinateSystem& coords)
  {
    this->Coords = coords;
    this->SetModified();
  }
  /// @copydoc GetCoordinates
  VISKORES_CONT void SetCoordinates(const viskores::cont::UnknownArrayHandle& coords)
  {
    this->SetCoordinates({ "coords", coords });
  }

  void Update() const;

protected:
  void SetModified() { this->Modified = true; }
  bool GetModified() const { return this->Modified; }

  virtual void Build() = 0;

private:
  viskores::cont::CoordinateSystem Coords;
  mutable bool Modified = true;
};

} // viskores::cont
} // viskores

#endif // viskores_cont_internal_PointLocatorBase_h

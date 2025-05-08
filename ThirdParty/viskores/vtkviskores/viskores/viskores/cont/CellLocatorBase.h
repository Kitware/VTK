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
#ifndef viskores_cont_internal_CellLocatorBase_h
#define viskores_cont_internal_CellLocatorBase_h

#include <viskores/cont/viskores_cont_export.h>

#include <viskores/Types.h>
#include <viskores/cont/CoordinateSystem.h>
#include <viskores/cont/ExecutionObjectBase.h>
#include <viskores/cont/UnknownCellSet.h>

namespace viskores
{
namespace cont
{

/// @brief Base class for all `CellLocator` classes.
///
/// `CellLocatorBase` subclasses must implement the pure virtual `Build()` method.
/// They also must provide a `PrepareForExecution()` method to satisfy the
/// `ExecutionObjectBase` superclass.
///
/// If a derived class changes its state in a way that invalidates its internal search
/// structure, it should call the protected `SetModified()` method. This will alert the
/// base class to rebuild the structure on the next call to `Update()`.
class VISKORES_CONT_EXPORT CellLocatorBase : public viskores::cont::ExecutionObjectBase
{
  viskores::cont::UnknownCellSet CellSet;
  viskores::cont::CoordinateSystem Coords;
  mutable bool Modified = true;

public:
  virtual ~CellLocatorBase() = default;

  /// @brief Specify the `CellSet` defining the structure of the cells being searched.
  ///
  /// This is typically retrieved from the `viskores::cont::DataSet::GetCellSet()` method.
  VISKORES_CONT const viskores::cont::UnknownCellSet& GetCellSet() const { return this->CellSet; }
  /// @copydoc GetCellSet
  VISKORES_CONT void SetCellSet(const viskores::cont::UnknownCellSet& cellSet)
  {
    this->CellSet = cellSet;
    this->SetModified();
  }

  /// @brief Specify the `CoordinateSystem` defining the location of the cells.
  ///
  /// This is typically retrieved from the `viskores::cont::DataSet::GetCoordinateSystem()` method.
  VISKORES_CONT const viskores::cont::CoordinateSystem& GetCoordinates() const
  {
    return this->Coords;
  }
  /// @copydoc GetCoordinates
  VISKORES_CONT void SetCoordinates(const viskores::cont::CoordinateSystem& coords)
  {
    this->Coords = coords;
    this->SetModified();
  }
  /// @copydoc GetCoordinates
  VISKORES_CONT void SetCoordinates(const viskores::cont::UnknownArrayHandle& coords)
  {
    this->SetCoordinates({ "coords", coords });
  }

  /// @brief Build the search structure used to look up cells.
  ///
  /// This method must be called after the cells and coordiantes are specified with
  /// `SetCellSet()` and `SetCoordinates()`, respectively.
  /// The method must also be called before it is used with a worklet.
  /// Before building the search structure `Update()` checks to see if the structure is
  /// already built and up to date. If so, the method quickly returns.
  /// Thus, it is good practice to call `Update()` before each use in a worklet.
  ///
  /// Although `Update()` is called from the control environment, it lauches jobs in the
  /// execution environment to quickly build the search structure.
  VISKORES_CONT void Update() const;

protected:
  void SetModified() { this->Modified = true; }
  bool GetModified() const { return this->Modified; }

  virtual void Build() = 0;
};

}
} // viskores::cont::internal

#endif //viskores_cont_internal_CellLocatorBase_h

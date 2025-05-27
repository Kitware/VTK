// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef HelixSource_h
#define HelixSource_h

#include "esExoticShapesModule.h" // for export macro

#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN

class ESEXOTICSHAPES_EXPORT HelixSource : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(HelixSource, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static HelixSource* New();

  ///@{
  /**
   * Radius of helix
   */
  vtkSetMacro(Radius, double);
  vtkGetMacro(Radius, double);
  ///@}

  ///@{
  /**
   * Pitch of helix.
   */
  vtkSetMacro(Pitch, double);
  vtkGetMacro(Pitch, double);
  ///@}

  ///@{
  /**
   * no. of turns in helix.
   */
  vtkSetMacro(NumberOfTurns, vtkIdType);
  vtkGetMacro(NumberOfTurns, vtkIdType);
  ///@}

  ///@{
  /**
   * no. of points per turn in helix.
   */
  vtkSetMacro(ResolutionPerTurn, vtkIdType);
  vtkGetMacro(ResolutionPerTurn, vtkIdType);
  ///@}

protected:
  HelixSource();
  ~HelixSource() override;

  int RequestData(
    vtkInformation* info, vtkInformationVector** inInfo, vtkInformationVector* outInfo) override;

private:
  HelixSource(const HelixSource&) = delete;
  void operator=(const HelixSource&) = delete;

  double Radius = 1.0;
  double Pitch = 1.0;

  vtkIdType NumberOfTurns = 1;
  vtkIdType ResolutionPerTurn = 10;
};

VTK_ABI_NAMESPACE_END

#endif

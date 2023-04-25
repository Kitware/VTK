/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridGeometry.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkHyperTreeGridGeometryInternal1D_h
#define vtkHyperTreeGridGeometryInternal1D_h

#include "vtkHyperTreeGridGeometryInternal2D.h"

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
class vtkHyperTreeGridGeometry::vtkInternal1D : public vtkHyperTreeGridGeometry::vtkInternal2D
{
public:
  //----------------------------------------------------------------------------------------------
  vtkInternal1D(std::string _trace, bool _merging_points, vtkHyperTreeGrid* _input,
    vtkPoints* _outputPoints, vtkCellArray* _outputCells,
    vtkDataSetAttributes* _inputCellDataAttributes, vtkDataSetAttributes* _outputCellDataAttributes,
    bool _passThroughCellIds, const std::string& _originalCellIdArrayName);

  //----------------------------------------------------------------------------------------------
  virtual ~vtkInternal1D() override;

private:
  //----------------------------------------------------------------------------------------------
  virtual void processLeafCellWithoutInterface(vtkIdType _inputCellIndex) override;

  //----------------------------------------------------------------------------------------------
  virtual void processLeafCellWithOneInterface(
    vtkIdType _inputCellIndex, double signe, const std::vector<double>& _scalarsInterface) override;

  //----------------------------------------------------------------------------------------------
  virtual void processLeafCellWithDoubleInterface(vtkIdType _inputCellIndex,
    const std::vector<double>& _scalarsInterfaceA,
    const std::vector<double>& _scalarsInterfaceB) override;

  //----------------------------------------------------------------------------------------------
  virtual void buildCellPoints() override;
};

VTK_ABI_NAMESPACE_END
#endif /* vtkHyperTreeGridGeometryInternal1D_h */

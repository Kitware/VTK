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
#ifndef vtkHyperTreeGridGeometryInternal2D_h
#define vtkHyperTreeGridGeometryInternal2D_h

#include "vtkHyperTreeGridGeometryInternal.h"

VTK_ABI_NAMESPACE_BEGIN

#include <vector>

class vtkHyperTreeGridNonOrientedGeometryCursor;

//------------------------------------------------------------------------------
class vtkHyperTreeGridGeometry::vtkInternal2D : public vtkHyperTreeGridGeometry::vtkInternal
{
public:
  vtkInternal2D(std::string _trace, bool _merging_points, vtkHyperTreeGrid* _input,
    vtkPoints* _outputPoints, vtkCellArray* _outputCells,
    vtkDataSetAttributes* _inputCellDataAttributes, vtkDataSetAttributes* _outputCellDataAttributes,
    bool _passThroughCellIds, const std::string& _originalCellIdArrayName);

  //----------------------------------------------------------------------------------------------
  virtual ~vtkInternal2D() override;

protected:
  //----------------------------------------------------------------------------------------------
  unsigned int m_axis_1;
  unsigned int m_axis_2;
  vtkHyperTreeGridNonOrientedGeometryCursor* m_cursor;
  unsigned int m_number_of_children = 0; // depend on branchfactor value
  vtkPoints* m_cell_points;

  //----------------------------------------------------------------------------------------------
  void recursivelyProcessTree();

  //----------------------------------------------------------------------------------------------
  virtual void processLeafCellWithoutInterface(vtkIdType _inputCellIndex);

  //----------------------------------------------------------------------------------------------
  virtual void processLeafCellWithOneInterface(
    vtkIdType _inputCellIndex, double signe, const std::vector<double>& _scalarsInterface);

  //----------------------------------------------------------------------------------------------
  virtual void processLeafCellWithDoubleInterface(vtkIdType _inputCellIndex,
    const std::vector<double>& _scalarsInterfaceA, const std::vector<double>& _scalarsInterfaceB);

  //----------------------------------------------------------------------------------------------
  virtual void buildCellPoints();

  //----------------------------------------------------------------------------------------------
  void processLeafCellWithInterface(vtkIdType _inputCellIndex);
};

VTK_ABI_NAMESPACE_END
#endif /* vtkHyperTreeGridGeometryInternal2D_h */

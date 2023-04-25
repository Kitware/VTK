/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkHyperTreeGridGeometry.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkHyperTreeGridGeometryInternal_h
#define vtkHyperTreeGridGeometryInternal_h

#include "vtkHyperTreeGridGeometry.h"

VTK_ABI_NAMESPACE_BEGIN

#include "vtkIdTypeArray.h"

class vtkDataArray;
class vtkIncrementalPointLocator;
class vtkPoints;
class vtkUnsignedCharArray;

//------------------------------------------------------------------------------
#include <cstdlib>
#include <iostream>
#include <limits>
#include <set>
#include <vector>

extern bool __trace_htg_geometry;
extern double __trace_htg_env_vtkcellid;

#define HAS_TRACE __trace_htg_geometry

#define TRACE(a)                                                                                   \
  if (__trace_htg_geometry)                                                                        \
  {                                                                                                \
    std::cerr << "vtkHyperTreeGridGeometry::" << this->m_trace << "::" << a << std::endl;          \
  }

#define WARNING(a)                                                                                 \
  if (__trace_htg_geometry)                                                                        \
  {                                                                                                \
    std::cerr << "#### WARNING vtkHyperTreeGridGeometry::" << this->m_trace << "::" << a           \
              << std::endl;                                                                        \
  }

#define ERROR(cnd, a)                                                                              \
  if (cnd)                                                                                         \
  {                                                                                                \
    std::cerr << "#### ERROR vtkHyperTreeGridGeometry::" << this->m_trace << "::" << a             \
              << std::endl;                                                                        \
  }

// #define TRACE(a)

//------------------------------------------------------------------------------
class vtkHyperTreeGridGeometry::vtkInternal
{
public:
  //----------------------------------------------------------------------------------------------
  vtkInternal(std::string _trace, bool _merging_points, vtkHyperTreeGrid* _input,
    vtkPoints* _outputPoints, vtkCellArray* _outputCells,
    vtkDataSetAttributes* _inputCellDataAttributes, vtkDataSetAttributes* _outputCellDataAttributes,
    bool _passThroughCellIds, const std::string& _originalCellIdArrayName);

  //----------------------------------------------------------------------------------------------
  virtual ~vtkInternal();

  //----------------------------------------------------------------------------------------------
  bool hasInterfaceOnThisCell() const { return this->m_hasInterfaceOnThisCell; }

  //----------------------------------------------------------------------------------------------
  double getInterfaceTypeOnThisCell() const { return this->m_cell_interface_type; }

  //----------------------------------------------------------------------------------------------
  double getInterfaceInterceptsA() const { return this->m_cell_intercepts[0]; }

  //----------------------------------------------------------------------------------------------
  double getInterfaceInterceptsB() const { return this->m_cell_intercepts[1]; }

  //----------------------------------------------------------------------------------------------
  const double* getInterfaceNormal() const { return this->m_cell_normal.data(); }

  //----------------------------------------------------------------------------------------------
  double computeInterfaceA(const double* xyz) const;

  //----------------------------------------------------------------------------------------------
  double computeInterfaceB(const double* xyz) const;

  //----------------------------------------------------------------------------------------------
  vtkIdType insertPoint(const double* xyz);

  //----------------------------------------------------------------------------------------------
  vtkIdType insertPoint(const std::vector<double>& xyz);

protected:
  //----------------------------------------------------------------------------------------------
  void finish();

  //----------------------------------------------------------------------------------------------
  std::string m_trace;
  bool m_merging_points;
  vtkIncrementalPointLocator* m_locator;
  vtkIdType m_max_id_point = -1;
  vtkHyperTreeGrid* m_input;
  vtkPoints* m_outputPoints;
  vtkCellArray* m_outputCells;
  vtkDataSetAttributes* m_inputCellDataAttributes;
  vtkDataSetAttributes* m_outputCellDataAttributes;
  vtkUnsignedCharArray* m_inGhostArray;
  vtkBitArray* m_inMaskArray;
  bool m_hasInterface;
  bool m_orientation;
  bool m_hasInterfaceOnThisCell;
  double m_cell_interface_type = 2.; // pure cell

  //----------------------------------------------------------------------------------------------
  void createNewCellAndCopyData(
    const std::vector<vtkIdType>& _outputIndexPoints, vtkIdType _inputCellIndex);

  //----------------------------------------------------------------------------------------------
  bool isMaskedOrGhosted(vtkIdType _global_node_index) const;

  //----------------------------------------------------------------------------------------------
  bool extractCellInterface(vtkIdType _inputCellIndex, bool _with_inversion = true);

  //----------------------------------------------------------------------------------------------
  // pour savoir si la cellule voisine est pure ou non
  bool hasInterface(vtkIdType _inputCellIndex) const;

private:
  vtkDataArray* m_inputIntercepts;
  vtkDataArray* m_inputNormals;
  vtkIdTypeArray* m_outputOriginalVtkCellLocalIdOnServer;

  std::vector<double> m_cell_intercepts;
  std::vector<double> m_cell_normal;
};

VTK_ABI_NAMESPACE_END
#endif /* vtkHyperTreeGridGeometryInternal_h */

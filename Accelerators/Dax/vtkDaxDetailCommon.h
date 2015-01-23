//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2012 Sandia Corporation.
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//=============================================================================

#ifndef vtkDaxDetailCommon_h
#define vtkDaxDetailCommon_h

#include "vtkCellTypes.h"
#include "vtkDataSet.h"
#include "vtkGenericCell.h"
#include "vtkNew.h"

namespace vtkDax {
namespace detail {
  struct CellTypeInDataSet
    {
    explicit CellTypeInDataSet(int cellType):
      Cell(vtkGenericCell::InstantiateCell(cellType)){}
    ~CellTypeInDataSet(){this->Cell->Delete();}
    vtkCell* Cell;
    };

  //returns if a dataset can be used from within Dax
  inline CellTypeInDataSet cellType(vtkDataSet* input)
  {
    //determine the cell types that the dataset has
    vtkNew<vtkCellTypes> cellTypes;
    input->GetCellTypes(cellTypes.GetPointer());

    if(cellTypes->GetNumberOfTypes() > 1)
      {
      //we currently only support a single cell type
      return CellTypeInDataSet(VTK_EMPTY_CELL);
      }

    return CellTypeInDataSet(cellTypes->GetCellType(0));
  }
}}
#endif // DaxDetailCommon_h
// VTK-HeaderTest-Exclude: vtkDaxDetailCommon.h

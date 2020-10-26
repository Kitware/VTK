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

#ifndef vtkmFilterPolicy_h
#define vtkmFilterPolicy_h
#ifndef __VTK_WRAP__
#ifndef VTK_WRAPPING_CXX

#include "vtkmConfigDataModel.h" //required for general vtkm setup

#include <vtkm/List.h>
#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/CellSetPermutation.h>
#include <vtkm/cont/CellSetSingleType.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/DefaultTypes.h>
#include <vtkm/filter/PolicyDefault.h>

//------------------------------------------------------------------------------
class vtkmOutputFilterPolicy : public vtkm::filter::PolicyBase<vtkmOutputFilterPolicy>
{
public:
  using FieldTypeList = tovtkm::FieldTypeOutVTK;

  using StructuredCellSetList = tovtkm::CellListStructuredOutVTK;
  using UnstructuredCellSetList = tovtkm::CellListUnstructuredOutVTK;
  using AllCellSetList = tovtkm::CellListAllOutVTK;
};

#endif
#endif
#endif
// VTK-HeaderTest-Exclude: vtkmFilterPolicy.h

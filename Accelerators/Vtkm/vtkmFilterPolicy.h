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

#include "vtkmConfig.h" //required for general vtkm setup
#include "vtkmTags.h"

#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/CellSetPermutation.h>
#include <vtkm/cont/CellSetSingleType.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/filter/PolicyDefault.h>

// Forward declaration of types.
namespace vtkm
{
namespace cont
{
class vtkmCellSetExplicitAOS;
class vtkmCellSetSingleType;
}
}

namespace tovtkm
{

//------------------------------------------------------------------------------
// All scalar types in vtkType.h
struct VTKScalarTypes
  : vtkm::ListTagBase<char,
                      signed char,
                      unsigned char,
                      short,
                      unsigned short,
                      int,
                      unsigned int,
                      long,
                      unsigned long,
                      long long,
                      unsigned long long,
                      float,
                      double>
{
};

struct SpecialGradientOutTypes
    : vtkm::ListTagBase<
                        vtkm::Vec< vtkm::Vec<vtkm::Float32,3>, 3>,
                        vtkm::Vec< vtkm::Vec<vtkm::Float64,3>, 3>
                        >
{
};

struct FieldTypeInVTK
    : vtkm::ListTagJoin<
        vtkm::TypeListTagVecCommon,
        VTKScalarTypes>
{
};

struct FieldTypeOutVTK
    : vtkm::ListTagJoin<
        vtkm::ListTagJoin<
          vtkm::TypeListTagVecCommon,
          SpecialGradientOutTypes
          >,
        VTKScalarTypes
      >
{
};

//------------------------------------------------------------------------------
struct CellListStructuredInVTK
    : vtkm::ListTagBase<vtkm::cont::CellSetStructured<3>, vtkm::cont::CellSetStructured<2>>
{
};
struct CellListStructuredOutVTK
    : vtkm::ListTagBase<
          vtkm::cont::CellSetPermutation<vtkm::cont::CellSetStructured<3>>,
          vtkm::cont::CellSetPermutation<vtkm::cont::CellSetStructured<2>> >
{
};

//------------------------------------------------------------------------------
struct CellListUnstructuredInVTK
    : vtkm::ListTagBase<vtkm::cont::vtkmCellSetExplicitAOS,
                        vtkm::cont::vtkmCellSetSingleType>
{
};
struct CellListUnstructuredOutVTK
    : vtkm::ListTagBase<
          vtkm::cont::CellSetExplicit<>, vtkm::cont::CellSetSingleType<>,
          vtkm::cont::vtkmCellSetExplicitAOS, vtkm::cont::vtkmCellSetSingleType,
          vtkm::cont::CellSetPermutation<vtkm::cont::vtkmCellSetExplicitAOS>,
          vtkm::cont::CellSetPermutation<vtkm::cont::vtkmCellSetSingleType>>
{
};

//------------------------------------------------------------------------------
struct CellListAllInVTK
    : vtkm::ListTagJoin<CellListStructuredInVTK, CellListUnstructuredInVTK>
{
};
struct CellListAllOutVTK
    : vtkm::ListTagJoin<CellListStructuredOutVTK, CellListUnstructuredOutVTK>
{
};
}

//------------------------------------------------------------------------------
class vtkmInputFilterPolicy
    : public vtkm::filter::PolicyBase<vtkmInputFilterPolicy>
{
public:
  using FieldTypeList = tovtkm::FieldTypeInVTK;

  using StructuredCellSetList = tovtkm::CellListStructuredInVTK;
  using UnstructuredCellSetList = tovtkm::CellListUnstructuredInVTK;
  using AllCellSetList = tovtkm::CellListAllInVTK;

};

//------------------------------------------------------------------------------
class vtkmOutputFilterPolicy
    : public vtkm::filter::PolicyBase<vtkmOutputFilterPolicy>
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

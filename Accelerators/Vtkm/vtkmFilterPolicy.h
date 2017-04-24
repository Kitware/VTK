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
        vtkm::TypeListTagScalarAll>
{
};

struct FieldTypeOutVTK
    : vtkm::ListTagJoin<
        vtkm::ListTagJoin<
          vtkm::TypeListTagVecCommon,
          SpecialGradientOutTypes
          >,
        vtkm::TypeListTagScalarAll
      >
{
};

//------------------------------------------------------------------------------
struct ArrayListInVTK : vtkm::ListTagBase<
#if defined(VTKM_FILTER_INCLUDE_AOS)
                                          tovtkm::vtkAOSArrayContainerTag
#endif
#if defined(VTKM_FILTER_INCLUDE_SOA)
                                          ,tovtkm::vtkSOAArrayContainerTag
#endif
                                          >
{
};

// Currently vtk-m doesn't offer an easy way to auto generate all these
// permutation
// tag types that are needed to handle the output of the Threshold algorithm
struct TypeListTagPermutationVecCommon
    : vtkm::ListTagBase<
          vtkm::cont::internal::StorageTagPermutation<
              vtkm::cont::ArrayHandle<vtkm::Id>, vtkm::Vec<vtkm::UInt8, 2>>,
          vtkm::cont::internal::StorageTagPermutation<
              vtkm::cont::ArrayHandle<vtkm::Id>, vtkm::Vec<vtkm::Int32, 2>>,
          vtkm::cont::internal::StorageTagPermutation<
              vtkm::cont::ArrayHandle<vtkm::Id>, vtkm::Vec<vtkm::Int64, 2>>,
          vtkm::cont::internal::StorageTagPermutation<
              vtkm::cont::ArrayHandle<vtkm::Id>, vtkm::Vec<vtkm::Float32, 2>>,
          vtkm::cont::internal::StorageTagPermutation<
              vtkm::cont::ArrayHandle<vtkm::Id>, vtkm::Vec<vtkm::Float64, 2>>,
          vtkm::cont::internal::StorageTagPermutation<
              vtkm::cont::ArrayHandle<vtkm::Id>, vtkm::Vec<vtkm::UInt8, 3>>,
          vtkm::cont::internal::StorageTagPermutation<
              vtkm::cont::ArrayHandle<vtkm::Id>, vtkm::Vec<vtkm::Int32, 3>>,
          vtkm::cont::internal::StorageTagPermutation<
              vtkm::cont::ArrayHandle<vtkm::Id>, vtkm::Vec<vtkm::Int64, 3>>,
          vtkm::cont::internal::StorageTagPermutation<
              vtkm::cont::ArrayHandle<vtkm::Id>, vtkm::Vec<vtkm::Float32, 3>>,
          vtkm::cont::internal::StorageTagPermutation<
              vtkm::cont::ArrayHandle<vtkm::Id>, vtkm::Vec<vtkm::Float64, 3>>,
          vtkm::cont::internal::StorageTagPermutation<
              vtkm::cont::ArrayHandle<vtkm::Id>, vtkm::Vec<vtkm::UInt8, 4>>,
          vtkm::cont::internal::StorageTagPermutation<
              vtkm::cont::ArrayHandle<vtkm::Id>, vtkm::Vec<vtkm::Int32, 4>>,
          vtkm::cont::internal::StorageTagPermutation<
              vtkm::cont::ArrayHandle<vtkm::Id>, vtkm::Vec<vtkm::Int64, 4>>,
          vtkm::cont::internal::StorageTagPermutation<
              vtkm::cont::ArrayHandle<vtkm::Id>, vtkm::Vec<vtkm::Float32, 4>>,
          vtkm::cont::internal::StorageTagPermutation<
              vtkm::cont::ArrayHandle<vtkm::Id>, vtkm::Vec<vtkm::Float64, 4>>>
{
};

struct TypeListTagPermutationScalarAll
    : vtkm::ListTagBase<vtkm::cont::internal::StorageTagPermutation<
                            vtkm::cont::ArrayHandle<vtkm::Id>, vtkm::Int8>,
                        vtkm::cont::internal::StorageTagPermutation<
                            vtkm::cont::ArrayHandle<vtkm::Id>, vtkm::UInt8>,
                        vtkm::cont::internal::StorageTagPermutation<
                            vtkm::cont::ArrayHandle<vtkm::Id>, vtkm::Int16>,
                        vtkm::cont::internal::StorageTagPermutation<
                            vtkm::cont::ArrayHandle<vtkm::Id>, vtkm::UInt16>,
                        vtkm::cont::internal::StorageTagPermutation<
                            vtkm::cont::ArrayHandle<vtkm::Id>, vtkm::Int32>,
                        vtkm::cont::internal::StorageTagPermutation<
                            vtkm::cont::ArrayHandle<vtkm::Id>, vtkm::UInt32>,
                        vtkm::cont::internal::StorageTagPermutation<
                            vtkm::cont::ArrayHandle<vtkm::Id>, vtkm::Int64>,
                        vtkm::cont::internal::StorageTagPermutation<
                            vtkm::cont::ArrayHandle<vtkm::Id>, vtkm::UInt64>,
                        vtkm::cont::internal::StorageTagPermutation<
                            vtkm::cont::ArrayHandle<vtkm::Id>, vtkm::Float32>,
                        vtkm::cont::internal::StorageTagPermutation<
                            vtkm::cont::ArrayHandle<vtkm::Id>, vtkm::Float64>>
{
};

struct TypeListTagPermutationVTK
    : vtkm::ListTagJoin<tovtkm::TypeListTagPermutationVecCommon,
                        tovtkm::TypeListTagPermutationScalarAll>
{
};

struct TypeListTagVTMOut : vtkm::ListTagBase<vtkm::cont::StorageTagBasic
#if defined(VTKM_FILTER_INCLUDE_AOS)
                                          ,tovtkm::vtkAOSArrayContainerTag
#endif
#if defined(VTKM_FILTER_INCLUDE_SOA)
                                          ,tovtkm::vtkSOAArrayContainerTag
#endif
                                          >
{
};

struct ArrayListOutVTK
    : vtkm::ListTagJoin<TypeListTagVTMOut, TypeListTagPermutationVTK>
{
};

//------------------------------------------------------------------------------
struct PointListInVTK
    : vtkm::ListTagBase<
          vtkm::cont::ArrayHandleUniformPointCoordinates::StorageTag,
          tovtkm::vtkAOSArrayContainerTag
#if defined(VTKM_FILTER_INCLUDE_SOA)
          ,
          tovtkm::vtkSOAArrayContainerTag
#endif
          >
{
};
struct PointListOutVTK
    : vtkm::ListTagBase<
          vtkm::cont::ArrayHandleUniformPointCoordinates::StorageTag,
          vtkm::cont::StorageTagBasic,
          tovtkm::vtkAOSArrayContainerTag>
{
};

//------------------------------------------------------------------------------
struct CellListStructuredInVTK
    : vtkm::ListTagBase<vtkm::cont::CellSetStructured<3>>
{
};
struct CellListStructuredOutVTK
    : vtkm::ListTagBase<
          vtkm::cont::CellSetPermutation<vtkm::cont::CellSetStructured<3>>>
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
  typedef tovtkm::FieldTypeInVTK FieldTypeList;
  typedef tovtkm::ArrayListInVTK FieldStorageList;

  typedef tovtkm::CellListStructuredInVTK StructuredCellSetList;
  typedef tovtkm::CellListUnstructuredInVTK UnstructuredCellSetList;
  typedef tovtkm::CellListAllInVTK AllCellSetList;

  typedef vtkm::TypeListTagFieldVec3 CoordinateTypeList;
  typedef tovtkm::PointListInVTK CoordinateStorageList;

  typedef vtkm::filter::PolicyDefault::DeviceAdapterList DeviceAdapterList;
};

//------------------------------------------------------------------------------
class vtkmOutputFilterPolicy
    : public vtkm::filter::PolicyBase<vtkmOutputFilterPolicy>
{
public:
  typedef tovtkm::FieldTypeOutVTK FieldTypeList;
  typedef tovtkm::ArrayListOutVTK FieldStorageList;

  typedef tovtkm::CellListStructuredOutVTK StructuredCellSetList;
  typedef tovtkm::CellListUnstructuredOutVTK UnstructuredCellSetList;
  typedef tovtkm::CellListAllOutVTK AllCellSetList;

  typedef vtkm::TypeListTagFieldVec3 CoordinateTypeList;
  typedef tovtkm::PointListOutVTK CoordinateStorageList;

  typedef vtkm::filter::PolicyDefault::DeviceAdapterList DeviceAdapterList;
};

#endif
// VTK-HeaderTest-Exclude: vtkmFilterPolicy.h

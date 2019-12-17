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

#include <vtkm/List.h>
#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/CellSetPermutation.h>
#include <vtkm/cont/CellSetSingleType.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/filter/PolicyDefault.h>

namespace tovtkm
{

//------------------------------------------------------------------------------
// All scalar types in vtkType.h
using VTKScalarTypes = vtkm::List< //
  char,                            //
  signed char,                     //
  unsigned char,                   //
  short,                           //
  unsigned short,                  //
  int,                             //
  unsigned int,                    //
  long,                            //
  unsigned long,                   //
  long long,                       //
  unsigned long long,              //
  float,                           //
  double                           //
  >;

using SpecialGradientOutTypes =
  vtkm::List<vtkm::Vec<vtkm::Vec<vtkm::Float32, 3>, 3>, vtkm::Vec<vtkm::Vec<vtkm::Float64, 3>, 3> >;

using FieldTypeInVTK = vtkm::ListAppend<vtkm::TypeListVecCommon, VTKScalarTypes>;

using FieldTypeOutVTK =
  vtkm::ListAppend<vtkm::TypeListVecCommon, SpecialGradientOutTypes, VTKScalarTypes>;

//------------------------------------------------------------------------------
using CellListStructuredInVTK =
  vtkm::List<vtkm::cont::CellSetStructured<3>, vtkm::cont::CellSetStructured<2> >;
using CellListStructuredOutVTK =
  vtkm::List<vtkm::cont::CellSetPermutation<vtkm::cont::CellSetStructured<3> >,
    vtkm::cont::CellSetPermutation<vtkm::cont::CellSetStructured<2> > >;

// vtkCellArray may use either 32 or 64 bit arrays to hold connectivity/offset
// data, so we may be using ArrayHandleCast to convert to vtkm::Ids.
#ifdef VTKM_USE_64BIT_IDS
using Int32AOSHandle = vtkm::cont::ArrayHandle<vtkTypeInt32>;
using Int32AsIdAOSHandle = vtkm::cont::ArrayHandleCast<vtkm::Id, Int32AOSHandle>;
using Int32AsIdAOSStorage = typename Int32AsIdAOSHandle::StorageTag;

using CellSetExplicit32Bit = vtkm::cont::CellSetExplicit<vtkm::cont::StorageTagBasic,
  Int32AsIdAOSStorage, Int32AsIdAOSStorage>;
using CellSetExplicit64Bit = vtkm::cont::CellSetExplicit<vtkm::cont::StorageTagBasic,
  vtkm::cont::StorageTagBasic, vtkm::cont::StorageTagBasic>;
using CellSetSingleType32Bit = vtkm::cont::CellSetSingleType<Int32AsIdAOSStorage>;
using CellSetSingleType64Bit = vtkm::cont::CellSetSingleType<vtkm::cont::StorageTagBasic>;
#else  // VTKM_USE_64BIT_IDS
using Int64AOSHandle = vtkm::cont::ArrayHandle<vtkTypeInt64, vtkm::cont::StorageTagBasic>;
using Int64AsIdAOSHandle = vtkm::cont::ArrayHandleCast<vtkm::Id, Int64AOSHandle>;
using Int64AsIdAOSStorage = typename Int64AsIdAOSHandle::StorageTag;

using CellSetExplicit32Bit = vtkm::cont::CellSetExplicit<vtkm::cont::StorageTagBasic,
  vtkm::cont::StorageTagBasic, vtkm::cont::StorageTagBasic>;
using CellSetExplicit64Bit = vtkm::cont::CellSetExplicit<vtkm::cont::StorageTagBasic,
  Int64AsIdAOSStorage, Int64AsIdAOSStorage>;
using CellSetSingleType32Bit = vtkm::cont::CellSetSingleType<vtkm::cont::StorageTagBasic>;
using CellSetSingleType64Bit = vtkm::cont::CellSetSingleType<Int64AsIdAOSStorage>;
#endif // VTKM_USE_64BIT_IDS

//------------------------------------------------------------------------------
using CellListUnstructuredInVTK = vtkm::List< //
  CellSetExplicit32Bit,                       //
  CellSetExplicit64Bit,                       //
  CellSetSingleType32Bit,                     //
  CellSetSingleType64Bit                      //
  >;

using CellListUnstructuredOutVTK = vtkm::List<                     //
  vtkm::cont::CellSetExplicit<>,                                   //
  vtkm::cont::CellSetSingleType<>,                                 //
  CellSetExplicit32Bit,                                            //
  CellSetExplicit64Bit,                                            //
  CellSetSingleType32Bit,                                          //
  CellSetSingleType64Bit,                                          //
  vtkm::cont::CellSetPermutation<CellSetExplicit32Bit>,            //
  vtkm::cont::CellSetPermutation<CellSetExplicit64Bit>,            //
  vtkm::cont::CellSetPermutation<CellSetSingleType32Bit>,          //
  vtkm::cont::CellSetPermutation<CellSetSingleType64Bit>,          //
  vtkm::cont::CellSetPermutation<vtkm::cont::CellSetExplicit<> >,  //
  vtkm::cont::CellSetPermutation<vtkm::cont::CellSetSingleType<> > //
  >;

//------------------------------------------------------------------------------
using CellListAllInVTK = vtkm::ListAppend<CellListStructuredInVTK, CellListUnstructuredInVTK>;
using CellListAllOutVTK = vtkm::ListAppend<CellListStructuredOutVTK, CellListUnstructuredOutVTK>;

} // end namespace tovtkm

//------------------------------------------------------------------------------
class vtkmInputFilterPolicy : public vtkm::filter::PolicyBase<vtkmInputFilterPolicy>
{
public:
  using FieldTypeList = tovtkm::FieldTypeInVTK;

  using StructuredCellSetList = tovtkm::CellListStructuredInVTK;
  using UnstructuredCellSetList = tovtkm::CellListUnstructuredInVTK;
  using AllCellSetList = tovtkm::CellListAllInVTK;
};

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

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtkmConnectivityExec_h
#define vtkmConnectivityExec_h

#include "vtkmTags.h"

#include <vtkm/CellShape.h>
#include <vtkm/TopologyElementTag.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/VecFromPortal.h>

#include <vtkm/cont/serial/DeviceAdapterSerial.h>
#include <vtkm/cont/cuda/DeviceAdapterCuda.h>
#include <vtkm/cont/tbb/DeviceAdapterTBB.h>

namespace vtkm {
namespace exec {

template <typename Device> class VTKM_ALWAYS_EXPORT ConnectivityVTKAOS
{
  typedef vtkm::cont::ArrayHandle<vtkm::UInt8, tovtkm::vtkAOSArrayContainerTag>
      ShapeHandleType;
  typedef vtkm::cont::ArrayHandle<vtkm::Id, tovtkm::vtkCellArrayContainerTag>
      ConnectivityHandleType;
  typedef vtkm::cont::ArrayHandle<vtkm::Id, tovtkm::vtkAOSArrayContainerTag>
      OffsetHandleType;

  typedef typename ShapeHandleType::template ExecutionTypes<Device>::PortalConst
      ShapePortalType;
  typedef typename ConnectivityHandleType::template ExecutionTypes<
      Device>::PortalConst ConnectivityPortalType;
  typedef
      typename OffsetHandleType::template ExecutionTypes<Device>::PortalConst
          IndexOffsetPortalType;

public:
  typedef typename vtkm::Id SchedulingRangeType;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ConnectivityVTKAOS();

  VTKM_EXEC_CONT
  ConnectivityVTKAOS(const ShapePortalType& shapePortal,
                     const ConnectivityPortalType& connPortal,
                     const IndexOffsetPortalType& indexOffsetPortal);

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  vtkm::Id GetNumberOfElements() const;

  typedef vtkm::CellShapeTagGeneric CellShapeTag;

  VTKM_EXEC
  CellShapeTag GetCellShape(vtkm::Id index) const;

  typedef vtkm::VecFromPortal<ConnectivityPortalType> IndicesType;

  /// Returns a Vec-like object containing the indices for the given index.
  /// The object returned is not an actual array, but rather an object that
  /// loads the indices lazily out of the connectivity array. This prevents
  /// us from having to know the number of indices at compile time.
  ///
  VTKM_EXEC
  IndicesType GetIndices(vtkm::Id index) const;

private:
  ShapePortalType Shapes;
  ConnectivityPortalType Connectivity;
  IndexOffsetPortalType IndexOffsets;
};


template <typename Device> class VTKM_ALWAYS_EXPORT ConnectivityVTKSingleType
{
  typedef vtkm::cont::ArrayHandle<vtkm::Id, tovtkm::vtkCellArrayContainerTag>
      ConnectivityHandleType;
  typedef typename ConnectivityHandleType::template ExecutionTypes<
      Device>::PortalConst ConnectivityPortalType;

public:
  typedef typename vtkm::Id SchedulingRangeType;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ConnectivityVTKSingleType();

  VTKM_EXEC_CONT
  ConnectivityVTKSingleType(const ConnectivityPortalType& connPortal,
                            vtkm::Id numCells,
                            vtkm::IdComponent numPointsPerCell,
                            vtkm::UInt8 shapeType);

  VTKM_EXEC
  vtkm::Id GetNumberOfElements() const;

  typedef vtkm::CellShapeTagGeneric CellShapeTag;

  VTKM_EXEC
  CellShapeTag GetCellShape(vtkm::Id index) const;

  typedef vtkm::VecFromPortal<ConnectivityPortalType> IndicesType;

  /// Returns a Vec-like object containing the indices for the given index.
  /// The object returned is not an actual array, but rather an object that
  /// loads the indices lazily out of the connectivity array. This prevents
  /// us from having to know the number of indices at compile time.
  ///
  VTKM_EXEC
  IndicesType GetIndices(vtkm::Id index) const;

private:
  ConnectivityPortalType Connectivity;
  vtkm::Id NumberOfCells;
  vtkm::IdComponent NumberOfPointsPerCell;
  vtkm::UInt8 ShapeType;
};


template <typename Device> class VTKM_ALWAYS_EXPORT ReverseConnectivityVTK
{
  typedef vtkm::cont::ArrayHandle<vtkm::Id> ConnectivityHandleType;
  typedef vtkm::cont::ArrayHandle<vtkm::IdComponent> NumIndicesHandleType;
  typedef vtkm::cont::ArrayHandle<vtkm::Id> OffsetHandleType;

  typedef typename ConnectivityHandleType::template ExecutionTypes<
      Device>::PortalConst ConnectivityPortalType;

  typedef typename OffsetHandleType::template ExecutionTypes<Device>::PortalConst
          IndexOffsetPortalType;

  typedef typename NumIndicesHandleType::template ExecutionTypes<Device>::PortalConst
          NumIndicesPortalType;

public:
  typedef typename vtkm::Id SchedulingRangeType;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ReverseConnectivityVTK();

  VTKM_EXEC_CONT
  ReverseConnectivityVTK(const ConnectivityPortalType& connPortal,
                            const NumIndicesPortalType& numIndicesPortal,
                            const IndexOffsetPortalType& indexOffsetPortal);

  VTKM_EXEC
  vtkm::Id GetNumberOfElements() const;

  typedef vtkm::CellShapeTagVertex CellShapeTag;

  VTKM_EXEC
  CellShapeTag GetCellShape(vtkm::Id) const { return vtkm::CellShapeTagVertex(); }

  typedef vtkm::VecFromPortal<ConnectivityPortalType> IndicesType;

  /// Returns a Vec-like object containing the indices for the given index.
  /// The object returned is not an actual array, but rather an object that
  /// loads the indices lazily out of the connectivity array. This prevents
  /// us from having to know the number of indices at compile time.
  ///
  VTKM_EXEC
  IndicesType GetIndices(vtkm::Id index) const;

private:
  ConnectivityPortalType Connectivity;
  NumIndicesPortalType NumIndices;
  IndexOffsetPortalType IndexOffsets;
};

// template methods we want to compile only once
extern template class VTKACCELERATORSVTKM_TEMPLATE_EXPORT ConnectivityVTKAOS<vtkm::cont::DeviceAdapterTagSerial>;
extern template class VTKACCELERATORSVTKM_TEMPLATE_EXPORT ConnectivityVTKSingleType<vtkm::cont::DeviceAdapterTagSerial>;
extern template class VTKACCELERATORSVTKM_TEMPLATE_EXPORT ReverseConnectivityVTK<vtkm::cont::DeviceAdapterTagSerial>;

#ifdef VTKM_ENABLE_TBB
extern template class VTKACCELERATORSVTKM_TEMPLATE_EXPORT ConnectivityVTKAOS<vtkm::cont::DeviceAdapterTagTBB>;
extern template class VTKACCELERATORSVTKM_TEMPLATE_EXPORT ConnectivityVTKSingleType<vtkm::cont::DeviceAdapterTagTBB>;
extern template class VTKACCELERATORSVTKM_TEMPLATE_EXPORT ReverseConnectivityVTK<vtkm::cont::DeviceAdapterTagTBB>;
#endif

//only when cuda is enabled, and the compiler is cuda
#if defined(VTKM_ENABLE_CUDA) && defined(VTKM_CUDA)
extern template class VTKACCELERATORSVTKM_TEMPLATE_EXPORT ConnectivityVTKAOS<vtkm::cont::DeviceAdapterTagCuda>;
extern template class VTKACCELERATORSVTKM_TEMPLATE_EXPORT ConnectivityVTKSingleType<vtkm::cont::DeviceAdapterTagCuda>;
extern template class VTKACCELERATORSVTKM_TEMPLATE_EXPORT ReverseConnectivityVTK<vtkm::cont::DeviceAdapterTagCuda>;
#endif
}
}


#endif
// VTK-HeaderTest-Exclude: vtkmConnectivityExec.h

// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkConduitArrayUtilitiesDevice.h"

#include "vtkArrayDispatch.h"
#include "vtkCellArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkDeviceMemoryType.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkSOADataArrayTemplate.h"
#include "vtkSetGet.h"
#include "vtkTypeFloat32Array.h"
#include "vtkTypeFloat64Array.h"
#include "vtkTypeInt16Array.h"
#include "vtkTypeInt32Array.h"
#include "vtkTypeInt64Array.h"
#include "vtkTypeInt8Array.h"
#include "vtkTypeUInt16Array.h"
#include "vtkTypeUInt32Array.h"
#include "vtkTypeUInt64Array.h"
#include "vtkTypeUInt8Array.h"

#include "vtkm/CellShape.h"
#include "vtkm/cont/ArrayHandle.h"
#include "vtkm/cont/ArrayHandleCast.h"
#include "vtkm/cont/CellSetSingleType.h"
#include "vtkmDataArray.h"
#include "vtkmlib/CellSetConverters.h"
#if defined(VTK_USE_CUDA)
#include <cuda_runtime_api.h>
#endif // VTK_USE_CUDA

#include <catalyst_conduit.hpp>
#include <catalyst_conduit_blueprint.hpp>

#include <type_traits>
#include <typeinfo>
#include <vector>

namespace internals
{
VTK_ABI_NAMESPACE_BEGIN

//----------------------------------------------------------------------------
conduit_cpp::DataType::Id GetTypeId(conduit_cpp::DataType::Id type, bool force_signed)
{
  if (!force_signed)
  {
    return type;
  }
  switch (type)
  {
    case conduit_cpp::DataType::Id::uint8:
      return conduit_cpp::DataType::Id::int8;

    case conduit_cpp::DataType::Id::uint16:
      return conduit_cpp::DataType::Id::int16;

    case conduit_cpp::DataType::Id::uint32:
      return conduit_cpp::DataType::Id::int32;

    case conduit_cpp::DataType::Id::uint64:
      return conduit_cpp::DataType::Id::int64;

    default:
      return type;
  }
}

VTK_ABI_NAMESPACE_END
} // internals

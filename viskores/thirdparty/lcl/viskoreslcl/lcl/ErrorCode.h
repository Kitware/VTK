//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.md for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef lcl_ErrorCode_h
#define lcl_ErrorCode_h

#include <lcl/internal/Config.h>

#include <cstdint>

namespace lcl
{

enum class ErrorCode : std::int32_t
{
  SUCCESS = 0,
  INVALID_SHAPE_ID,
  INVALID_NUMBER_OF_POINTS,
  WRONG_SHAPE_ID_FOR_TAG_TYPE,
  INVALID_POINT_ID,
  SOLUTION_DID_NOT_CONVERGE,
  MATRIX_LUP_FACTORIZATION_FAILED,
  DEGENERATE_CELL_DETECTED
};

LCL_EXEC
inline const char* errorString(ErrorCode code) noexcept
{
  switch (code)
  {
    case ErrorCode::SUCCESS:
      return "Success";
    case ErrorCode::INVALID_SHAPE_ID:
      return "Invalid shape id";
    case ErrorCode::INVALID_NUMBER_OF_POINTS:
      return "Invalid number of points";
    case ErrorCode::WRONG_SHAPE_ID_FOR_TAG_TYPE:
      return "Wrong shape id for tag type";
    case ErrorCode::INVALID_POINT_ID:
      return "Invalid point id";
    case ErrorCode::SOLUTION_DID_NOT_CONVERGE:
      return "Solution did not converge";
    case ErrorCode::MATRIX_LUP_FACTORIZATION_FAILED:
      return "LUP factorization failed";
    case ErrorCode::DEGENERATE_CELL_DETECTED:
      return "Degenerate cell detected";
  }

  return "Invalid error";
}

} // lcl

#define LCL_RETURN_ON_ERROR(call)                                                                 \
  {                                                                                                \
    auto status = call;                                                                            \
    if (status != lcl::ErrorCode::SUCCESS)                                                        \
    {                                                                                              \
      return status;                                                                               \
    }                                                                                              \
  }

#endif // lcl_ErrorCode_h

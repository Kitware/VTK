// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkONNXInferenceInternals
 * @brief VTK internal class for hiding ONNX members
 *
 * VTK internal class to hide ONNX members. Designed to be used by
 * vtkONNXInference.
 */
#ifndef vtkONNXInferenceInternals_h
#define vtkONNXInferenceInternals_h

#include <memory> // For unique_ptr
#include <onnxruntime_cxx_api.h>

VTK_ABI_NAMESPACE_BEGIN
struct vtkONNXInferenceInternals
{
  Ort::Env OrtEnv;
  std::unique_ptr<Ort::Session> Session;
};

VTK_ABI_NAMESPACE_END
#endif // vtkONNXInferenceInternals_h

// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAOSDataArrayTemplate.h"
#include "vtkAffineArray.h"
#include "vtkConstantArray.h"
#include "vtkDeserializer.h"
#include "vtkInvoker.h"
#include "vtkMarshalContext.h"
#include "vtkSerializer.h"

#include <iostream>

int TestDataArraySerDesDemangledNames(int, char*[])
{
  vtkNew<vtkMarshalContext> context;
  vtkNew<vtkSerializer> serializer;
  vtkNew<vtkDeserializer> deserializer; // not used in this test, but needed to call registrars
  vtkNew<vtkInvoker> invoker;           // same as above
  serializer->SetContext(context);

  bool success = true;
  if (const char* error = nullptr;
      !context->CallRegistrars(serializer, deserializer, invoker, &error))
  {
    std::cerr << "Failed to call registrars: " << error << std::endl;
    success &= false;
  }

  vtkNew<vtkAOSDataArrayTemplate<int>> aosArray;
  const auto aosArrayResult = serializer->SerializeJSON(aosArray);
  if (auto idIt = aosArrayResult.find("Id");
      idIt != aosArrayResult.end() && idIt->is_number_unsigned())
  {
    const auto state = context->GetState(idIt.value());
    if (auto classNameIt = state.find("ClassName"); classNameIt != state.end())
    {
      std::string className = classNameIt->get<std::string>();
      if (className != "vtkAOSDataArrayTemplate<int>")
      {
        std::cerr << "Expected vtkAOSDataArrayTemplate<int> but got " << className << std::endl;
        success &= false;
      }
    }
    else
    {
      std::cerr << "ClassName not found in serialized state." << std::endl;
      success &= false;
    }
  }
  else
  {
    std::cerr << "Id not found in serialized state." << std::endl;
    success &= false;
  }

  vtkNew<vtkAffineArray<double>> affineArray;
  affineArray->SetBackend(std::make_shared<vtkAffineImplicitBackend<double>>(2.0, 1.0));
  const auto affineArrayResult = serializer->SerializeJSON(affineArray);
  if (auto idIt = affineArrayResult.find("Id");
      idIt != affineArrayResult.end() && idIt->is_number_unsigned())
  {
    const auto state = context->GetState(idIt.value());
    if (auto classNameIt = state.find("ClassName"); classNameIt != state.end())
    {
      std::string className = classNameIt->get<std::string>();
      if (className != "vtkAffineArray<double>")
      {
        std::cerr << "Expected vtkAffineArray<double> but got " << className << std::endl;
        success &= false;
      }
    }
    else
    {
      std::cerr << "ClassName not found in serialized state." << std::endl;
      success &= false;
    }
  }
  else
  {
    std::cerr << "Id not found in serialized state." << std::endl;
    success &= false;
  }

  vtkNew<vtkConstantArray<float>> constantArray;
  constantArray->SetBackend(std::make_shared<vtkConstantImplicitBackend<float>>(3.14f));
  const auto constantArrayResult = serializer->SerializeJSON(constantArray);
  if (auto idIt = constantArrayResult.find("Id");
      idIt != constantArrayResult.end() && idIt->is_number_unsigned())
  {
    const auto state = context->GetState(idIt.value());
    if (auto classNameIt = state.find("ClassName"); classNameIt != state.end())
    {
      std::string className = classNameIt->get<std::string>();
      if (className != "vtkConstantArray<float>")
      {
        std::cerr << "Expected vtkConstantArray<float> but got " << className << std::endl;
        success &= false;
      }
    }
    else
    {
      std::cerr << "ClassName not found in serialized state." << std::endl;
      success &= false;
    }
  }
  else
  {
    std::cerr << "Id not found in serialized state." << std::endl;
    success &= false;
  }
  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}

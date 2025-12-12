// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "TestExerciseMultiProcessController.h"

#include "vtkCellData.h"
#include "vtkCharArray.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkImageData.h"
#include "vtkImageGaussianSource.h"
#include "vtkIntArray.h"
#include "vtkLogger.h"
#include "vtkMath.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPartitionedDataSetCollectionSource.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProcessGroup.h"
#include "vtkSMPTools.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"
#include "vtkStringFormatter.h"
#include "vtkTestUtilities.h"
#include "vtkTypeTraits.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedLongArray.h"

#include <cmath>
#include <time.h>
#include <vector>

#include <iostream>

// Update progress only on root node.
#define COUT(msg)                                                                                  \
  do                                                                                               \
  {                                                                                                \
    vtkLogIf(INFO, controller->GetLocalProcessId() == 0, "" msg);                                  \
  } while (false)

//=============================================================================
// A simple structure for passing data in and out of the parallel function.
struct ExerciseMultiProcessControllerArgs
{
  int retval;
};

//------------------------------------------------------------------------------
// A class to throw in the case of an error.
class ExerciseMultiProcessControllerError
{
};

//=============================================================================
// Establish a custom reduction operation that multiplies 2x2 matrices.
template <class T>
void MatrixMultArray(const T* A, T* B, vtkIdType length)
{
  for (vtkIdType i = 0; i < length / 4; i++)
  {
    T newVal[4];
    newVal[0] = A[0] * B[0] + A[1] * B[2];
    newVal[1] = A[0] * B[1] + A[1] * B[3];
    newVal[2] = A[2] * B[0] + A[3] * B[2];
    newVal[3] = A[2] * B[1] + A[3] * B[3];
    std::copy(newVal, newVal + 4, B);
    A += 4;
    B += 4;
  }
}

class MatrixMultOperation : public vtkCommunicator::Operation
{
public:
  void Function(const void* A, void* B, vtkIdType length, int type) override
  {
    switch (type)
    {
      vtkTemplateMacro(MatrixMultArray((const VTK_TT*)A, (VTK_TT*)B, length));
    }
  }
  int Commutative() override { return 0; }
};

//=============================================================================
// Compare if things are equal (or as close as we can expect).
template <class T>
inline int AreEqual(T a, T b)
{
  return a == b;
}

template <>
inline int AreEqual(float a, float b)
{
  float tolerance = std::abs(0.01f * a);
  return (std::abs(a - b) <= tolerance);
}

template <>
inline int AreEqual(double a, double b)
{
  double tolerance = std::abs(0.000001f * a);
  return (std::abs(a - b) <= tolerance);
}

//=============================================================================
// Check to see if any of the processes failed.
static void CheckSuccess(vtkMultiProcessController* controller, int success)
{
  int allSuccess;
  controller->Reduce(&success, &allSuccess, 1, vtkCommunicator::LOGICAL_AND_OP, 0);
  controller->Broadcast(&allSuccess, 1, 0);

  if (!allSuccess || !success)
  {
    COUT("**** Detected an ERROR ****");
    throw ExerciseMultiProcessControllerError();
  }
}

//------------------------------------------------------------------------------
template <class baseType, class arrayType>
void ExerciseType(vtkMultiProcessController* controller)
{
  COUT("---- Exercising " << vtkTypeTraits<baseType>::SizedName());

  typedef typename vtkTypeTraits<baseType>::PrintType printType;

  const int rank = controller->GetLocalProcessId();
  const int numProc = controller->GetNumberOfProcesses();
  int i;
  int srcProcessId;
  int destProcessId;
  vtkIdType length;
  std::vector<vtkIdType> lengths;
  lengths.resize(numProc);
  std::vector<vtkIdType> offsets;
  offsets.resize(numProc);
  const int arraySize = (numProc < 8) ? 8 : numProc;

  // Fill up some random arrays.  Note that here and elsewhere we are careful to
  // have each process request the same random numbers.  The pseudorandomness
  // gives us the same values on all processes.
  std::vector<vtkSmartPointer<arrayType>> sourceArrays;
  sourceArrays.resize(numProc);
  for (i = 0; i < numProc; i++)
  {
    sourceArrays[i] = vtkSmartPointer<arrayType>::New();
    sourceArrays[i]->SetNumberOfComponents(1);
    sourceArrays[i]->SetNumberOfTuples(arraySize);
    auto name = vtk::format("{:f}", vtkMath::Random());
    sourceArrays[i]->SetName(name.c_str());
    double min = std::is_unsigned<baseType>() ? 0.0 : -16.0;
    for (int j = 0; j < arraySize; j++)
    {
      sourceArrays[i]->SetValue(j, static_cast<baseType>(vtkMath::Random(min, 16.0)));
    }
  }
  COUT("Source Arrays:");
  if (rank == 0)
  {
    for (i = 0; i < numProc; i++)
    {
      for (int j = 0; j < arraySize; j++)
      {
        std::cout << setw(9) << static_cast<printType>(sourceArrays[i]->GetValue(j));
      }
      std::cout << std::endl;
    }
  }

  vtkNew<arrayType> buffer;
  vtkNew<arrayType> tmpSource;

  COUT("Basic send and receive.");
  bool result = true;
  buffer->Initialize();
  buffer->SetNumberOfComponents(1);
  buffer->SetNumberOfTuples(arraySize);
  for (i = 0; i < numProc; i++)
  {
    if (i < rank)
    {
      controller->Receive(buffer->GetPointer(0), arraySize, i, 9876);
      result &= vtkTestUtilities::CompareAbstractArray(sourceArrays[i], buffer);
      controller->Send(sourceArrays[rank]->GetPointer(0), arraySize, i, 5432);
    }
    else if (i > rank)
    {
      controller->Send(sourceArrays[rank]->GetPointer(0), arraySize, i, 9876);
      controller->Receive(buffer->GetPointer(0), arraySize, i, 5432);
      result &= vtkTestUtilities::CompareAbstractArray(sourceArrays[i], buffer);
    }
  }
  CheckSuccess(controller, result);

  COUT("Broadcast");
  srcProcessId = static_cast<int>(vtkMath::Random(0.0, numProc - 0.01));
  if (rank == srcProcessId)
  {
    buffer->DeepCopy(sourceArrays[srcProcessId]);
  }
  controller->Broadcast(buffer->GetPointer(0), arraySize, srcProcessId);
  result = vtkTestUtilities::CompareAbstractArray(sourceArrays[srcProcessId], buffer, arraySize);
  CheckSuccess(controller, result);

  COUT("Gather");
  destProcessId = static_cast<int>(vtkMath::Random(0.0, numProc - 0.99));
  buffer->SetNumberOfTuples(numProc * arraySize);
  result = true;
  if (rank == destProcessId)
  {
    controller->Gather(
      sourceArrays[rank]->GetPointer(0), buffer->GetPointer(0), arraySize, destProcessId);
    for (i = 0; i < numProc; i++)
    {
      for (int j = 0; j < arraySize; j++)
      {
        if (sourceArrays[i]->GetValue(j) != buffer->GetValue(i * arraySize + j))
        {
          vtkGenericWarningMacro("Gathered array from " << i << " incorrect at " << j << ".");
          result = false;
          break;
        }
      }
    }
  }
  else
  {
    controller->Gather(sourceArrays[rank]->GetPointer(0), nullptr, arraySize, destProcessId);
  }
  CheckSuccess(controller, result);

  COUT("All Gather");
  result = true;
  controller->AllGather(sourceArrays[rank]->GetPointer(0), buffer->GetPointer(0), arraySize);
  for (i = 0; i < numProc; i++)
  {
    for (int j = 0; j < arraySize; j++)
    {
      if (sourceArrays[i]->GetValue(j) != buffer->GetValue(i * arraySize + j))
      {
        vtkGenericWarningMacro("Gathered array from " << i << " incorrect at " << j << ".");
        result = false;
        break;
      }
    }
  }
  CheckSuccess(controller, result);

  COUT("Vector Gather");
  offsets[0] = static_cast<vtkIdType>(vtkMath::Random(0.0, 2.99));
  lengths[0] = static_cast<vtkIdType>(vtkMath::Random(0.0, arraySize + 0.99));
  for (i = 1; i < numProc; i++)
  {
    offsets[i] =
      (offsets[i - 1] + lengths[i - 1] + static_cast<vtkIdType>(vtkMath::Random(0.0, 2.99)));
    lengths[i] = static_cast<vtkIdType>(vtkMath::Random(0.0, arraySize + 0.99));
  }
  destProcessId = static_cast<int>(vtkMath::Random(0.0, numProc - 0.01));
  buffer->SetNumberOfTuples(offsets[numProc - 1] + lengths[numProc - 1]);
  result = true;
  if (rank == destProcessId)
  {
    controller->GatherV(sourceArrays[rank]->GetPointer(0), buffer->GetPointer(0), lengths[rank],
      lengths.data(), offsets.data(), destProcessId);
    for (i = 0; i < numProc; i++)
    {
      for (int j = 0; j < lengths[i]; j++)
      {
        if (sourceArrays[i]->GetValue(j) != buffer->GetValue(offsets[i] + j))
        {
          vtkGenericWarningMacro("Gathered array from " << i << " incorrect.");
          result = false;
          break;
        }
      }
    }
  }
  else
  {
    controller->GatherV(sourceArrays[rank]->GetPointer(0), nullptr, lengths[rank], lengths.data(),
      offsets.data(), destProcessId);
  }
  CheckSuccess(controller, result);

  COUT("Vector All Gather");
  offsets[0] = static_cast<vtkIdType>(vtkMath::Random(0.0, 2.99));
  lengths[0] = static_cast<vtkIdType>(vtkMath::Random(0.0, arraySize + 0.99));
  for (i = 1; i < numProc; i++)
  {
    offsets[i] =
      (offsets[i - 1] + lengths[i - 1] + static_cast<vtkIdType>(vtkMath::Random(0.0, 2.99)));
    lengths[i] = static_cast<vtkIdType>(vtkMath::Random(0.0, arraySize + 0.99));
  }
  buffer->SetNumberOfTuples(offsets[numProc - 1] + lengths[numProc - 1]);
  buffer->Fill(0.);
  result = true;
  controller->AllGatherV(sourceArrays[rank]->GetPointer(0), buffer->GetPointer(0), lengths[rank],
    lengths.data(), offsets.data());
  for (i = 0; i < numProc; i++)
  {
    for (int j = 0; j < lengths[i]; j++)
    {
      if (sourceArrays[i]->GetValue(j) != buffer->GetValue(offsets[i] + j))
      {
        vtkGenericWarningMacro("Gathered array from " << i << " incorrect at " << j << ".");
        result = false;
        break;
      }
    }
  }
  CheckSuccess(controller, result);

  COUT("Scatter");
  srcProcessId = static_cast<int>(vtkMath::Random(0.0, numProc - 0.01));
  length = arraySize / numProc;
  buffer->SetNumberOfTuples(length);
  if (rank == srcProcessId)
  {
    controller->Scatter(
      sourceArrays[rank]->GetPointer(0), buffer->GetPointer(0), length, srcProcessId);
  }
  else
  {
    controller->Scatter(nullptr, buffer->GetPointer(0), length, srcProcessId);
  }
  result = true;
  for (i = 0; i < length; i++)
  {
    if (sourceArrays[srcProcessId]->GetValue(rank * length + i) != buffer->GetValue(i))
    {
      vtkGenericWarningMacro(<< "Scattered array from " << srcProcessId << " incorrect at " << i
                             << ".");
      result = false;
      break;
    }
  }
  CheckSuccess(controller, result);

  COUT("Vector Scatter");
  srcProcessId = static_cast<int>(vtkMath::Random(0.0, numProc - 0.01));
  for (i = 0; i < numProc; i++)
  {
    offsets[i] = static_cast<vtkIdType>(vtkMath::Random(0.0, arraySize - 0.01));
    lengths[i] = static_cast<vtkIdType>(vtkMath::Random(0.0, arraySize - offsets[i] + 0.99));
  }
  buffer->SetNumberOfTuples(lengths[rank]);
  if (rank == srcProcessId)
  {
    controller->ScatterV(sourceArrays[rank]->GetPointer(0), buffer->GetPointer(0), lengths.data(),
      offsets.data(), lengths[rank], srcProcessId);
  }
  else
  {
    controller->ScatterV(
      nullptr, buffer->GetPointer(0), lengths.data(), offsets.data(), lengths[rank], srcProcessId);
  }
  result = true;
  for (i = 0; i < lengths[rank]; i++)
  {
    if (sourceArrays[srcProcessId]->GetValue(offsets[rank] + i) != buffer->GetValue(i))
    {
      vtkGenericWarningMacro(<< "Scattered array from " << srcProcessId << " incorrect.");
      result = false;
      break;
    }
  }
  CheckSuccess(controller, result);

  if (sizeof(baseType) > 1)
  {
    // Sum operation not defined for char/byte in some MPI implementations.
    COUT("Reduce");
    destProcessId = static_cast<int>(vtkMath::Random(0.0, numProc - 0.01));
    buffer->SetNumberOfTuples(arraySize);
    result = true;
    controller->Reduce(sourceArrays[rank]->GetPointer(0), buffer->GetPointer(0), arraySize,
      vtkCommunicator::SUM_OP, destProcessId);
    if (rank == destProcessId)
    {
      for (i = 0; i < arraySize; i++)
      {
        baseType total = static_cast<baseType>(0);
        for (int j = 0; j < numProc; j++)
          total += sourceArrays[j]->GetValue(i);
        if (!AreEqual(total, buffer->GetValue(i)))
        {
          vtkGenericWarningMacro(<< "Unequal computation in reduce: " << total << " vs. "
                                 << buffer->GetValue(i));
          result = false;
          break;
        }
      }
    }
    CheckSuccess(controller, result);
  }

  COUT("Custom Reduce");
  MatrixMultOperation operation;
  destProcessId = static_cast<int>(vtkMath::Random(0.0, numProc - 0.01));
  buffer->SetNumberOfTuples(arraySize);
  result = true;
  controller->Reduce(
    sourceArrays[rank]->GetPointer(0), buffer->GetPointer(0), arraySize, &operation, destProcessId);
  vtkNew<arrayType> totalArray;
  totalArray->DeepCopy(sourceArrays[numProc - 1]);
  for (i = numProc - 2; i >= 0; i--)
  {
    MatrixMultArray(sourceArrays[i]->GetPointer(0), totalArray->GetPointer(0), arraySize);
  }
  if (rank == destProcessId)
  {
    if (!vtkTestUtilities::CompareAbstractArray(totalArray, buffer))
    {
      result = false;
    }
  }
  CheckSuccess(controller, result);

  if (sizeof(baseType) > 1)
  {
    // Sum operation not defined for char/byte in some MPI implementations.
    COUT("All Reduce");
    buffer->SetNumberOfTuples(arraySize);
    result = true;
    controller->AllReduce(
      sourceArrays[rank]->GetPointer(0), buffer->GetPointer(0), arraySize, vtkCommunicator::SUM_OP);
    for (i = 0; i < arraySize; i++)
    {
      baseType total = static_cast<baseType>(0);
      for (int j = 0; j < numProc; j++)
        total += sourceArrays[j]->GetValue(i);
      if (!AreEqual(total, buffer->GetValue(i)))
      {
        vtkGenericWarningMacro(<< "Unequal computation in reduce: " << total << " vs. "
                               << buffer->GetValue(i));
        result = false;
        break;
      }
    }
    CheckSuccess(controller, result);
  }

  COUT("Custom All Reduce");
  buffer->SetNumberOfTuples(arraySize);
  result = true;
  controller->AllReduce(
    sourceArrays[rank]->GetPointer(0), buffer->GetPointer(0), arraySize, &operation);
  if (!vtkTestUtilities::CompareAbstractArray(totalArray, buffer))
  {
    result = false;
  }
  CheckSuccess(controller, result);

  //------------------------------------------------------------------
  // Repeat all the tests, but this time passing the vtkDataArray directly.
  COUT("Basic send and receive with vtkDataArray.");
  result = true;
  buffer->Initialize();
  for (i = 0; i < numProc; i++)
  {
    if (i < rank)
    {
      controller->Receive(buffer, i, 9876);
      result &= vtkTestUtilities::CompareAbstractArray(sourceArrays[i], buffer);
      controller->Send(sourceArrays[rank], i, 5432);
    }
    else if (i > rank)
    {
      controller->Send(sourceArrays[rank], i, 9876);
      controller->Receive(buffer, i, 5432);
      result &= vtkTestUtilities::CompareAbstractArray(sourceArrays[i], buffer);
    }
  }
  CheckSuccess(controller, result);

  COUT("Send and receive vtkDataArray with ANY_SOURCE as source.");
  if (rank == 0)
  {
    for (i = 1; i < numProc; i++)
    {
      buffer->Initialize();
      controller->Receive(buffer, vtkMultiProcessController::ANY_SOURCE, 7127);
      result &= vtkTestUtilities::CompareAbstractArray(sourceArrays[0], buffer);
    }
  }
  else
  {
    controller->Send(sourceArrays[0], 0, 7127);
  }
  CheckSuccess(controller, result);

  COUT("Broadcast with vtkDataArray");
  buffer->Initialize();
  srcProcessId = static_cast<int>(vtkMath::Random(0.0, numProc - 0.01));
  if (rank == srcProcessId)
  {
    buffer->DeepCopy(sourceArrays[srcProcessId]);
    buffer->SetName(sourceArrays[srcProcessId]->GetName());
  }
  controller->Broadcast(buffer, srcProcessId);
  result = vtkTestUtilities::CompareAbstractArray(sourceArrays[srcProcessId], buffer);
  CheckSuccess(controller, result);

  COUT("Gather with vtkDataArray");
  destProcessId = static_cast<int>(vtkMath::Random(0.0, numProc - 0.99));
  buffer->Initialize();
  result = true;
  if (rank == destProcessId)
  {
    controller->Gather(sourceArrays[rank], buffer, destProcessId);
    for (i = 0; i < numProc; i++)
    {
      for (int j = 0; j < arraySize; j++)
      {
        if (sourceArrays[i]->GetValue(j) != buffer->GetValue(i * arraySize + j))
        {
          vtkGenericWarningMacro("Gathered array from " << i << " incorrect.");
          result = false;
          break;
        }
      }
    }
  }
  else
  {
    controller->Gather(sourceArrays[rank], nullptr, destProcessId);
  }
  CheckSuccess(controller, result);

  COUT("Vector Gather with vtkDataArray");
  offsets[0] = static_cast<vtkIdType>(vtkMath::Random(0.0, 2.99));
  lengths[0] = static_cast<vtkIdType>(vtkMath::Random(0.0, arraySize + 0.99));
  for (i = 1; i < numProc; i++)
  {
    offsets[i] =
      (offsets[i - 1] + lengths[i - 1] + static_cast<vtkIdType>(vtkMath::Random(0.0, 2.99)));
    lengths[i] = static_cast<vtkIdType>(vtkMath::Random(0.0, arraySize + 0.99));
  }
  destProcessId = static_cast<int>(vtkMath::Random(0.0, numProc - 0.01));
  tmpSource->DeepCopy(sourceArrays[rank]);
  tmpSource->SetNumberOfTuples(lengths[rank]);
  buffer->SetNumberOfTuples(offsets[numProc - 1] + lengths[numProc - 1]);
  result = true;
  controller->GatherV(tmpSource, buffer, lengths.data(), offsets.data(), destProcessId);
  if (rank == destProcessId)
  {
    for (i = 0; i < numProc; i++)
    {
      for (int j = 0; j < lengths[i]; j++)
      {
        if (sourceArrays[i]->GetValue(j) != buffer->GetValue(offsets[i] + j))
        {
          vtkGenericWarningMacro("Gathered array from " << i << " incorrect.");
          result = false;
          break;
        }
      }
    }
  }
  CheckSuccess(controller, result);

  COUT("Vector Gather with vtkDataArray (automatic receive sizes)");
  lengths[0] = static_cast<vtkIdType>(vtkMath::Random(0.0, arraySize + 0.99));
  for (i = 1; i < numProc; i++)
  {
    lengths[i] = static_cast<vtkIdType>(vtkMath::Random(0.0, arraySize + 0.99));
  }
  destProcessId = static_cast<int>(vtkMath::Random(0.0, numProc - 0.01));
  tmpSource->DeepCopy(sourceArrays[rank]);
  tmpSource->SetNumberOfTuples(lengths[rank]);
  buffer->Initialize();
  result = true;
  if (rank == destProcessId)
  {
    controller->GatherV(tmpSource, buffer, destProcessId);
    int k = 0;
    for (i = 0; i < numProc; i++)
    {
      for (int j = 0; j < lengths[i]; j++)
      {
        if (sourceArrays[i]->GetValue(j) != buffer->GetValue(k++))
        {
          vtkGenericWarningMacro("Gathered array from " << i << " incorrect.");
          result = false;
          break;
        }
      }
    }
  }
  else
  {
    controller->GatherV(tmpSource, nullptr, destProcessId);
  }
  CheckSuccess(controller, result);

  COUT("All Gather with vtkDataArray");
  buffer->Initialize();
  result = true;
  controller->AllGather(sourceArrays[rank], buffer);
  for (i = 0; i < numProc; i++)
  {
    for (int j = 0; j < arraySize; j++)
    {
      if (sourceArrays[i]->GetValue(j) != buffer->GetValue(i * arraySize + j))
      {
        vtkGenericWarningMacro("Gathered array from " << i << " incorrect.");
        result = false;
        break;
      }
    }
  }
  CheckSuccess(controller, result);

  COUT("Vector All Gather with vtkDataArray");
  offsets[0] = static_cast<vtkIdType>(vtkMath::Random(0.0, 2.99));
  lengths[0] = static_cast<vtkIdType>(vtkMath::Random(0.0, arraySize + 0.99));
  for (i = 1; i < numProc; i++)
  {
    offsets[i] =
      (offsets[i - 1] + lengths[i - 1] + static_cast<vtkIdType>(vtkMath::Random(0.0, 2.99)));
    lengths[i] = static_cast<vtkIdType>(vtkMath::Random(0.0, arraySize + 0.99));
  }
  tmpSource->DeepCopy(sourceArrays[rank]);
  tmpSource->SetNumberOfTuples(lengths[rank]);
  buffer->SetNumberOfTuples(offsets[numProc - 1] + lengths[numProc - 1]);
  result = true;
  controller->AllGatherV(tmpSource, buffer, lengths.data(), offsets.data());
  for (i = 0; i < numProc; i++)
  {
    for (int j = 0; j < lengths[i]; j++)
    {
      if (sourceArrays[i]->GetValue(j) != buffer->GetValue(offsets[i] + j))
      {
        vtkGenericWarningMacro("Gathered array from " << i << " incorrect.");
        result = false;
        break;
      }
    }
  }
  CheckSuccess(controller, result);

  COUT("Vector All Gather with vtkDataArray (automatic receive sizes)");
  lengths[0] = static_cast<vtkIdType>(vtkMath::Random(0.0, arraySize + 0.99));
  for (i = 1; i < numProc; i++)
  {
    lengths[i] = static_cast<vtkIdType>(vtkMath::Random(0.0, arraySize + 0.99));
  }
  tmpSource->DeepCopy(sourceArrays[rank]);
  tmpSource->SetNumberOfTuples(lengths[rank]);
  buffer->Initialize();
  result = true;
  controller->AllGatherV(tmpSource, buffer);
  int k = 0;
  for (i = 0; i < numProc; i++)
  {
    for (int j = 0; j < lengths[i]; j++)
    {
      if (sourceArrays[i]->GetValue(j) != buffer->GetValue(k++))
      {
        vtkGenericWarningMacro("Gathered array from " << i << " incorrect.");
        result = false;
        break;
      }
    }
  }
  CheckSuccess(controller, result);

  COUT("Scatter with vtkDataArray");
  srcProcessId = static_cast<int>(vtkMath::Random(0.0, numProc - 0.01));
  length = arraySize / numProc;
  buffer->SetNumberOfTuples(length);
  if (rank == srcProcessId)
  {
    controller->Scatter(sourceArrays[rank], buffer, srcProcessId);
  }
  else
  {
    controller->Scatter(nullptr, buffer, srcProcessId);
  }
  result = true;
  for (i = 0; i < length; i++)
  {
    if (sourceArrays[srcProcessId]->GetValue(rank * length + i) != buffer->GetValue(i))
    {
      vtkGenericWarningMacro(<< "Scattered array from " << srcProcessId << " incorrect.");
      result = false;
      break;
    }
  }
  CheckSuccess(controller, result);

  if (sizeof(baseType) > 1)
  {
    // Sum operation not defined for char/byte in some MPI implementations.
    COUT("Reduce with vtkDataArray");
    destProcessId = static_cast<int>(vtkMath::Random(0.0, numProc - 0.01));
    buffer->Initialize();
    result = true;
    controller->Reduce(sourceArrays[rank], buffer, vtkCommunicator::SUM_OP, destProcessId);
    if (rank == destProcessId)
    {
      for (i = 0; i < arraySize; i++)
      {
        baseType total = static_cast<baseType>(0);
        for (int j = 0; j < numProc; j++)
          total += sourceArrays[j]->GetValue(i);
        if (!AreEqual(total, buffer->GetValue(i)))
        {
          vtkGenericWarningMacro(<< "Unequal computation in reduce: " << total << " vs. "
                                 << buffer->GetValue(i));
          result = false;
          break;
        }
      }
    }
    CheckSuccess(controller, result);
  }

  COUT("Custom Reduce with vtkDataArray");
  destProcessId = static_cast<int>(vtkMath::Random(0.0, numProc - 0.01));
  buffer->Initialize();
  result = true;
  controller->Reduce(sourceArrays[rank], buffer, &operation, destProcessId);
  if (rank == destProcessId)
  {
    if (!vtkTestUtilities::CompareAbstractArray(totalArray, buffer))
    {
      result = false;
    }
  }
  CheckSuccess(controller, result);

  if (sizeof(baseType) > 1)
  {
    // Sum operation not defined for char/byte in some MPI implementations.
    COUT("All Reduce with vtkDataArray");
    buffer->Initialize();
    result = true;
    controller->AllReduce(sourceArrays[rank], buffer, vtkCommunicator::SUM_OP);
    for (i = 0; i < arraySize; i++)
    {
      baseType total = static_cast<baseType>(0);
      for (int j = 0; j < numProc; j++)
        total += sourceArrays[j]->GetValue(i);
      if (!AreEqual(total, buffer->GetValue(i)))
      {
        vtkGenericWarningMacro(<< "Unequal computation in reduce: " << total << " vs. "
                               << buffer->GetValue(i));
        result = false;
        break;
      }
    }
    CheckSuccess(controller, result);
  }

  COUT("Custom All Reduce with vtkDataArray");
  buffer->Initialize();
  result = true;
  controller->AllReduce(sourceArrays[rank], buffer, &operation);
  if (!vtkTestUtilities::CompareAbstractArray(totalArray, buffer))
  {
    result = false;
  }
  CheckSuccess(controller, result);
}

//------------------------------------------------------------------------------
// Check the functions that transfer a data object.
static void ExerciseDataObject(
  vtkMultiProcessController* controller, vtkDataObject* source, vtkDataObject* buffer)
{
  COUT("---- Exercising " << source->GetClassName());

  const int rank = controller->GetLocalProcessId();
  const int numProc = controller->GetNumberOfProcesses();
  bool result = true;

  COUT("Basic send and receive with vtkDataObject.");
  for (int i = 0; i < numProc; i++)
  {
    if (i < rank)
    {
      buffer->Initialize();
      controller->Receive(buffer, i, 9876);
      result &= vtkTestUtilities::CompareDataObjects(source, buffer);
      controller->Send(source, i, 5432);
    }
    else if (i > rank)
    {
      controller->Send(source, i, 9876);
      buffer->Initialize();
      controller->Receive(buffer, i, 5432);
      result &= vtkTestUtilities::CompareDataObjects(source, buffer);
    }
  }
  CheckSuccess(controller, result);

  COUT("Send and receive vtkDataObject with ANY_SOURCE as source.");
  if (rank == 0)
  {
    for (int i = 1; i < numProc; i++)
    {
      buffer->Initialize();
      controller->Receive(buffer, vtkMultiProcessController::ANY_SOURCE, 3462);
      result &= vtkTestUtilities::CompareDataObjects(source, buffer);
    }
  }
  else
  {
    controller->Send(source, 0, 3462);
  }
  CheckSuccess(controller, result);

  COUT("Broadcast with vtkDataObject");
  buffer->Initialize();
  int srcProcessId = static_cast<int>(vtkMath::Random(0.0, numProc - 0.01));
  if (rank == srcProcessId)
  {
    buffer->DeepCopy(source);
  }
  controller->Broadcast(buffer, srcProcessId);
  result = vtkTestUtilities::CompareDataObjects(source, buffer);
  CheckSuccess(controller, result);

  COUT("AllGather with vtkDataObject");
  std::vector<vtkSmartPointer<vtkDataObject>> bufferVec;
  controller->AllGather(source, bufferVec);
  if (static_cast<int>(bufferVec.size()) != numProc)
  {
    vtkGenericWarningMacro("Incorrect vector size " << bufferVec.size());
    result &= 0;
  }
  else
  {
    for (auto& dobj : bufferVec)
    {
      result &= vtkTestUtilities::CompareDataObjects(source, dobj);
    }
  }

  COUT("Gather with vtkDataObject");
  bufferVec.clear();
  const int destProcessId = static_cast<int>(vtkMath::Random(0.0, numProc - 0.01));
  controller->Gather(source, bufferVec, destProcessId);
  if (rank == destProcessId)
  {
    if (static_cast<int>(bufferVec.size()) != numProc)
    {
      vtkGenericWarningMacro("Incorrect vector size " << bufferVec.size());
      result &= 0;
    }
    else
    {
      for (auto& dobj : bufferVec)
      {
        result &= vtkTestUtilities::CompareDataObjects(source, dobj);
      }
    }
  }
  else
  {
    if (!bufferVec.empty())
    {
      vtkGenericWarningMacro("Expected empty vector!");
      result &= 0;
    }
  }
  CheckSuccess(controller, result);
}

//------------------------------------------------------------------------------
static void Run(vtkMultiProcessController* controller, void* _args)
{
  ExerciseMultiProcessControllerArgs* args =
    reinterpret_cast<ExerciseMultiProcessControllerArgs*>(_args);
  args->retval = 0;

  COUT(<< endl
       << "Exercising " << controller->GetClassName() << ", " << controller->GetNumberOfProcesses()
       << " processes");

  try
  {
    vtkSMPTools::SetBackend("SEQUENTIAL");
    ExerciseType<int, vtkIntArray>(controller);
    ExerciseType<unsigned long, vtkUnsignedLongArray>(controller);
    ExerciseType<char, vtkCharArray>(controller);
    ExerciseType<unsigned char, vtkUnsignedCharArray>(controller);
    ExerciseType<float, vtkFloatArray>(controller);
    ExerciseType<double, vtkDoubleArray>(controller);
    ExerciseType<vtkIdType, vtkIdTypeArray>(controller);

    vtkNew<vtkImageGaussianSource> imageSource;
    imageSource->SetWholeExtent(-10, 10, -10, 10, -10, 10);
    imageSource->Update();
    ExerciseDataObject(controller, imageSource->GetOutput(), vtkSmartPointer<vtkImageData>::New());

    vtkNew<vtkSphereSource> polySource;
    polySource->Update();
    ExerciseDataObject(controller, polySource->GetOutput(), vtkSmartPointer<vtkPolyData>::New());

    vtkNew<vtkPartitionedDataSetCollectionSource> pdcSource;
    pdcSource->SetNumberOfShapes(12);
    pdcSource->Update();
    ExerciseDataObject(
      controller, pdcSource->GetOutput(), vtkSmartPointer<vtkPartitionedDataSetCollection>::New());
  }
  catch (ExerciseMultiProcessControllerError)
  {
    args->retval = 1;
  }
}

//------------------------------------------------------------------------------
int TestExerciseMultiProcessController(vtkMultiProcessController* controller)
{
  controller->CreateOutputWindow();

  // First, let us create a random seed that everyone will have.
  int seed;
  seed = time(nullptr);
  controller->Broadcast(&seed, 1, 0);
  COUT("**** Random Seed = " << seed << " ****");
  vtkMath::RandomSeed(seed);

  ExerciseMultiProcessControllerArgs args;

  controller->SetSingleMethod(Run, &args);
  controller->SingleMethodExecute();

  if (args.retval)
    return args.retval;

  // Run the same tests, except this time on a subgroup of processes.
  // We make sure that each subgroup has at least one process in it.
  vtkNew<vtkProcessGroup> group1;
  vtkNew<vtkProcessGroup> group2;
  group1->Initialize(controller);
  group1->RemoveProcessId(controller->GetNumberOfProcesses() - 1);
  group2->Initialize(controller);
  group2->RemoveAllProcessIds();
  group2->AddProcessId(controller->GetNumberOfProcesses() - 1);
  for (int i = controller->GetNumberOfProcesses() - 2; i >= 1; i--)
  {
    if (vtkMath::Random() < 0.5)
    {
      group1->RemoveProcessId(i);
      group2->AddProcessId(i);
    }
  }
  vtkMultiProcessController *subcontroller1, *subcontroller2;
  subcontroller1 = controller->CreateSubController(group1);
  subcontroller2 = controller->CreateSubController(group2);
  if (subcontroller1 && subcontroller2)
  {
    std::cout << "**** ERROR: Process " << controller->GetLocalProcessId()
              << " belongs to both subgroups! ****" << std::endl;
    subcontroller1->Delete();
    subcontroller2->Delete();
    return 1;
  }
  else if (subcontroller1)
  {
    subcontroller1->SetSingleMethod(Run, &args);
    subcontroller1->SingleMethodExecute();
    subcontroller1->Delete();
  }
  else if (subcontroller2)
  {
    subcontroller2->SetSingleMethod(Run, &args);
    subcontroller2->SingleMethodExecute();
    subcontroller2->Delete();
  }
  else
  {
    std::cout << "**** Error: Process " << controller->GetLocalProcessId()
              << " does not belong to either subgroup! ****" << std::endl;
  }
  try
  {
    CheckSuccess(controller, !args.retval);
  }
  catch (ExerciseMultiProcessControllerError)
  {
    args.retval = 1;
  }

  int color = (group1->GetLocalProcessId() >= 0) ? 1 : 2;
  vtkMultiProcessController* subcontroller = controller->PartitionController(color, 0);
  subcontroller->SetSingleMethod(Run, &args);
  subcontroller->SingleMethodExecute();
  subcontroller->Delete();

  try
  {
    CheckSuccess(controller, !args.retval);
  }
  catch (ExerciseMultiProcessControllerError)
  {
    args.retval = 1;
  }

  return args.retval;
}

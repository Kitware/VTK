/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ExerciseMultiProcessController.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "ExerciseMultiProcessController.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCharArray.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkImageData.h"
#include "vtkImageGaussianSource.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkMultiProcessController.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkProcessGroup.h"
#include "vtkSphereSource.h"
#include "vtkTypeTraits.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedLongArray.h"

#include <string.h>
#include <time.h>
#include <vector>

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

// Update progress only on root node.
#define COUT(msg) \
  if (controller->GetLocalProcessId() == 0) cout << "" msg << endl;

//=============================================================================
// A simple structure for passing data in and out of the parallel function.
struct ExerciseMultiProcessControllerArgs
{
  int retval;
};

//-----------------------------------------------------------------------------
// A class to throw in the case of an error.
class ExerciseMultiProcessControllerError {};


//=============================================================================
// Establish a custom reduction operation that multiplies 2x2 matrices.
template<class T>
void MatrixMultArray(const T *A, T *B, vtkIdType length)
{
  for (vtkIdType i = 0; i < length/4; i++)
    {
    T newVal[4];
    newVal[0] = A[0]*B[0] + A[1]*B[2];
    newVal[1] = A[0]*B[1] + A[1]*B[3];
    newVal[2] = A[2]*B[0] + A[3]*B[2];
    newVal[3] = A[2]*B[1] + A[3]*B[3];
    std::copy(newVal, newVal+4, B);
    A += 4;  B += 4;
    }
}

// Specialize for floats for greater precision.
VTK_TEMPLATE_SPECIALIZE
void MatrixMultArray(const float *A, float *B, vtkIdType length)
{
  double *tmpA = new double[length];
  double *tmpB = new double[length];
  for (vtkIdType i = 0; i < length; i++)
    {
    tmpA[i] = static_cast<double>(A[i]);
    tmpB[i] = static_cast<double>(B[i]);
    }
  MatrixMultArray(tmpA, tmpB, length);
  for (vtkIdType i = 0; i < length; i++)
    {
    B[i] = static_cast<float>(tmpB[i]);
    }
  delete[] tmpA;
  delete[] tmpB;
}

class MatrixMultOperation : public vtkCommunicator::Operation
{
public:
  void Function(const void *A, void *B, vtkIdType length, int type) {
    switch (type)
      {
      vtkTemplateMacro(MatrixMultArray((const VTK_TT*)A, (VTK_TT *)B, length));
      }
  }
  int Commutative() { return 0; }
};

//=============================================================================
// Compare if things are equal (or as close as we can expect).
template<class T>
inline int AreEqual(T a, T b)
{
  return a == b;
}

template<class T>
inline T myAbs(T x)
{
  return (x < 0) ? -x : x;
}

VTK_TEMPLATE_SPECIALIZE inline int AreEqual(float a, float b)
{
  float tolerance = myAbs(0.01f*a);
  return (myAbs(a-b) <= tolerance);
}

VTK_TEMPLATE_SPECIALIZE inline int AreEqual(double a, double b)
{
  double tolerance = myAbs(0.000001f*a);
  return (myAbs(a-b) <= tolerance);
}

//=============================================================================
// Check to see if any of the processes failed.
static void CheckSuccess(vtkMultiProcessController *controller, int success)
{
  int allSuccess;
  controller->Reduce(&success, &allSuccess, 1,
                     vtkCommunicator::LOGICAL_AND_OP, 0);
  controller->Broadcast(&allSuccess, 1, 0);

  if (!allSuccess || !success)
    {
    COUT("**** Detected an ERROR ****");
    throw ExerciseMultiProcessControllerError();
    }
}

//-----------------------------------------------------------------------------
template<class T>
int CompareArrays(const T *A, const T *B, vtkIdType length)
{
  for (vtkIdType i = 0; i < length; i++)
    {
    if (A[i] != B[i])
      {
      vtkGenericWarningMacro("Encountered mismatched arrays.");
      return 0;
      }
    }
  return 1;
}

int CompareDataArrays(vtkDataArray *A, vtkDataArray *B)
{
  if (A == B) return 1;

  int type = A->GetDataType();
  int numComponents = A->GetNumberOfComponents();
  vtkIdType numTuples = A->GetNumberOfTuples();
  if (type != B->GetDataType())
    {
    vtkGenericWarningMacro("Arrays have different types.");
    return 0;
    }
  if (numComponents != B->GetNumberOfComponents())
    {
    vtkGenericWarningMacro("Arrays have different numbers of components.");
    return 0;
    }
  if (numTuples != B->GetNumberOfTuples())
    {
    vtkGenericWarningMacro("Arrays have different numbers of tuples.");
    return 0;
    }
  if (A->GetName() && (strcmp(A->GetName(), B->GetName()) != 0))
    {
    vtkGenericWarningMacro("Arrays have different names.");
    return 0;
    }
  switch (type)
    {
    vtkTemplateMacro(return CompareArrays((VTK_TT *)A->GetVoidPointer(0),
                                          (VTK_TT *)B->GetVoidPointer(0),
                                          numComponents*numTuples));
    default:
      vtkGenericWarningMacro("Invalid type?");
    }
  return 0;
}

static int CompareFieldData(vtkFieldData *fd1, vtkFieldData *fd2)
{
  if (fd1->GetNumberOfArrays() != fd2->GetNumberOfArrays())
    {
    vtkGenericWarningMacro(<< "Different number of arrays in "
                           << fd1->GetClassName());
    return 0;
    }
  for (int i = 0; i < fd1->GetNumberOfArrays(); i++)
    {
    vtkAbstractArray *array1 = fd1->GetAbstractArray(i);
    // If the array does not have a name, then there is no good way to get
    // the equivalent array on the other end since the arrays may not be in
    // the same order.
    if (!array1->GetName()) continue;
    vtkAbstractArray *array2 = fd2->GetAbstractArray(array1->GetName());
    if (!CompareDataArrays(vtkDataArray::SafeDownCast(array1),
                           vtkDataArray::SafeDownCast(array2))) return 0;
    }

  return 1;
}

static int CompareDataSetAttributes(vtkDataSetAttributes *dsa1,
                                    vtkDataSetAttributes *dsa2)
{
  if (!CompareDataArrays(dsa1->GetScalars(), dsa2->GetScalars())) return 0;

  return CompareFieldData(dsa1, dsa2);
}

// This is not a complete comparison.  There are plenty of things not actually
// checked.  It only checks vtkImageData and vtkPolyData in detail.
static int CompareDataObjects(vtkDataObject *obj1, vtkDataObject *obj2)
{
  if (obj1->GetDataObjectType() != obj2->GetDataObjectType())
    {
    vtkGenericWarningMacro("Data objects are not of the same tyep.");
    return 0;
    }

  if (!CompareFieldData(obj1->GetFieldData(), obj2->GetFieldData())) return 0;

  vtkDataSet *ds1 = vtkDataSet::SafeDownCast(obj1);
  vtkDataSet *ds2 = vtkDataSet::SafeDownCast(obj2);

  if (ds1->GetNumberOfPoints() != ds2->GetNumberOfPoints())
    {
    vtkGenericWarningMacro("Point counts do not agree.");
    return 0;
    }
  if (ds1->GetNumberOfCells() != ds2->GetNumberOfCells())
    {
    vtkGenericWarningMacro("Cell counts do not agree.");
    return 0;
    }

  if (!CompareDataSetAttributes(ds1->GetPointData(), ds2->GetPointData()))
    {
    return 0;
    }
  if (!CompareDataSetAttributes(ds1->GetCellData(), ds2->GetCellData()))
    {
    return 0;
    }

  vtkImageData *id1 = vtkImageData::SafeDownCast(ds1);
  vtkImageData *id2 = vtkImageData::SafeDownCast(ds1);
  if (id1 && id2)
    {
    if (   (id1->GetDataDimension() != id2->GetDataDimension())
        || (id1->GetDimensions()[0] != id2->GetDimensions()[0])
        || (id1->GetDimensions()[1] != id2->GetDimensions()[1])
        || (id1->GetDimensions()[2] != id2->GetDimensions()[2]) )
      {
      vtkGenericWarningMacro("Dimensions of image data do not agree.");
      return 0;
      }

    if (!CompareArrays(id1->GetExtent(), id2->GetExtent(), 6)) return 0;
    if (!CompareArrays(id1->GetSpacing(), id2->GetSpacing(), 3)) return 0;
    if (!CompareArrays(id1->GetOrigin(), id2->GetOrigin(), 3)) return 0;
    }

  vtkPointSet *ps1 = vtkPointSet::SafeDownCast(ds1);
  vtkPointSet *ps2 = vtkPointSet::SafeDownCast(ds2);
  if (ps1 && ps2)
    {
    if (!CompareDataArrays(ps1->GetPoints()->GetData(),
                           ps2->GetPoints()->GetData())) return 0;

    vtkPolyData *pd1 = vtkPolyData::SafeDownCast(ps1);
    vtkPolyData *pd2 = vtkPolyData::SafeDownCast(ps2);
    if (pd1 && pd2)
      {
      if (!CompareDataArrays(pd1->GetVerts()->GetData(),
                             pd2->GetVerts()->GetData())) return 0;
      if (!CompareDataArrays(pd1->GetLines()->GetData(),
                             pd2->GetLines()->GetData())) return 0;
      if (!CompareDataArrays(pd1->GetPolys()->GetData(),
                             pd2->GetPolys()->GetData())) return 0;
      if (!CompareDataArrays(pd1->GetStrips()->GetData(),
                             pd2->GetStrips()->GetData())) return 0;
      }
    }

  return 1;
}

//-----------------------------------------------------------------------------
template<class baseType, class arrayType>
void ExerciseType(vtkMultiProcessController *controller)
{
  COUT("---- Exercising " << vtkTypeTraits<baseType>::SizedName());

  typedef typename vtkTypeTraits<baseType>::PrintType printType;

  const int rank = controller->GetLocalProcessId();
  const int numProc = controller->GetNumberOfProcesses();
  int i;
  int result;
  int srcProcessId;
  int destProcessId;
  vtkIdType length;
  std::vector<vtkIdType> lengths;  lengths.resize(numProc);
  std::vector<vtkIdType> offsets;  offsets.resize(numProc);
  const int arraySize = (numProc < 8) ? 8 : numProc;

  // Fill up some random arrays.  Note that here and elsewhere we are careful to
  // have each process request the same random numbers.  The pseudorandomness
  // gives us the same values on all processes.
  std::vector<vtkSmartPointer<arrayType> > sourceArrays;
  sourceArrays.resize(numProc);
  for (i = 0; i < numProc; i++)
    {
    sourceArrays[i] = vtkSmartPointer<arrayType>::New();
    sourceArrays[i]->SetNumberOfComponents(1);
    sourceArrays[i]->SetNumberOfTuples(arraySize);
    char name[80];
    sprintf(name, "%lf", vtkMath::Random());
    sourceArrays[i]->SetName(name);
    for (int j = 0; j < arraySize; j++)
      {
      sourceArrays[i]->SetValue(j,
                           static_cast<baseType>(vtkMath::Random(-16.0, 16.0)));
      }
    }
  COUT("Source Arrays:");
  if (rank == 0)
    {
    for (i = 0; i < numProc; i++)
      {
      for (int j = 0; j < arraySize; j++)
        {
        cout << setw(9) << static_cast<printType>(sourceArrays[i]->GetValue(j));
        }
      cout << endl;
      }
    }

  VTK_CREATE(arrayType, buffer);
  VTK_CREATE(arrayType, tmpSource);

  COUT("Basic send and receive.");
  result = 1;
  buffer->Initialize();
  buffer->SetNumberOfComponents(1);  buffer->SetNumberOfTuples(arraySize);
  for (i = 0; i < numProc; i++)
    {
    if (i < rank)
      {
      controller->Receive(buffer->GetPointer(0), arraySize, i, 9876);
      result &= CompareArrays(sourceArrays[i]->GetPointer(0),
                              buffer->GetPointer(0), arraySize);
      controller->Send(sourceArrays[rank]->GetPointer(0), arraySize, i, 5432);
      }
    else if (i > rank)
      {
      controller->Send(sourceArrays[rank]->GetPointer(0), arraySize, i, 9876);
      controller->Receive(buffer->GetPointer(0), arraySize, i, 5432);
      result &= CompareArrays(sourceArrays[i]->GetPointer(0),
                              buffer->GetPointer(0), arraySize);
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
  result = CompareArrays(sourceArrays[srcProcessId]->GetPointer(0),
                         buffer->GetPointer(0), arraySize);
  CheckSuccess(controller, result);

  COUT("Gather");
  destProcessId = static_cast<int>(vtkMath::Random(0.0, numProc - 0.99));
  buffer->SetNumberOfTuples(numProc*arraySize);
  result = 1;
  if (rank == destProcessId)
    {
    controller->Gather(sourceArrays[rank]->GetPointer(0), buffer->GetPointer(0),
                       arraySize, destProcessId);
    for (i = 0; i < numProc; i++)
      {
      for (int j = 0; j < arraySize; j++)
        {
        if (sourceArrays[i]->GetValue(j) != buffer->GetValue(i*arraySize + j))
          {
          vtkGenericWarningMacro("Gathered array from " << i << " incorrect.");
          result = 0;
          break;
          }
        }
      }
    }
  else
    {
    controller->Gather(sourceArrays[rank]->GetPointer(0), NULL, arraySize,
                       destProcessId);
    }
  CheckSuccess(controller, result);

  COUT("All Gather");
  result = 1;
  controller->AllGather(sourceArrays[rank]->GetPointer(0),
                        buffer->GetPointer(0), arraySize);
  for (i = 0; i < numProc; i++)
    {
    for (int j = 0; j < arraySize; j++)
      {
      if (sourceArrays[i]->GetValue(j) != buffer->GetValue(i*arraySize + j))
        {
        vtkGenericWarningMacro("Gathered array from " << i << " incorrect.");
        result = 0;
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
    offsets[i] = (  offsets[i-1] + lengths[i-1]
                  + static_cast<vtkIdType>(vtkMath::Random(0.0, 2.99)) );
    lengths[i]
      = static_cast<vtkIdType>(vtkMath::Random(0.0, arraySize + 0.99));
    }
  destProcessId = static_cast<int>(vtkMath::Random(0.0, numProc - 0.01));
  buffer->SetNumberOfTuples(offsets[numProc-1]+lengths[numProc-1]);
  result = 1;
  if (rank == destProcessId)
    {
    controller->GatherV(sourceArrays[rank]->GetPointer(0),
                        buffer->GetPointer(0), lengths[rank],
                        &lengths[0], &offsets[0], destProcessId);
    for (i = 0; i < numProc; i++)
      {
      for (int j = 0; j < lengths[i]; j++)
        {
        if (sourceArrays[i]->GetValue(j) != buffer->GetValue(offsets[i] + j))
          {
          vtkGenericWarningMacro("Gathered array from " << i << " incorrect.");
          result = 0;
          break;
          }
        }
      }
    }
  else
    {
    controller->GatherV(sourceArrays[rank]->GetPointer(0), NULL,
                        lengths[rank], NULL, NULL, destProcessId);
    }
  CheckSuccess(controller, result);

  COUT("Vector All Gather");
  offsets[0] = static_cast<vtkIdType>(vtkMath::Random(0.0, 2.99));
  lengths[0] = static_cast<vtkIdType>(vtkMath::Random(0.0, arraySize + 0.99));
  for (i = 1; i < numProc; i++)
    {
    offsets[i] = (  offsets[i-1] + lengths[i-1]
                  + static_cast<vtkIdType>(vtkMath::Random(0.0, 2.99)) );
    lengths[i]
      = static_cast<vtkIdType>(vtkMath::Random(0.0, arraySize + 0.99));
    }
  buffer->SetNumberOfTuples(offsets[numProc-1]+lengths[numProc-1]);
  result = 1;
  controller->AllGatherV(sourceArrays[rank]->GetPointer(0),
                         buffer->GetPointer(0), lengths[rank],
                         &lengths[0], &offsets[0]);
  for (i = 0; i < numProc; i++)
    {
    for (int j = 0; j < lengths[i]; j++)
      {
      if (sourceArrays[i]->GetValue(j) != buffer->GetValue(offsets[i] + j))
        {
        vtkGenericWarningMacro("Gathered array from " << i << " incorrect.");
        result = 0;
        break;
        }
      }
    }
  CheckSuccess(controller, result);

  COUT("Scatter");
  srcProcessId = static_cast<int>(vtkMath::Random(0.0, numProc - 0.01));
  length = arraySize/numProc;
  buffer->SetNumberOfTuples(length);
  if (rank == srcProcessId)
    {
    controller->Scatter(sourceArrays[rank]->GetPointer(0),
                        buffer->GetPointer(0), length, srcProcessId);
    }
  else
    {
    controller->Scatter(NULL, buffer->GetPointer(0), length, srcProcessId);
    }
  result = 1;
  for (i = 0; i < length; i++)
    {
    if (   sourceArrays[srcProcessId]->GetValue(rank*length+i)
        != buffer->GetValue(i) )
      {
      vtkGenericWarningMacro(<< "Scattered array from " << srcProcessId
                             << " incorrect.");
      result = 0;
      break;
      }
    }
  CheckSuccess(controller, result);

  COUT("Vector Scatter");
  srcProcessId = static_cast<int>(vtkMath::Random(0.0, numProc - 0.01));
  for (i = 0; i < numProc; i++)
    {
    offsets[i] = static_cast<vtkIdType>(vtkMath::Random(0.0, arraySize - 0.01));
    lengths[i] = static_cast<vtkIdType>(
                           vtkMath::Random(0.0, arraySize - offsets[i] + 0.99));
    }
  buffer->SetNumberOfTuples(lengths[rank]);
  if (rank == srcProcessId)
    {
    controller->ScatterV(sourceArrays[rank]->GetPointer(0),
                         buffer->GetPointer(0), &lengths[0], &offsets[0],
                         lengths[rank], srcProcessId);
    }
  else
    {
    controller->ScatterV(NULL, buffer->GetPointer(0), &lengths[0], &offsets[0],
                         lengths[rank], srcProcessId);
    }
  result = 1;
  for (i = 0; i < lengths[rank]; i++)
    {
    if (   sourceArrays[srcProcessId]->GetValue(offsets[rank]+i)
        != buffer->GetValue(i) )
      {
      vtkGenericWarningMacro(<< "Scattered array from " << srcProcessId
                             << " incorrect.");
      result = 0;
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
    result = 1;
    controller->Reduce(sourceArrays[rank]->GetPointer(0), buffer->GetPointer(0),
                       arraySize, vtkCommunicator::SUM_OP,
                       destProcessId);
    if (rank == destProcessId)
      {
      for (i = 0; i < arraySize; i++)
        {
        baseType total = static_cast<baseType>(0);
        for (int j = 0; j < numProc; j++) total += sourceArrays[j]->GetValue(i);
        if (!AreEqual(total, buffer->GetValue(i)))
          {
          vtkGenericWarningMacro(<< "Unequal computation in reduce: "
                                 << total << " vs. " << buffer->GetValue(i));
          result = 0;
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
  result = 1;
  controller->Reduce(sourceArrays[rank]->GetPointer(0), buffer->GetPointer(0),
                     arraySize, &operation, destProcessId);
  VTK_CREATE(arrayType, totalArray);
  totalArray->DeepCopy(sourceArrays[numProc-1]);
  for (i = numProc-2; i >= 0; i--)
    {
    MatrixMultArray(sourceArrays[i]->GetPointer(0), totalArray->GetPointer(0),
                    arraySize);
    }
  if (rank == destProcessId)
    {
    for (i = 0; i < arraySize; i++)
      {
      if (!AreEqual(totalArray->GetValue(i), buffer->GetValue(i)))
        {
        vtkGenericWarningMacro(<< "Unequal computation in reduce: "
                               << totalArray->GetValue(i) << " vs. "
                               << buffer->GetValue(i));
        result = 0;
        break;
        }
      }
    }
  CheckSuccess(controller, result);

  if (sizeof(baseType) > 1)
    {
    // Sum operation not defined for char/byte in some MPI implementations.
    COUT("All Reduce");
    buffer->SetNumberOfTuples(arraySize);
    result = 1;
    controller->AllReduce(sourceArrays[rank]->GetPointer(0),
                          buffer->GetPointer(0),
                          arraySize, vtkCommunicator::SUM_OP);
    for (i = 0; i < arraySize; i++)
      {
      baseType total = static_cast<baseType>(0);
      for (int j = 0; j < numProc; j++) total += sourceArrays[j]->GetValue(i);
      if (!AreEqual(total, buffer->GetValue(i)))
        {
        vtkGenericWarningMacro(<< "Unequal computation in reduce: "
                               << total << " vs. " << buffer->GetValue(i));
        result = 0;
        break;
        }
      }
    CheckSuccess(controller, result);
    }

  COUT("Custom All Reduce");
  buffer->SetNumberOfTuples(arraySize);
  result = 1;
  controller->AllReduce(sourceArrays[rank]->GetPointer(0),
                        buffer->GetPointer(0),
                        arraySize, &operation);
  for (i = 0; i < arraySize; i++)
    {
    if (!AreEqual(totalArray->GetValue(i), buffer->GetValue(i)))
      {
      vtkGenericWarningMacro(<< "Unequal computation in reduce: "
                             << totalArray->GetValue(i) << " vs. "
                             << buffer->GetValue(i));
      result = 0;
      break;
      }
    }
  CheckSuccess(controller, result);

  //------------------------------------------------------------------
  // Repeat all the tests, but this time passing the vtkDataArray directly.
  COUT("Basic send and receive with vtkDataArray.");
  result = 1;
  buffer->Initialize();
  for (i = 0; i < numProc; i++)
    {
    if (i < rank)
      {
      controller->Receive(buffer, i, 9876);
      result &= CompareDataArrays(sourceArrays[i], buffer);
      controller->Send(sourceArrays[rank], i, 5432);
      }
    else if (i > rank)
      {
      controller->Send(sourceArrays[rank], i, 9876);
      controller->Receive(buffer, i, 5432);
      result &= CompareDataArrays(sourceArrays[i], buffer);
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
      result &= CompareDataArrays(sourceArrays[0], buffer);
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
  result = CompareDataArrays(sourceArrays[srcProcessId], buffer);
  CheckSuccess(controller, result);

  COUT("Gather with vtkDataArray");
  destProcessId = static_cast<int>(vtkMath::Random(0.0, numProc - 0.99));
  buffer->Initialize();
  result = 1;
  if (rank == destProcessId)
    {
    controller->Gather(sourceArrays[rank], buffer, destProcessId);
    for (i = 0; i < numProc; i++)
      {
      for (int j = 0; j < arraySize; j++)
        {
        if (sourceArrays[i]->GetValue(j) != buffer->GetValue(i*arraySize + j))
          {
          vtkGenericWarningMacro("Gathered array from " << i << " incorrect.");
          result = 0;
          break;
          }
        }
      }
    }
  else
    {
    controller->Gather(sourceArrays[rank], NULL, destProcessId);
    }
  CheckSuccess(controller, result);

  COUT("Vector Gather with vtkDataArray");
  offsets[0] = static_cast<vtkIdType>(vtkMath::Random(0.0, 2.99));
  lengths[0] = static_cast<vtkIdType>(vtkMath::Random(0.0, arraySize + 0.99));
  for (i = 1; i < numProc; i++)
    {
    offsets[i] = (  offsets[i-1] + lengths[i-1]
                  + static_cast<vtkIdType>(vtkMath::Random(0.0, 2.99)) );
    lengths[i]
      = static_cast<vtkIdType>(vtkMath::Random(0.0, arraySize + 0.99));
    }
  destProcessId = static_cast<int>(vtkMath::Random(0.0, numProc - 0.01));
  tmpSource->DeepCopy(sourceArrays[rank]);
  tmpSource->SetNumberOfTuples(lengths[rank]);
  buffer->SetNumberOfTuples(offsets[numProc-1]+lengths[numProc-1]);
  result = 1;
  if (rank == destProcessId)
    {
    controller->GatherV(tmpSource, buffer,
                        &lengths[0], &offsets[0], destProcessId);
    for (i = 0; i < numProc; i++)
      {
      for (int j = 0; j < lengths[i]; j++)
        {
        if (sourceArrays[i]->GetValue(j) != buffer->GetValue(offsets[i] + j))
          {
          vtkGenericWarningMacro("Gathered array from " << i << " incorrect.");
          result = 0;
          break;
          }
        }
      }
    }
  else
    {
    controller->GatherV(tmpSource, NULL, NULL, NULL, destProcessId);
    }
  CheckSuccess(controller, result);

  COUT("Vector Gather with vtkDataArray (automatic receive sizes)");
  lengths[0] = static_cast<vtkIdType>(vtkMath::Random(0.0, arraySize + 0.99));
  for (i = 1; i < numProc; i++)
    {
    lengths[i]
      = static_cast<vtkIdType>(vtkMath::Random(0.0, arraySize + 0.99));
    }
  destProcessId = static_cast<int>(vtkMath::Random(0.0, numProc - 0.01));
  tmpSource->DeepCopy(sourceArrays[rank]);
  tmpSource->SetNumberOfTuples(lengths[rank]);
  buffer->Initialize();
  result = 1;
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
          result = 0;
          break;
          }
        }
      }
    }
  else
    {
    controller->GatherV(tmpSource, NULL, destProcessId);
    }
  CheckSuccess(controller, result);

  COUT("All Gather with vtkDataArray");
  buffer->Initialize();
  result = 1;
  controller->AllGather(sourceArrays[rank], buffer);
  for (i = 0; i < numProc; i++)
    {
    for (int j = 0; j < arraySize; j++)
      {
      if (sourceArrays[i]->GetValue(j) != buffer->GetValue(i*arraySize + j))
        {
        vtkGenericWarningMacro("Gathered array from " << i << " incorrect.");
        result = 0;
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
    offsets[i] = (  offsets[i-1] + lengths[i-1]
                  + static_cast<vtkIdType>(vtkMath::Random(0.0, 2.99)) );
    lengths[i]
      = static_cast<vtkIdType>(vtkMath::Random(0.0, arraySize + 0.99));
    }
  tmpSource->DeepCopy(sourceArrays[rank]);
  tmpSource->SetNumberOfTuples(lengths[rank]);
  buffer->SetNumberOfTuples(offsets[numProc-1]+lengths[numProc-1]);
  result = 1;
  controller->AllGatherV(tmpSource, buffer, &lengths[0], &offsets[0]);
  for (i = 0; i < numProc; i++)
    {
    for (int j = 0; j < lengths[i]; j++)
      {
      if (sourceArrays[i]->GetValue(j) != buffer->GetValue(offsets[i] + j))
        {
        vtkGenericWarningMacro("Gathered array from " << i << " incorrect.");
        result = 0;
        break;
        }
      }
    }
  CheckSuccess(controller, result);

  COUT("Vector All Gather with vtkDataArray (automatic receive sizes)");
  lengths[0] = static_cast<vtkIdType>(vtkMath::Random(0.0, arraySize + 0.99));
  for (i = 1; i < numProc; i++)
    {
    lengths[i]
      = static_cast<vtkIdType>(vtkMath::Random(0.0, arraySize + 0.99));
    }
  tmpSource->DeepCopy(sourceArrays[rank]);
  tmpSource->SetNumberOfTuples(lengths[rank]);
  buffer->Initialize();
  result = 1;
  controller->AllGatherV(tmpSource, buffer);
  int k = 0;
  for (i = 0; i < numProc; i++)
    {
    for (int j = 0; j < lengths[i]; j++)
      {
      if (sourceArrays[i]->GetValue(j) != buffer->GetValue(k++))
        {
        vtkGenericWarningMacro("Gathered array from " << i << " incorrect.");
        result = 0;
        break;
        }
      }
    }
  CheckSuccess(controller, result);

  COUT("Scatter with vtkDataArray");
  srcProcessId = static_cast<int>(vtkMath::Random(0.0, numProc - 0.01));
  length = arraySize/numProc;
  buffer->SetNumberOfTuples(length);
  if (rank == srcProcessId)
    {
    controller->Scatter(sourceArrays[rank], buffer, srcProcessId);
    }
  else
    {
    controller->Scatter(NULL, buffer, srcProcessId);
    }
  result = 1;
  for (i = 0; i < length; i++)
    {
    if (   sourceArrays[srcProcessId]->GetValue(rank*length+i)
        != buffer->GetValue(i) )
      {
      vtkGenericWarningMacro(<< "Scattered array from " << srcProcessId
                             << " incorrect.");
      result = 0;
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
    result = 1;
    controller->Reduce(sourceArrays[rank], buffer,
                       vtkCommunicator::SUM_OP, destProcessId);
    if (rank == destProcessId)
      {
      for (i = 0; i < arraySize; i++)
        {
        baseType total = static_cast<baseType>(0);
        for (int j = 0; j < numProc; j++) total += sourceArrays[j]->GetValue(i);
        if (!AreEqual(total, buffer->GetValue(i)))
          {
          vtkGenericWarningMacro(<< "Unequal computation in reduce: "
                                 << total << " vs. " << buffer->GetValue(i));
          result = 0;
          break;
          }
        }
      }
    CheckSuccess(controller, result);
    }

  COUT("Custom Reduce with vtkDataArray");
  destProcessId = static_cast<int>(vtkMath::Random(0.0, numProc - 0.01));
  buffer->Initialize();
  result = 1;
  controller->Reduce(sourceArrays[rank], buffer, &operation, destProcessId);
  if (rank == destProcessId)
    {
    for (i = 0; i < arraySize; i++)
      {
      if (!AreEqual(totalArray->GetValue(i), buffer->GetValue(i)))
        {
        vtkGenericWarningMacro(<< "Unequal computation in reduce: "
                               << totalArray->GetValue(i) << " vs. "
                               << buffer->GetValue(i));
        result = 0;
        break;
        }
      }
    }
  CheckSuccess(controller, result);

  if (sizeof(baseType) > 1)
    {
    // Sum operation not defined for char/byte in some MPI implementations.
    COUT("All Reduce with vtkDataArray");
    buffer->Initialize();
    result = 1;
    controller->AllReduce(sourceArrays[rank], buffer,
                          vtkCommunicator::SUM_OP);
    for (i = 0; i < arraySize; i++)
      {
      baseType total = static_cast<baseType>(0);
      for (int j = 0; j < numProc; j++) total += sourceArrays[j]->GetValue(i);
      if (!AreEqual(total, buffer->GetValue(i)))
        {
        vtkGenericWarningMacro(<< "Unequal computation in reduce: "
                               << total << " vs. " << buffer->GetValue(i));
        result = 0;
        break;
        }
      }
    CheckSuccess(controller, result);
    }

  COUT("Custom All Reduce with vtkDataArray");
  buffer->Initialize();
  result = 1;
  controller->AllReduce(sourceArrays[rank], buffer, &operation);
  for (i = 0; i < arraySize; i++)
    {
    if (!AreEqual(totalArray->GetValue(i), buffer->GetValue(i)))
      {
      vtkGenericWarningMacro(<< "Unequal computation in reduce: "
                             << totalArray->GetValue(i) << " vs. "
                             << buffer->GetValue(i));
      result = 0;
      break;
      }
    }
  CheckSuccess(controller, result);
}

//-----------------------------------------------------------------------------
// Check the functions that transfer a data object.
static void ExerciseDataObject(vtkMultiProcessController *controller,
                               vtkDataObject *source, vtkDataObject *buffer)
{
  COUT("---- Exercising " << source->GetClassName());

  int rank = controller->GetLocalProcessId();
  int numProc = controller->GetNumberOfProcesses();
  int result;
  int i;

  COUT("Basic send and receive with vtkDataObject.");
  result = 1;
  for (i = 0; i < numProc; i++)
    {
    if (i < rank)
      {
      buffer->Initialize();
      controller->Receive(buffer, i, 9876);
      result &= CompareDataObjects(source, buffer);
      controller->Send(source, i, 5432);
      }
    else if (i > rank)
      {
      controller->Send(source, i, 9876);
      buffer->Initialize();
      controller->Receive(buffer, i, 5432);
      result &= CompareDataObjects(source, buffer);
      }
    }
  CheckSuccess(controller, result);

  COUT("Send and receive vtkDataObject with ANY_SOURCE as source.");
  if (rank == 0)
    {
    for (i = 1; i < numProc; i++)
      {
      buffer->Initialize();
      controller->Receive(buffer, vtkMultiProcessController::ANY_SOURCE, 3462);
      result &= CompareDataObjects(source, buffer);
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
  result = CompareDataObjects(source, buffer);
  CheckSuccess(controller, result);
}

//-----------------------------------------------------------------------------
static void Run(vtkMultiProcessController *controller, void *_args)
{
  ExerciseMultiProcessControllerArgs *args
    = reinterpret_cast<ExerciseMultiProcessControllerArgs *>(_args);
  args->retval = 0;

  COUT(<< endl << "Exercising " << controller->GetClassName()
       << ", " << controller->GetNumberOfProcesses() << " processes");

  try
    {
    ExerciseType<int, vtkIntArray>(controller);
    ExerciseType<unsigned long, vtkUnsignedLongArray>(controller);
    ExerciseType<char, vtkCharArray>(controller);
    ExerciseType<unsigned char, vtkUnsignedCharArray>(controller);
    ExerciseType<float, vtkFloatArray>(controller);
    ExerciseType<double, vtkDoubleArray>(controller);
    ExerciseType<vtkIdType, vtkIdTypeArray>(controller);

    VTK_CREATE(vtkImageGaussianSource, imageSource);
    imageSource->SetWholeExtent(-10, 10, -10, 10, -10, 10);
    imageSource->Update();
    ExerciseDataObject(controller, imageSource->GetOutput(),
                       vtkSmartPointer<vtkImageData>::New());

    VTK_CREATE(vtkSphereSource, polySource);
    polySource->Update();
    ExerciseDataObject(controller, polySource->GetOutput(),
                       vtkSmartPointer<vtkPolyData>::New());
    }
  catch (ExerciseMultiProcessControllerError)
    {
    args->retval = 1;
    }
}

//-----------------------------------------------------------------------------
int ExerciseMultiProcessController(vtkMultiProcessController *controller)
{
  controller->CreateOutputWindow();

  // First, let us create a random seed that everyone will have.
  int seed;
  seed = time(NULL);
  controller->Broadcast(&seed, 1, 0);
  COUT("**** Random Seed = " << seed << " ****");
  vtkMath::RandomSeed(seed);

  ExerciseMultiProcessControllerArgs args;

  controller->SetSingleMethod(Run, &args);
  controller->SingleMethodExecute();

  if (args.retval) return args.retval;

  // Run the same tests, except this time on a subgroup of processes.
  VTK_CREATE(vtkProcessGroup, group1);
  VTK_CREATE(vtkProcessGroup, group2);
  group1->Initialize(controller);
  group2->Initialize(controller);
  group2->RemoveAllProcessIds();
  for (int i = controller->GetNumberOfProcesses() - 1; i >= 0; i--)
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
    cout << "**** ERROR: Process " << controller->GetLocalProcessId()
         << " belongs to both subgroups! ****" << endl;
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
    cout << "**** Error: Process " << controller->GetLocalProcessId()
         << " does not belong to either subgroup! ****" << endl;
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
  vtkMultiProcessController *subcontroller
    = controller->PartitionController(color, 0);
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

// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Test SetBuffer() for AOS and SOA data array templates, and verify that
// BufferChangedEvent fires from SetBuffer, SetArray, and ShallowCopy.

#include "vtkAOSDataArrayTemplate.h"
#include "vtkBuffer.h"
#include "vtkCallbackCommand.h"
#include "vtkCommand.h"
#include "vtkNew.h"
#include "vtkSOADataArrayTemplate.h"

#include <iostream>

namespace
{

int EventCount = 0;

void OnBufferChanged(vtkObject*, unsigned long, void*, void*)
{
  EventCount++;
}

// Helper: attach a BufferChangedEvent observer and reset the counter.
vtkNew<vtkCallbackCommand> AttachObserver(vtkObject* obj)
{
  EventCount = 0;
  vtkNew<vtkCallbackCommand> cb;
  cb->SetCallback(OnBufferChanged);
  obj->AddObserver(vtkCommand::BufferChangedEvent, cb);
  return cb;
}

} // namespace

int TestSetBuffer(int, char*[])
{
  int retVal = 0;

  // ===================================================================
  // AOS SetBuffer — verify data is accessible
  // ===================================================================
  {
    vtkNew<vtkBuffer<double>> buffer;
    buffer->Allocate(6);
    double* ptr = buffer->GetBuffer();
    for (int i = 0; i < 6; i++)
    {
      ptr[i] = static_cast<double>(i + 1);
    }

    vtkNew<vtkAOSDataArrayTemplate<double>> arr;
    arr->SetNumberOfComponents(2);
    arr->SetBuffer(buffer, true);

    if (arr->GetNumberOfTuples() != 3)
    {
      std::cerr << "AOS SetBuffer: expected 3 tuples, got " << arr->GetNumberOfTuples() << "\n";
      retVal++;
    }
    for (int i = 0; i < 6; i++)
    {
      if (arr->GetValue(i) != static_cast<double>(i + 1))
      {
        std::cerr << "AOS SetBuffer: value mismatch at index " << i << "\n";
        retVal++;
        break;
      }
    }
    double tuple[2];
    arr->GetTuple(1, tuple);
    if (tuple[0] != 3.0 || tuple[1] != 4.0)
    {
      std::cerr << "AOS SetBuffer: GetTuple(1) returned wrong values\n";
      retVal++;
    }
  }

  // ===================================================================
  // AOS SetBuffer without updateMaxId
  // ===================================================================
  {
    vtkNew<vtkBuffer<double>> buffer;
    buffer->Allocate(10);

    vtkNew<vtkAOSDataArrayTemplate<double>> arr;
    arr->SetNumberOfComponents(1);
    arr->SetNumberOfTuples(0);
    arr->SetBuffer(buffer, false);

    if (arr->GetNumberOfTuples() != 0)
    {
      std::cerr << "AOS SetBuffer (no updateMaxId): expected 0 tuples, got "
                << arr->GetNumberOfTuples() << "\n";
      retVal++;
    }
  }

  // ===================================================================
  // AOS SetBuffer fires BufferChangedEvent
  // ===================================================================
  {
    vtkNew<vtkAOSDataArrayTemplate<double>> arr;
    arr->SetNumberOfComponents(1);
    AttachObserver(arr);

    vtkNew<vtkBuffer<double>> buffer;
    buffer->Allocate(4);
    arr->SetBuffer(buffer, true);

    if (EventCount < 1)
    {
      std::cerr << "AOS SetBuffer did not fire BufferChangedEvent\n";
      retVal++;
    }
  }

  // ===================================================================
  // AOS SetArray fires BufferChangedEvent
  // ===================================================================
  {
    vtkNew<vtkAOSDataArrayTemplate<double>> arr;
    arr->SetNumberOfComponents(1);
    AttachObserver(arr);

    double data[] = { 1.0, 2.0, 3.0 };
    arr->SetArray(data, 3, 1); // save=1 so array won't free

    if (EventCount < 1)
    {
      std::cerr << "AOS SetArray did not fire BufferChangedEvent\n";
      retVal++;
    }
  }

  // ===================================================================
  // AOS ShallowCopy fires BufferChangedEvent
  // ===================================================================
  {
    vtkNew<vtkAOSDataArrayTemplate<double>> src;
    src->SetNumberOfComponents(2);
    src->SetNumberOfTuples(3);
    for (int i = 0; i < 6; i++)
    {
      src->SetValue(i, static_cast<double>(i));
    }

    vtkNew<vtkAOSDataArrayTemplate<double>> dst;
    dst->SetNumberOfComponents(2);
    AttachObserver(dst);

    dst->ShallowCopy(src);

    if (EventCount < 1)
    {
      std::cerr << "AOS ShallowCopy did not fire BufferChangedEvent\n";
      retVal++;
    }
  }

  // ===================================================================
  // SOA SetBuffer — verify data is accessible
  // ===================================================================
  {
    const int numTuples = 4;
    vtkNew<vtkBuffer<double>> buf0;
    buf0->Allocate(numTuples);
    vtkNew<vtkBuffer<double>> buf1;
    buf1->Allocate(numTuples);

    for (int i = 0; i < numTuples; i++)
    {
      buf0->GetBuffer()[i] = static_cast<double>(i);
      buf1->GetBuffer()[i] = static_cast<double>(10 + i);
    }

    vtkNew<vtkSOADataArrayTemplate<double>> arr;
    arr->SetNumberOfComponents(2);
    arr->SetBuffer(0, buf0, true);
    arr->SetBuffer(1, buf1, false);

    if (arr->GetNumberOfTuples() != numTuples)
    {
      std::cerr << "SOA SetBuffer: expected " << numTuples << " tuples, got "
                << arr->GetNumberOfTuples() << "\n";
      retVal++;
    }
    double tuple[2];
    arr->GetTuple(2, tuple);
    if (tuple[0] != 2.0 || tuple[1] != 12.0)
    {
      std::cerr << "SOA SetBuffer: GetTuple(2) returned wrong values\n";
      retVal++;
    }
  }

  // ===================================================================
  // SOA SetBuffer fires BufferChangedEvent
  // ===================================================================
  {
    vtkNew<vtkSOADataArrayTemplate<double>> arr;
    arr->SetNumberOfComponents(2);
    arr->SetNumberOfTuples(1);
    AttachObserver(arr);

    vtkNew<vtkBuffer<double>> buffer;
    buffer->Allocate(4);
    arr->SetBuffer(0, buffer, true);

    if (EventCount < 1)
    {
      std::cerr << "SOA SetBuffer did not fire BufferChangedEvent\n";
      retVal++;
    }
  }

  // ===================================================================
  // SOA SetArray fires BufferChangedEvent
  // ===================================================================
  {
    vtkNew<vtkSOADataArrayTemplate<double>> arr;
    arr->SetNumberOfComponents(2);
    arr->SetNumberOfTuples(1);
    AttachObserver(arr);

    double data[] = { 1.0, 2.0, 3.0 };
    arr->SetArray(0, data, 3, false, true);

    if (EventCount < 1)
    {
      std::cerr << "SOA SetArray did not fire BufferChangedEvent\n";
      retVal++;
    }
  }

  // ===================================================================
  // SOA ShallowCopy fires BufferChangedEvent
  // ===================================================================
  {
    vtkNew<vtkSOADataArrayTemplate<double>> src;
    src->SetNumberOfComponents(2);
    src->SetNumberOfTuples(3);
    for (int i = 0; i < 3; i++)
    {
      double tuple[2] = { static_cast<double>(i), static_cast<double>(10 + i) };
      src->SetTuple(i, tuple);
    }

    vtkNew<vtkSOADataArrayTemplate<double>> dst;
    dst->SetNumberOfComponents(2);
    AttachObserver(dst);

    dst->ShallowCopy(src);

    if (EventCount < 1)
    {
      std::cerr << "SOA ShallowCopy did not fire BufferChangedEvent\n";
      retVal++;
    }
  }

  return retVal;
}

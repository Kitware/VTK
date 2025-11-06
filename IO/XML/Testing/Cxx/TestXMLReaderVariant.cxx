// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkContourFilter.h"
#include "vtkFieldData.h"
#include "vtkTesting.h"
#include "vtkVariantArray.h"
#include "vtkXMLPolyDataReader.h"
#include "vtkXMLPolyDataWriter.h"

int TestXMLReaderVariant(int argc, char* argv[])
{
  vtkNew<vtkPolyData> pd;

  vtkNew<vtkVariantArray> a;
  a->SetName("data");

  a->InsertNextValue(vtkVariant(2));

  pd->GetFieldData()->AddArray(a);

  vtkNew<vtkTesting> testing;
  testing->AddArguments(argc, argv);
  vtkNew<vtkXMLPolyDataWriter> writer;
  writer->SetFileName((std::string(testing->GetTempDirectory()) + "/temp.vtp").c_str());
  writer->SetInputDataObject(pd);
  writer->Update();

  vtkNew<vtkXMLPolyDataReader> reader;
  reader->SetFileName(writer->GetFileName());
  reader->Update();

  return EXIT_SUCCESS;
}

// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDebugLeaks.h"
#include "vtkFileResourceStream.h"
#include "vtkOBJReader.h"

#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkTestUtilities.h"

//------------------------------------------------------------------------------
int TestOBJReaderMultiline(int argc, char* argv[])
{
  // Create the reader.
  vtkNew<vtkFileResourceStream> file;

  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/obj_multiline.obj");
  file->Open(fname);
  delete[] fname;

  if (file->EndOfStream())
  {
    std::cerr << "Can not open test file Data/obj_multiline.obj" << std::endl;
    return EXIT_FAILURE;
  }

  vtkNew<vtkOBJReader> reader;
  reader->SetStream(file);
  reader->Update();

  vtkPolyData* data = reader->GetOutput();

  if (!data)
  {
    std::cerr << "Could not read data" << std::endl;
    return EXIT_FAILURE;
  }

  if (data->GetNumberOfPoints() != 3)
  {
    std::cerr << "Invalid number of points" << std::endl;
    return EXIT_FAILURE;
  }

  if (data->GetVerts()->GetNumberOfCells() != 2)
  {
    std::cerr << "Invalid number of verts" << std::endl;
    return EXIT_FAILURE;
  }

  vtkNew<vtkIdList> ids;
  data->GetVerts()->GetCellAtId(0, ids);
  if (ids->GetNumberOfIds() != 2 || ids->GetId(0) != 0 || ids->GetId(1) != 2)
  {
    std::cerr << "Invalid vert cell (0)" << std::endl;
    return EXIT_FAILURE;
  }

  data->GetVerts()->GetCellAtId(1, ids);
  if (ids->GetNumberOfIds() != 1 || ids->GetId(0) != 1)
  {
    std::cerr << "Invalid vert cell (1)" << std::endl;
    return EXIT_FAILURE;
  }

  if (data->GetNumberOfLines() != 2)
  {
    std::cerr << "Invalid number of lines" << std::endl;
    return EXIT_FAILURE;
  }

  data->GetLines()->GetCellAtId(0, ids);
  if (ids->GetNumberOfIds() != 3 || ids->GetId(0) != 0 || ids->GetId(1) != 1 || ids->GetId(2) != 2)
  {
    std::cerr << "Invalid line cell (0)" << std::endl;
    return EXIT_FAILURE;
  }

  data->GetLines()->GetCellAtId(1, ids);
  if (ids->GetNumberOfIds() != 2 || ids->GetId(0) != 0 || ids->GetId(1) != 2)
  {
    std::cerr << "Invalid line cell (1)" << std::endl;
    return EXIT_FAILURE;
  }

  if (data->GetNumberOfPolys() != 1)
  {
    std::cerr << "Invalid number of polys" << std::endl;
    return EXIT_FAILURE;
  }

  data->GetPolys()->GetCellAtId(0, ids);
  if (ids->GetNumberOfIds() != 3 || ids->GetId(0) != 0 || ids->GetId(1) != 1 || ids->GetId(2) != 2)
  {
    std::cerr << "Invalid poly cell (0)" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

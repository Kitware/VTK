// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// .NAME Test of vtkOFFReader to read in a simple OFF file
// .SECTION Description
//

#include <vtkCellData.h>
#include <vtkMemoryResourceStream.h>
#include <vtkNew.h>
#include <vtkOFFReader.h>
#include <vtkPointData.h>
#include <vtkTestUtilities.h>

#include <cmath>

//------------------------------------------------------------------------------
int TestOFFReader(int /*argc*/, char** const /*argv*/)
{
  // a simple unit cube mesh with 3 quadrilateral and 6 triangular faces
  static const char* off_file_contents = "OFF\n"
                                         "# a simple unit cube mesh with 9 faces\n"
                                         "8 9 0\n"
                                         "# the set of 8 vertex\n"
                                         "0 0 0\n"
                                         "1 0 0\n"
                                         "0 1 0\n"
                                         "1 1 0\n"
                                         "0 0 1\n"
                                         "1 0 1\n"
                                         "0 1 1\n"
                                         "1 1 1\n"
                                         "# 3 quadrilateral faces\n"
                                         "4 0 1 3 2\n"
                                         "4 4 5 7 6\n"
                                         "4 0 1 5 4\n"
                                         "# 6 triangular faces\n"
                                         "3 2 3 7\n"
                                         "3 7 6 2\n"
                                         "3 0 2 6\n"
                                         "3 6 4 0\n"
                                         "3 1 3 7\n"
                                         "3 7 5 1\n";

  // create a memory resource stream for the file contents
  auto memStream = vtkSmartPointer<vtkMemoryResourceStream>::New();
  memStream->SetBuffer(std::string(off_file_contents));

  // create the reader.
  vtkNew<vtkOFFReader> reader;
  reader->SetStream(memStream);
  reader->Update();

  // get output
  vtkPolyData* data = reader->GetOutput();

  if (!data)
  {
    std::cerr << "Could not read data" << std::endl;
    return EXIT_FAILURE;
  }

  // we should have 8 points
  if (data->GetNumberOfPoints() != 8)
  {
    std::cerr << "Invalid number of points" << std::endl;
    return EXIT_FAILURE;
  }

  // explicitly test a few points
  double x[3];
  data->GetPoint(3, x); // 1 1 0
  x[0] -= 1.0;
  x[1] -= 1.0;
  if (x[0] * x[0] + x[1] * x[1] + x[2] * x[2] > 1E-5)
  {
    std::cerr << "Invalid point coordinates for point 3" << std::endl;
    return EXIT_FAILURE;
  }
  data->GetPoint(5, x); // 1 0 1
  x[0] -= 1.0;
  x[2] -= 1.0;
  if (x[0] * x[0] + x[1] * x[1] + x[2] * x[2] > 1E-5)
  {
    std::cerr << "Invalid point coordinates for point 5" << std::endl;
    return EXIT_FAILURE;
  }
  data->GetPoint(7, x); // 1 1 1
  x[0] -= 1.0;
  x[1] -= 1.0;
  x[2] -= 1.0;
  if (x[0] * x[0] + x[1] * x[1] + x[2] * x[2] > 1E-5)
  {
    std::cerr << "Invalid point coordinates for point 7" << std::endl;
    return EXIT_FAILURE;
  }

  // we should have 9 polygons
  if (data->GetNumberOfPolys() != 9)
  {
    std::cerr << "Invalid number of polygons" << std::endl;
    return EXIT_FAILURE;
  }

  // explicitly test two quads
  vtkNew<vtkIdList> ids;
  data->GetPolys()->GetCellAtId(0, ids);
  if (ids->GetNumberOfIds() != 4 || ids->GetId(0) != 0 || ids->GetId(1) != 1 ||
    ids->GetId(2) != 3 || ids->GetId(3) != 2)
  {
    std::cerr << "Invalid indices for polygon 0" << std::endl;
    return EXIT_FAILURE;
  }
  data->GetPolys()->GetCellAtId(2, ids);
  if (ids->GetNumberOfIds() != 4 || ids->GetId(0) != 0 || ids->GetId(1) != 1 ||
    ids->GetId(2) != 5 || ids->GetId(3) != 4)
  {
    std::cerr << "Invalid indices for polygon 2" << std::endl;
    return EXIT_FAILURE;
  }

  // test two triangles
  data->GetPolys()->GetCellAtId(4, ids);
  if (ids->GetNumberOfIds() != 3 || ids->GetId(0) != 7 || ids->GetId(1) != 6 || ids->GetId(2) != 2)
  {
    std::cerr << "Invalid indices for polygon 4" << std::endl;
    return EXIT_FAILURE;
  }
  data->GetPolys()->GetCellAtId(7, ids);
  if (ids->GetNumberOfIds() != 3 || ids->GetId(0) != 1 || ids->GetId(1) != 3 || ids->GetId(2) != 7)
  {
    std::cerr << "Invalid indices for polygon 7" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

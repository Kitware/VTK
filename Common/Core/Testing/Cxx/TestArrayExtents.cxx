// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include <vtkArrayCoordinates.h>
#include <vtkArrayExtents.h>

#include "vtkSetGet.h"
#include <iostream>
#include <sstream>
#include <stdexcept>

#define test_expression(expression)                                                                \
  do                                                                                               \
  {                                                                                                \
    if (!(expression))                                                                             \
    {                                                                                              \
      std::ostringstream buffer;                                                                   \
      buffer << "Expression failed at line " << __LINE__ << ": " << #expression;                   \
      throw std::runtime_error(buffer.str());                                                      \
    }                                                                                              \
  } while (false)

int TestArrayExtents(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  try
  {
    vtkArrayExtents slice(vtkArrayRange(2, 4), vtkArrayRange(6, 9));

    test_expression(slice.GetDimensions() == 2);
    test_expression(slice[0].GetSize() == 2);
    test_expression(slice[1].GetSize() == 3);
    test_expression(slice.GetSize() == 6);

    vtkArrayCoordinates coordinates;
    for (vtkArrayExtents::SizeT n = 0; n != slice.GetSize(); ++n)
    {
      slice.GetLeftToRightCoordinatesN(n, coordinates);
      cerr << coordinates << endl;
    }

    slice.GetLeftToRightCoordinatesN(0, coordinates);
    test_expression(coordinates == vtkArrayCoordinates(2, 6));
    slice.GetLeftToRightCoordinatesN(1, coordinates);
    test_expression(coordinates == vtkArrayCoordinates(3, 6));
    slice.GetLeftToRightCoordinatesN(2, coordinates);
    test_expression(coordinates == vtkArrayCoordinates(2, 7));
    slice.GetLeftToRightCoordinatesN(3, coordinates);
    test_expression(coordinates == vtkArrayCoordinates(3, 7));
    slice.GetLeftToRightCoordinatesN(4, coordinates);
    test_expression(coordinates == vtkArrayCoordinates(2, 8));
    slice.GetLeftToRightCoordinatesN(5, coordinates);
    test_expression(coordinates == vtkArrayCoordinates(3, 8));

    test_expression(slice.Contains(vtkArrayCoordinates(3, 7)));
    test_expression(!slice.Contains(vtkArrayCoordinates(1, 7)));

    return 0;
  }
  catch (std::exception& e)
  {
    cerr << e.what() << endl;
    return 1;
  }
}

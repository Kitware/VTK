/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ArraySlice.cxx
  
-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkArraySlice.h>

#include <vtksys/ios/iostream>
#include <vtksys/ios/sstream>
#include <vtksys/stl/stdexcept>
#include "vtkSetGet.h"

#define test_expression(expression) \
{ \
  if(!(expression)) \
    { \
    vtkstd::ostringstream buffer; \
    buffer << "Expression failed at line " << __LINE__ << ": " << #expression; \
    throw vtkstd::runtime_error(buffer.str()); \
    } \
}

int ArraySlice(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  try
    {
    vtkArraySlice slice(vtkArrayRange(2, 4), vtkArrayRange(6, 9));

    test_expression(slice.GetDimensions() == 2);
    test_expression(slice.GetExtents()[0] == 2);
    test_expression(slice.GetExtents()[1] == 3);
    test_expression(slice.GetExtents().GetSize() == 6);

    for(vtkIdType n = 0; n != slice.GetExtents().GetSize(); ++n)
      cerr << slice.GetCoordinatesN(n) << endl;

    test_expression(slice.GetCoordinatesN(0) == vtkArrayCoordinates(2, 6));
    test_expression(slice.GetCoordinatesN(1) == vtkArrayCoordinates(2, 7));
    test_expression(slice.GetCoordinatesN(2) == vtkArrayCoordinates(2, 8));
    test_expression(slice.GetCoordinatesN(3) == vtkArrayCoordinates(3, 6));

    return 0;
    }
  catch(vtkstd::exception& e)
    {
    cerr << e.what() << endl;
    return 1;
    }
}


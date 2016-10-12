/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestRect.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRect.h"

namespace {

//----------------------------------------------------------------------------
template<class T>
int TestAddPoint(vtkRect<T> & expandRect,
                 T x, T y,
                 const vtkRect<T> & expected)
{
  int returnValue = 0;

  std::cout << "Adding point (" << x << ", " << y << ") to rect "
            << expandRect << " ... ";

  expandRect.AddPoint(x, y);

  if (expandRect.GetX() != expected.GetX())
  {
    returnValue = EXIT_FAILURE;
    std::cout << "AddPoint()/GetX() ";
  }

  if (expandRect.GetY() != expected.GetY())
  {
    returnValue = EXIT_FAILURE;
    std::cout << "AddPoint()/GetY() ";
  }

  if (expandRect.GetWidth() != expected.GetWidth())
  {
    returnValue = EXIT_FAILURE;
    std::cout << "AddPoint()/GetWidth() ";
  }

  if (expandRect.GetHeight() != expected.GetHeight())
  {
    returnValue = EXIT_FAILURE;
    std::cout << "AddPoint()/GetHeight() ";
  }

  if (returnValue != EXIT_SUCCESS)
  {
    std::cout << "failed. Expected " << expected << ", got "
              << expandRect << "." << std::endl;
  }
  else
  {
    std::cout << "passed." << std::endl;
  }

  return returnValue;
}


//----------------------------------------------------------------------------
template<class T>
int TestAddRect(vtkRect<T> & expandRect,
                vtkRect<T> & addRect,
                const vtkRect<T> & expected)
{
  int returnValue = 0;

  std::cout << "Adding rect " << addRect << " to " << expandRect << " ... ";

  expandRect.AddRect(addRect);

  if (expandRect.GetX() != expected.GetX())
  {
    returnValue = EXIT_FAILURE;
    std::cout << "AddRect()/GetX() ";
  }

  if (expandRect.GetY() != expected.GetY())
  {
    returnValue = EXIT_FAILURE;
    std::cout << "AddRect()/GetY() ";
  }

  if (expandRect.GetWidth() != expected.GetWidth())
  {
    returnValue = EXIT_FAILURE;
    std::cout << "AddRect()/GetWidth() ";
  }

  if (expandRect.GetHeight() != expected.GetHeight())
  {
    returnValue = EXIT_FAILURE;
    std::cout << "AddRect()/GetHeight() ";
  }

  if (returnValue != EXIT_SUCCESS)
  {
    std::cout << "failed. Expected " << expected << ", got "
              << expandRect << "." << std::endl;
  }
  else
  {
    std::cout << "passed." << std::endl;
  }

  return returnValue;
}

} // end anonymous namespace

//----------------------------------------------------------------------------
int TestRect(int, char *[])
{
  int result = 0;

  // Test constructor/getter agreement ---------------------------------------
  vtkRectf rectf(2.0f, 3.0f, 4.0f, 5.0f);
  if (rectf.GetX() != 2.0f)
  {
    std::cout << "GetX() failed\n";
    std::cout << rectf << std::endl;
    return EXIT_FAILURE;
  }

  if (rectf.GetY() != 3.0f)
  {
    std::cout << "GetY() failed\n";
    std::cout << rectf << std::endl;
    return EXIT_FAILURE;
  }

  if (rectf.GetWidth() != 4.0f)
  {
    std::cout << "GetWidth() failed\n";
    std::cout << rectf << std::endl;
    return EXIT_FAILURE;
  }

  if (rectf.GetHeight() != 5.0f)
  {
    std::cout << "GetHeight() failed\n";
    std::cout << rectf << std::endl;
    return EXIT_FAILURE;
  }

  // Test Setters/getters ----------------------------------------------------
  rectf.SetX(1.0f);
  if (rectf.GetX() != 1.0f)
  {
    std::cout << "SetX()/GetX() failed\n";
    std::cout << rectf << std::endl;
    return EXIT_FAILURE;
  }

  rectf.SetY(8.0f);
  if (rectf.GetY() != 8.0f)
  {
    std::cout << "SetY()/GetY() failed\n";
    std::cout << rectf << std::endl;
    return EXIT_FAILURE;
  }

  rectf.SetWidth(7.0f);
  if (rectf.GetWidth() != 7.0f)
  {
    std::cout << "SetWidth()/GetWidth() failed\n";
    std::cout << rectf << std::endl;
    return EXIT_FAILURE;
  }

  rectf.SetHeight(9.0f);
  if (rectf.GetHeight() != 9.0f)
  {
    std::cout << "SetHeight()/GetHeight() failed\n";
    std::cout << rectf << std::endl;
    return EXIT_FAILURE;
  }

  // Test AddPoint() ----------------------------------------------------------
  vtkRectd expectedRect;
  vtkRectd expandRect = vtkRectd(0.0, 0.0, 0.0, 0.0);

  expectedRect = vtkRectd(-1.0, 0.0, 1.0, 1.0);
  result += TestAddPoint(expandRect, -1.0, 1.0, expectedRect);

  expectedRect = vtkRectd(-1.0, -3.0, 3.0, 4.0);
  result += TestAddPoint(expandRect, 2.0, -3.0, expectedRect);

  // Test AddRect() -----------------------------------------------------------
  vtkRectd addRect;

  // These five cases should exercise all the branches in vtkRect::AddRect().
  expandRect   = vtkRectd(0, 0, 4, 4);
  addRect      = vtkRectd(-1, 3, 2, 2);
  expectedRect = vtkRectd(-1, 0, 5, 5);
  result += TestAddRect(expandRect, addRect, expectedRect);

  expandRect   = vtkRectd(0, 0, 4, 4);
  addRect      = vtkRectd(3, 0, 2, 4);
  expectedRect = vtkRectd(0, 0, 5, 4);
  result += TestAddRect(expandRect, addRect, expectedRect);

  expandRect   = vtkRectd(0, 0, 4, 4);
  addRect      = vtkRectd(0, -1, 4, 2);
  expectedRect = vtkRectd(0, -1, 4, 5);
  result += TestAddRect(expandRect, addRect, expectedRect);

  expandRect   = vtkRectd(0, 0, 4, 4);
  addRect      = vtkRectd(1, 1, 2, 2);
  expectedRect = vtkRectd(0, 0, 4, 4);
  result += TestAddRect(expandRect, addRect, expectedRect);

  // Test IntersectsWith() -----------------------------------------------------
  vtkRecti recti(2, 3, 2, 1);
  vtkRecti doesntIntersect(-1, -2, 3, 4);
  if (recti.IntersectsWith(doesntIntersect))
  {
    std::cout << "Should not have intersected\n";
    std::cout << "recti:\n";
    std::cout << recti << "\n";
    std::cout << "doesntIntersect:\n";
    std::cout << doesntIntersect << "\n";
    return EXIT_FAILURE;
  }

  vtkRecti intersects(3, 2, 3, 4);
  if (!recti.IntersectsWith(intersects))
  {
    std::cout << "Should have intersected\n";
    std::cout << "recti:\n";
    std::cout << recti << "\n";
    std::cout << "intersect:\n";
    std::cout << intersects << "\n";
  }

  if (result != EXIT_SUCCESS)
  {
    result = EXIT_FAILURE;
  }

  return result;
}

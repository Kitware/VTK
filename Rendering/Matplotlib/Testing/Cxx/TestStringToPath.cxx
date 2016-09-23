/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestStringToPath.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMathTextUtilities.h"

#include "vtkContext2D.h"
#include "vtkContextItem.h"
#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkIntArray.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPath.h"
#include "vtkPen.h"
#include "vtkPoints.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTextProperty.h"

//----------------------------------------------------------------------------
class StringToPathContextTest : public vtkContextItem
{
public:
  static StringToPathContextTest *New();
  vtkTypeMacro(StringToPathContextTest, vtkContextItem);
  // Paint event for the chart, called whenever the chart needs to be drawn
  virtual bool Paint(vtkContext2D *painter);

  void SetPath(vtkPath *path) { this->Path = path; }

protected:
  vtkPath *Path;
};

//----------------------------------------------------------------------------
int TestStringToPath(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  // Set up a 2D context view, context test object and add it to the scene
  vtkNew<vtkContextView> view;
  view->GetRenderer()->SetBackground(1.0, 1.0, 1.0);
  view->GetRenderWindow()->SetSize(325, 150);
  vtkNew<StringToPathContextTest> test;
  view->GetScene()->AddItem(test.GetPointer());

  vtkNew<vtkPath> path;
  vtkNew<vtkTextProperty> tprop;

  vtkMathTextUtilities::GetInstance()->StringToPath(
        "$\\frac{-b\\pm\\sqrt{b^2-4ac}}{2a}$", path.GetPointer(),
        tprop.GetPointer(), view->GetRenderWindow()->GetDPI());

  test->SetPath(path.GetPointer());

  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();

  return EXIT_SUCCESS;
}

// Make our new derived class to draw a diagram
vtkStandardNewMacro(StringToPathContextTest)

// This function aims to test the primitives provided by the 2D API.
bool StringToPathContextTest::Paint(vtkContext2D *painter)
{
  // RGB color lookup table by path point code:
  double color [4][3];
  color[vtkPath::MOVE_TO][0] = 1.0;
  color[vtkPath::MOVE_TO][1] = 0.0;
  color[vtkPath::MOVE_TO][2] = 0.0;
  color[vtkPath::LINE_TO][0] = 0.0;
  color[vtkPath::LINE_TO][1] = 1.0;
  color[vtkPath::LINE_TO][2] = 0.0;
  color[vtkPath::CONIC_CURVE][0] = 0.0;
  color[vtkPath::CONIC_CURVE][1] = 0.0;
  color[vtkPath::CONIC_CURVE][2] = 1.0;
  color[vtkPath::CUBIC_CURVE][0] = 1.0;
  color[vtkPath::CUBIC_CURVE][1] = 0.0;
  color[vtkPath::CUBIC_CURVE][2] = 1.0;

  vtkPoints *points = this->Path->GetPoints();
  vtkIntArray *codes = this->Path->GetCodes();

  if (points->GetNumberOfPoints() != codes->GetNumberOfTuples())
  {
    return false;
  }

  // scaling factor and offset to ensure that the points will fit the view:
  double scale = 5.16591;
  double offset = 20.0;

  // Draw the control points, colored by codes:
  double point[3];
  painter->GetPen()->SetWidth(2);
  for (vtkIdType i = 0; i < points->GetNumberOfPoints(); ++i)
  {
    points->GetPoint(i, point);
    int code = codes->GetValue(i);

    painter->GetPen()->SetColorF(color[code]);
    painter->DrawPoint(point[0]*scale + offset, point[1]*scale + offset);
  }

  return true;
}

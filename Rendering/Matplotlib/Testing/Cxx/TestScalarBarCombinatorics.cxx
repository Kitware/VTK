/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestScalarBarCombinatorics.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkActor.h"
#include "vtkBandedPolyDataContourFilter.h"
#include "vtkCommand.h"
#include "vtkColorSeries.h"
#include "vtkDataArray.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkLookupTable.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiBlockPLOT3DReader.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkScalarBarActor.h"
#include "vtkSmartPointer.h"
#include "vtkStructuredGridGeometryFilter.h"
#include "vtkStructuredGrid.h"
#include "vtkTestUtilities.h"
#include "vtkTextProperty.h"
#include "vtkTesting.h"

#include <stdlib.h> // for atof

struct vtkScalarBarTestCondition
{
  const char* Title;
  int Orientation;
  int TextPosition;
  int DrawAnnotations;
  int DrawNanAnnotation;
  int IndexedLookup;
  int FixedAnnotationLeaderLineColor;
  double Position[2];
  double Position2[2];
  int ProcessEvents;
  int Enabled;
} conditions[] = {
  {"$T_1$", VTK_ORIENT_HORIZONTAL, vtkScalarBarActor::PrecedeScalarBar, 1, 1, 1, 0, {0.000, 0.015}, {0.400, 0.135}, 1, 1},
  {"$T_2$", VTK_ORIENT_HORIZONTAL, vtkScalarBarActor::PrecedeScalarBar, 1, 0, 1, 1, {0.000, 0.230}, {0.400, 0.146}, 1, 1},
  {"$T_3$", VTK_ORIENT_HORIZONTAL, vtkScalarBarActor::SucceedScalarBar, 1, 1, 1, 1, {0.000, 0.850}, {0.630, 0.154}, 1, 1},
  {"$T_4$", VTK_ORIENT_VERTICAL,   vtkScalarBarActor::PrecedeScalarBar, 1, 1, 1, 0, {0.799, 0.032}, {0.061, 0.794}, 1, 1},
  {"$T_5$", VTK_ORIENT_VERTICAL,   vtkScalarBarActor::PrecedeScalarBar, 1, 0, 1, 1, {0.893, 0.036}, {0.052, 0.752}, 1, 1},
  {"$T_6$", VTK_ORIENT_VERTICAL,   vtkScalarBarActor::SucceedScalarBar, 1, 1, 1, 1, {0.792, 0.081}, {0.061, 0.617}, 1, 1},
  {"$T_7$", VTK_ORIENT_VERTICAL,   vtkScalarBarActor::SucceedScalarBar, 1, 1, 0, 0, {0.646, 0.061}, {0.084, 0.714}, 1, 1},
  {"$T_8$", VTK_ORIENT_HORIZONTAL, vtkScalarBarActor::SucceedScalarBar, 0, 1, 0, 1, {0.076, 0.535}, {0.313, 0.225}, 1, 1},
};

static vtkSmartPointer<vtkScalarBarActor> CreateScalarBar(
  vtkScalarBarTestCondition& cond, vtkScalarsToColors* idxLut, vtkScalarsToColors* conLut, vtkRenderer* ren)
{
  vtkNew<vtkScalarBarActor> sba;
  sba->SetTitle(cond.Title);
  sba->SetLookupTable(cond.IndexedLookup ? idxLut : conLut);
  sba->SetOrientation(cond.Orientation);
  sba->SetTextPosition(cond.TextPosition);
  sba->SetDrawAnnotations(cond.DrawAnnotations);
  sba->SetDrawNanAnnotation(cond.DrawNanAnnotation);
  sba->SetFixedAnnotationLeaderLineColor(cond.FixedAnnotationLeaderLineColor);
  sba->SetPosition(cond.Position[0], cond.Position[1]);
  sba->SetPosition2(cond.Position2[0], cond.Position2[1]);
  ren->AddActor(sba.GetPointer());
  return sba.GetPointer();
}

int TestScalarBarCombinatorics(int argc, char* argv[])
{
  vtkTesting* t = vtkTesting::New();
  double threshold = 10.;
  for (int cc = 1; cc < argc; ++cc)
    {
    if (argv[cc][0] == '-' && argv[cc][1] == 'E' && cc < argc - 1)
      {
      threshold = atof(argv[++cc]);
      continue;
      }
    t->AddArgument(argv[cc]);
    }

  vtkNew<vtkRenderer> ren1;
  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(ren1.GetPointer());

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());

  vtkNew<vtkLookupTable> lutA;
  vtkNew<vtkLookupTable> lutB;
  // Create a grid of scalar bars
  int numBars = static_cast<int>(sizeof(conditions) / sizeof(conditions[0]));
  std::vector<vtkSmartPointer<vtkScalarBarActor> > actors;
  actors.reserve(numBars);
  for (int c = 0; c < numBars; ++c)
    {
    actors.push_back(CreateScalarBar(conditions[c], lutA.GetPointer(), lutB.GetPointer(), ren1.GetPointer()));
    }

  // Add the actors to the renderer, set the background and size
  //
  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(600, 300);

  vtkNew<vtkColorSeries> pal;
  pal->SetColorSchemeByName("Brewer Sequential Blue-Green (5)");
  pal->BuildLookupTable(lutB.GetPointer());
  lutB->IndexedLookupOff();
  lutB->Build();
  lutB->SetAnnotation(5.00, "Just Wow");
  lutB->SetAnnotation(4.00, "Super-Special");
  lutB->SetAnnotation(3.00, "Amazingly Special");
  lutB->SetAnnotation(1.00, "Special");
  lutB->SetAnnotation(0.00, "Special $\\cap$ This $= \\emptyset$");
  lutB->SetRange(0., 4.); // Force "Just Wow" to be omitted from rendering.
  lutB->Build();

  // Now make a second set of annotations with an even number of entries (10).
  // This tests another branch of the annotation label positioning code.
  pal->SetColorSchemeByName("Brewer Diverging Purple-Orange (10)");
  pal->BuildLookupTable(lutA.GetPointer());
  lutA->SetAnnotation(5.00, "A");
  lutA->SetAnnotation(4.00, "B");
  lutA->SetAnnotation(3.00, "C");
  lutA->SetAnnotation(2.00, "D");
  lutA->SetAnnotation(1.00, ""); // Test empty label omission
  lutA->SetAnnotation(0.00, "F");
  lutA->SetAnnotation(6.00, "G");
  lutA->SetAnnotation(7.00, "H");
  lutA->SetAnnotation(8.00, "I");
  lutA->SetAnnotation(9.00, ""); // Test empty label omission

  // render the image
  iren->Initialize();
  renWin->Render();
  t->SetRenderWindow(renWin.GetPointer());
  int res = t->RegressionTest(threshold);
  t->Delete();

  iren->Start();

  return res == vtkTesting::PASSED ? 0 : 1;
}

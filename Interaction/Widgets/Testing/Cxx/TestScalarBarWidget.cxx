// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSmartPointer.h"

#include "vtkActor.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiBlockPLOT3DReader.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkScalarBarActor.h"
#include "vtkScalarBarWidget.h"
#include "vtkStructuredGridGeometryFilter.h"
#include "vtkTesting.h"

#include "vtkTestUtilities.h"

constexpr char TSBWeventLog[] = "# StreamVersion 1\n"
                                "CharEvent 153 168 0 0 105 1 i\n"
                                "KeyReleaseEvent 153 168 0 0 105 1 i\n"

                                "MouseMoveEvent 254 151 0 0 0 0 i\n"
                                "LeftButtonPressEvent 254 151 0 0 0 0 i\n"
                                "MouseMoveEvent 140 269 0 0 0 0 i\n"
                                "LeftButtonReleaseEvent 140 269 0 0 0 0 i\n"

                                "MouseMoveEvent 38 253 0 0 0 0 i\n"
                                "LeftButtonPressEvent 38 253 0 0 0 0 i\n"
                                "MouseMoveEvent 139 258 0 0 0 0 i\n"
                                "LeftButtonReleaseEvent 139 258 0 0 0 0 i\n"

                                "MouseMoveEvent 141 268 0 0 0 0 i\n"
                                "LeftButtonPressEvent 141 268 0 0 0 0 i\n"
                                "MouseMoveEvent 59 268 0 0 0 0 i\n"
                                "LeftButtonReleaseEvent 59 268 0 0 0 0 i\n"

                                "MouseMoveEvent 155 249 0 0 0 0 i\n"
                                "LeftButtonPressEvent 155 249 0 0 0 0 i\n"
                                "MouseMoveEvent 163 125 0 0 0 0 i\n"
                                "LeftButtonReleaseEvent 163 125 0 0 0 0 i\n"

                                "MouseMoveEvent 63 147 0 0 0 0 i\n"
                                "LeftButtonPressEvent 63 147 0 0 0 0 i\n"
                                "MouseMoveEvent 163 146 0 0 0 0 i\n"
                                "LeftButtonReleaseEvent 163 146 0 0 0 0 i\n";

int TestScalarBarWidget(int argc, char* argv[])
{
  std::string fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/combxyz.bin");
  std::string fname2 = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/combq.bin");

  // Start by loading some data.
  vtkSmartPointer<vtkMultiBlockPLOT3DReader> pl3d =
    vtkSmartPointer<vtkMultiBlockPLOT3DReader>::New();
  pl3d->SetXYZFileName(fname.c_str());
  pl3d->SetQFileName(fname2.c_str());
  pl3d->SetScalarFunctionNumber(100);
  pl3d->SetVectorFunctionNumber(202);
  pl3d->Update();
  vtkDataSet* pl3d_block0 = vtkDataSet::SafeDownCast(pl3d->GetOutput()->GetBlock(0));

  // An outline is shown for context.
  vtkSmartPointer<vtkStructuredGridGeometryFilter> outline =
    vtkSmartPointer<vtkStructuredGridGeometryFilter>::New();
  outline->SetInputData(pl3d_block0);
  outline->SetExtent(0, 100, 0, 100, 9, 9);

  vtkSmartPointer<vtkPolyDataMapper> outlineMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  outlineMapper->SetInputConnection(outline->GetOutputPort());

  vtkSmartPointer<vtkActor> outlineActor = vtkSmartPointer<vtkActor>::New();
  outlineActor->SetMapper(outlineMapper);

  // Create the RenderWindow, Renderer and both Actors
  //
  vtkSmartPointer<vtkRenderer> ren1 = vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renWin = vtkSmartPointer<vtkRenderWindow>::New();
  renWin->AddRenderer(ren1);

  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  vtkSmartPointer<vtkScalarBarWidget> scalarWidget = vtkSmartPointer<vtkScalarBarWidget>::New();
  scalarWidget->SetInteractor(iren);
  scalarWidget->GetScalarBarActor()->SetTitle("Temperature");
  scalarWidget->GetScalarBarActor()->SetLookupTable(outlineMapper->GetLookupTable());

  ren1->AddActor(outlineActor);

  // Add the actors to the renderer, set the background and size
  //
  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(300, 300);

  return vtkTesting::InteractorEventLoop(argc, argv, iren, TSBWeventLog);
}

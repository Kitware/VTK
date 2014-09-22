/*=========================================================================

Copyright (c) 1998-2005 Kitware Inc. 28 Corporate Drive, Suite 204,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced,
distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES,
INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO
PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkSmartPointer.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkInteractorStyleSwitch.h"
#include "vtkCmbGlyphPointSource.h"
#include "vtkGlyph3DMapper.h"
#include "vtkCylinderSource.h"
#include "vtkCamera.h"
#include <string>
#include "vtkTesting.h"
#include "vtkTimerLog.h"
#include "vtkNew.h"

#include "vtkCmbGlyphPointSource.cxx"

//----------------------------------------------------------------------------
// Tests Glyph Point Source's individual property settings
int vtkCmbGlyphMappingTest3(int argc, char *argv[])
{
  int n = 300;
  vtkSmartPointer<vtkTesting> testHelper = vtkSmartPointer<vtkTesting>::New();

  testHelper->AddArguments(argc,const_cast<const char **>(argv));

  vtkNew<vtkTimerLog> timer;
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(800,800);
  vtkNew<vtkRenderer> renderer;

  vtkNew<vtkRenderWindowInteractor> iren;
  vtkNew<vtkInteractorStyleSwitch> style;
  style->SetCurrentStyleToTrackballCamera();
  iren->SetInteractorStyle( style.GetPointer() );

  iren->SetRenderWindow(renWin.GetPointer());
  renWin->AddRenderer( renderer.GetPointer() );

  vtkNew<vtkCmbGlyphPointSource> points;
  // Lets create some points
  int i, j, vis;
  double x, y, s, delta = 1.0;
  double start = -0.5 * delta * n;
  vtkIdType id;
  int k;
  int pointCount = 0;
  for (j = 0, y = start; j < n; j++, y+=delta)
    {
    for (i = 0, x = start; i < n; i++, x+=delta)
      {
      vis = 1;
      s = 10;
      k = i+j;
      if ((k % 2) == 0)
        {
        id = points->InsertNextPoint(x, 0.0, y);
        points->SetGlyphType(id, 0);
        points->SetColor(id, 1.0, 0.0, 0.0, 1.0);
        points->SetVisibility(id, vis);
        points->SetScale(id, 1.0, s, 1.0);

        id = points->InsertNextPoint(x, 0.0, y);
        points->SetGlyphType(id, 1);
        points->SetColor(id, 1.0, 1.0, 0.0, 1.0);
        points->SetVisibility(id, vis);
        points->SetScale(id, 1, s*.99, 1);

        pointCount += 2;
        }
      else if ((k % 3) == 0)
        {
        id = points->InsertNextPoint(x, 0.0, y);
        points->SetGlyphType(id, 3);
        points->SetColor(id, 1.0, 0.7, 0.0, 1.0);
        points->SetVisibility(id, vis);
        points->SetScale(id, 1, s, 1);
        pointCount++;
        }
      else
        {
        id = points->InsertNextPoint(x, 0.0, y);
        points->SetGlyphType(id, 2);
        points->SetColor(id, 0.5, 1.0, 0.5, 1.0);
        points->SetVisibility(id, vis);
        points->SetScale(id, 1, s, 1);
        pointCount++;
        }
      }
    }

  vtkNew<vtkCylinderSource> cyl;
  cyl->CappingOn();
  cyl->SetRadius(0.2);
  cyl->SetResolution(18);
  cyl->Update();

  vtkNew<vtkCylinderSource> cyl1;
  cyl1->CappingOn();
  cyl1->SetRadius(0.4);
  cyl1->SetResolution(18);
  cyl1->Update();

  vtkNew<vtkCylinderSource> cyl2;
  cyl2->CappingOn();
  cyl2->SetRadius(0.4);
  cyl2->SetResolution(18);
  cyl2->Update();

  vtkNew<vtkCylinderSource> cyl3;
  cyl3->CappingOn();
  cyl3->SetRadius(0.3);
  cyl3->SetResolution(18);
  cyl3->Update();

  int polygons = pointCount*cyl->GetOutput()->GetPolys()->GetNumberOfCells();

  vtkNew<vtkGlyph3DMapper> mapper;
  mapper->SetSourceConnection(0, cyl->GetOutputPort() );
  mapper->SetSourceConnection(1, cyl1->GetOutputPort() );
  mapper->SetSourceConnection(2, cyl2->GetOutputPort() );
  mapper->SetSourceConnection(3, cyl3->GetOutputPort() );
  mapper->SetInputConnection(points->GetOutputPort());
  mapper->SetMaskArray("Visibility");
  mapper->SetOrientationArray("Orientation");
  mapper->SetScaleArray("Scaling");
  mapper->SetSourceIndexArray("GlyphType");
  mapper->SetMasking(true);
  if (!testHelper->IsFlagSpecified("-N"))
    {
    mapper->SetSourceIndexing(true);
    mapper->SetRange(0.0,3.0);
    }

  mapper->SetOrientationModeToRotation();
  mapper->SetScaleModeToScaleByVectorComponents();

  vtkNew<vtkActor> actor;
  actor->SetMapper( mapper.GetPointer() );
  renderer->AddViewProp( actor.GetPointer() );

  renderer->ResetCamera();

  timer->StartTimer();
  iren->Initialize();
  renWin->Render();
  timer->StopTimer();

  cout << "First frame time: " << timer->GetElapsedTime() << "\n";

  timer->StartTimer();
  for (i = 0; i <= 100; i++)
    {
    renderer->GetActiveCamera()->Elevation(0.9);
    renderer->GetActiveCamera()->Zoom(1.02);
    renWin->Render();
    }

  timer->StopTimer();
  double t =  timer->GetElapsedTime();
  cout << "Avg Frame time: " << t/100.0 << " Frame Rate: " << 100.0 / t << "\n";
  cout << " polygons: " << polygons << " Mpolys/sec: " << 100.0*polygons/(1000000.0*t) << "\n";
  int retVal = vtkTesting::PASSED;
  if (testHelper->IsFlagSpecified("-V"))
    {
    testHelper->SetRenderWindow(renWin.GetPointer());
    retVal = testHelper->RegressionTest(10);
    }

  if (testHelper->IsInteractiveModeSpecified())
    {
    iren->Start();
    }

  return (retVal == vtkTesting::PASSED) ? 0 : 1;
}

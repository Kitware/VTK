/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestValuePainter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test covers the ability for the value painter to draw arrays as
// colors such that the visible values can be recovered from the pixels.

#include <vtkActor.h>
#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkDoubleArray.h>
#include <vtkMath.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPainterPolyDataMapper.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSmartPointer.h>
#include <vtkValuePainter.h>
#include <vtkWindowToImageFilter.h>

#include <set>

#define TESTVP_MAX 10

void PrepArray(bool byName, bool drawCell, int arrayIndex, int arrayComponent,
               vtkDataSet *dataset, vtkDataArray *values, vtkValuePainter *painter,
               double *&minmax)
{
  if (drawCell)
  {
    if (arrayIndex > dataset->GetCellData()->GetNumberOfArrays())
    {
      arrayIndex = 0;
    }
    values = dataset->GetCellData()->GetArray(arrayIndex);
    if (arrayComponent > values->GetNumberOfComponents())
    {
      arrayComponent = 0;
    }
    cerr << "Drawing CELL " << values->GetName() << " [" << arrayComponent << "]" << endl;
    if (!byName)
    {
      painter->SetInputArrayToProcess(VTK_SCALAR_MODE_USE_CELL_FIELD_DATA, arrayIndex);
    }
    else
    {
      painter->SetInputArrayToProcess(VTK_SCALAR_MODE_USE_CELL_FIELD_DATA, values->GetName());
    }
    minmax = values->GetRange(arrayComponent);
  }
  else
  {
    if (arrayIndex > dataset->GetPointData()->GetNumberOfArrays())
    {
      arrayIndex = 0;
    }
    values = dataset->GetPointData()->GetArray(arrayIndex);
    if (arrayComponent > values->GetNumberOfComponents())
    {
      arrayComponent = 0;
    }
    cerr << "Drawing POINT " << values->GetName() << " [" << arrayComponent << "]" << endl;
    if (!byName)
    {
      painter->SetInputArrayToProcess(VTK_SCALAR_MODE_USE_POINT_FIELD_DATA, arrayIndex);
    }
    else
    {
      painter->SetInputArrayToProcess(VTK_SCALAR_MODE_USE_POINT_FIELD_DATA, values->GetName());
    }
    minmax = values->GetRange(arrayComponent);
  }
  painter->SetInputComponentToProcess(arrayComponent);
  painter->SetScalarRange(minmax[0], minmax[1]);
}

int TestValuePainter(int argc, char* argv[])
{

  bool byName = true;
  bool drawCell = true;
  unsigned int arrayIndex = 0;
  unsigned int arrayComponent = 0;
  bool interactive = false;

  for (int i = 0; i < argc; i++)
  {
    if (!strcmp(argv[i],"index"))
    {
      byName = false;
    }
    if (!strcmp(argv[i],"point"))
    {
      drawCell = false;
    }
    if (!strcmp(argv[i],"N"))
    {
      arrayIndex = atoi(argv[i+1]);
    }
    if (!strcmp(argv[i],"C"))
    {
      arrayComponent = atoi(argv[i+1]);
    }
    if (!strcmp(argv[i],"-I"))
    {
      interactive = true;
    }
  }

  vtkSmartPointer<vtkPolyData> dataset = vtkSmartPointer<vtkPolyData>::New();
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  dataset->SetPoints(points);
  vtkSmartPointer<vtkDoubleArray> scalars = vtkSmartPointer<vtkDoubleArray>::New();
  scalars->SetNumberOfComponents(1);
  scalars->SetName("Point Scalar Array 1");
  dataset->GetPointData()->AddArray(scalars);
  vtkSmartPointer<vtkDoubleArray> vectors = vtkSmartPointer<vtkDoubleArray>::New();
  vectors->SetNumberOfComponents(3);
  vectors->SetName("Point Vector Array 1");
  dataset->GetPointData()->AddArray(vectors);
  double vector[3];
  for (unsigned int i = 0; i < TESTVP_MAX; i++)
  {
    for (unsigned int j = 0; j < TESTVP_MAX; j++)
    {
      points->InsertNextPoint(i,j,0.0);
      scalars->InsertNextValue((double)i/TESTVP_MAX+10);
      vector[0] = sin((double)j/TESTVP_MAX*6.1418);
      vector[1] = 1.0;
      vector[2] = 1.0;
      vtkMath::Normalize(vector);
      vectors->InsertNextTuple3(vector[0],vector[1],vector[2]);
    }
  }

  vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();
  dataset->SetPolys(cells);
  scalars = vtkSmartPointer<vtkDoubleArray>::New();
  scalars->SetNumberOfComponents(1);
  scalars->SetName("Cell Scalar Array 1");
  dataset->GetCellData()->AddArray(scalars);
  vectors = vtkSmartPointer<vtkDoubleArray>::New();
  vectors->SetNumberOfComponents(3);
  vectors->SetName("Cell Vector Array 1");
  dataset->GetCellData()->AddArray(vectors);
  for (unsigned int i = 0; i < (TESTVP_MAX-1); i++)
  {
    for (unsigned int j = 0; j < (TESTVP_MAX-1); j++)
    {
      cells->InsertNextCell(4);
      cells->InsertCellPoint(i*TESTVP_MAX    +j);
      cells->InsertCellPoint(i*TESTVP_MAX    +j+1);
      cells->InsertCellPoint((i+1)*TESTVP_MAX+j+1);
      cells->InsertCellPoint((i+1)*TESTVP_MAX+j);

      scalars->InsertNextValue((double)i/(TESTVP_MAX-1)-10);
      vector[0] = sin((double)j/(TESTVP_MAX-1)*6.1418);
      vector[1] = 1.0;
      vector[2] = 1.0;
      vtkMath::Normalize(vector);
      vectors->InsertNextTuple3(vector[0],vector[1],vector[2]);
    }
  }

  vtkSmartPointer<vtkPainterPolyDataMapper> mapper =
    vtkSmartPointer<vtkPainterPolyDataMapper>::New();
  mapper->SetInputData(dataset);

  mapper->SetScalarModeToUsePointData();
  mapper->SelectColorArray(0);

  vtkSmartPointer<vtkValuePainter> painter = vtkSmartPointer<vtkValuePainter>::New();
  vtkDataArray *values = NULL;
  double *minmax;
  PrepArray(byName, drawCell, arrayIndex, arrayComponent,
            dataset, values, painter,
            minmax);

  double scale = minmax[1]-minmax[0];
  painter->SetInputComponentToProcess(arrayComponent);
  mapper->SetPainter(painter);

  vtkSmartPointer<vtkActor> actor =
    vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);

  vtkSmartPointer<vtkRenderer> renderer =
    vtkSmartPointer<vtkRenderer>::New();

  //manually set background to the "nothing" color
  renderer->SetBackground(0.0,0.0,0.0);
  renderer->GradientBackgroundOff();

  vtkSmartPointer<vtkRenderWindow> renderWindow =
    vtkSmartPointer<vtkRenderWindow>::New();
  renderWindow->AddRenderer(renderer);
  vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  renderWindowInteractor->SetRenderWindow(renderWindow);

  renderer->AddActor(actor);

  renderWindow->Render();

  //iterate to look for leaks and such
  for (int i = 0; i < 8; i++)
  {
    bool _byName = true;
    bool _drawCell = true;
    vtkFieldData *fd = dataset->GetCellData();
    if (i<4)
    {
      _byName = false;
    }
    if (i%2)
    {
      _drawCell = false;
      fd = dataset->GetPointData();
    }
    for (int j = 0; j < fd->GetNumberOfArrays(); j++)
    {
      for (int k = 0; k < fd->GetArray(j)->GetNumberOfComponents(); k++)
      {
        PrepArray(_byName, _drawCell, j, k, dataset, values, painter, minmax);
        renderWindow->Render();

        //std::string v;
        //cin >> v;
      }
    }
  }

  PrepArray(byName, drawCell, arrayIndex, arrayComponent,
            dataset, values, painter,
            minmax);
  renderWindow->Render();

  vtkSmartPointer<vtkWindowToImageFilter> grabber = vtkSmartPointer<vtkWindowToImageFilter>::New();
  grabber->SetInput(renderWindow);
  grabber->Update();
  vtkImageData *id = grabber->GetOutput();
  //id->PrintSelf(cerr, vtkIndent(0));

  vtkUnsignedCharArray *ar = vtkArrayDownCast<vtkUnsignedCharArray>(id->GetPointData()->GetArray("ImageScalars"));
  unsigned char *ptr = static_cast<unsigned char*>(ar->GetVoidPointer(0));
  std::set<double> found;
  double value;
  for (int i = 0; i < id->GetNumberOfPoints(); i++)
  {
    vtkValuePainter::ColorToValue(ptr, minmax[0], scale, value);
    if (found.find(value)==found.end())
    {
      found.insert(value);
      cerr << "READ "
           << std::hex
           << (int) ptr[0] << (int) ptr[1] << (int) ptr[2] << "\t"
           << std::dec
           << value << endl;
    }
    ptr+=3;
  }

  std::set<double>::iterator it;
  double min = VTK_DOUBLE_MAX;
  double max = VTK_DOUBLE_MIN;
  for (it = found.begin(); it != found.end(); ++it)
  {
    if (*it < min)
    {
      min = *it;
    }
    if (*it > max)
    {
      max = *it;
    }
  }
  bool fail = false;
  if (fabs(min - -10.0) > 0.12)
  {
    cerr << "ERROR min value not correct" << endl;
    fail = true;
  }
  if (fabs(max - -9.0) > 0.12)
  {
    cerr << "ERROR max value not correct" << endl;
    fail = true;
  }

  if (interactive)
  {
    renderWindowInteractor->Start();
  }

  return fail;
}

// VTK: Spiral with vtkTubeFilter
// Varying tube radius and independent RGB colors with an unsignedCharArray
// Contributed by Marcus Thamson

#include <vtkPolyData.h>
#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkDoubleArray.h>
#include <vtkPolyData.h>
#include <vtkPointData.h>

#include <vtkCell.h>
#include <vtkCellData.h>
#include <vtkDataSet.h>
#include <vtkDataSetAttributes.h>
#include <vtkProperty.h>
#include <vtkSmartPointer.h>
#include <vtkTubeFilter.h>

#include <vtkDataSetMapper.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkCamera.h>
#include <vtkInteractorStyleTrackballCamera.h>

#include <vtkMath.h>

int main (int, char *[])
{
  // Spiral tube
  double vX, vY, vZ;
  unsigned int nV = 256;      // No. of vertices
  unsigned int nCyc = 5;      // No. of spiral cycles
  double rT1 = 0.1, rT2 = 0.5;// Start/end tube radii
  double rS = 2;              // Spiral radius
  double h = 10;              // Height
  unsigned int nTv = 8;       // No. of surface elements for each tube vertex

  unsigned int i;

  // Create points and cells for the spiral
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  for(i = 0; i < nV; i++)
    {
    // Spiral coordinates
    vX = rS * cos(2 * vtkMath::Pi() * nCyc * i / (nV - 1));
    vY = rS * sin(2 * vtkMath::Pi() * nCyc * i / (nV - 1));
    vZ = h * i / nV;
    points->InsertPoint(i, vX, vY, vZ);
    }

  vtkSmartPointer<vtkCellArray> lines =
    vtkSmartPointer<vtkCellArray>::New();
  lines->InsertNextCell(nV);
  for (i = 0; i < nV; i++)
    {
    lines->InsertCellPoint(i);
    }

  vtkSmartPointer<vtkPolyData> polyData =
    vtkSmartPointer<vtkPolyData>::New();
  polyData->SetPoints(points);
  polyData->SetLines(lines);

  // Varying tube radius using sine-function
  vtkSmartPointer<vtkDoubleArray> tubeRadius =
    vtkSmartPointer<vtkDoubleArray>::New();
  tubeRadius->SetName("TubeRadius");
  tubeRadius->SetNumberOfTuples(nV);
  for (i=0 ;i<nV ; i++)
    {
    tubeRadius->SetTuple1(i,
                          rT1 + (rT2 - rT1) * sin(vtkMath::Pi() * i / (nV - 1)));
    }
  polyData->GetPointData()->AddArray(tubeRadius);
  polyData->GetPointData()->SetActiveScalars("TubeRadius");

  // RBG array (could add Alpha channel too I guess...)
  // Varying from blue to red
  vtkSmartPointer<vtkUnsignedCharArray> colors =
    vtkSmartPointer<vtkUnsignedCharArray>::New();
  colors->SetName("Colors");
  colors->SetNumberOfComponents(3);
  colors->SetNumberOfTuples(nV);
  for (i = 0; i < nV ;i++)
    {
    colors->InsertTuple3(i,
                       int(255 * i/ (nV - 1)),
                       0,
                       int(255 * (nV - 1 - i)/(nV - 1)) );
    }
  polyData->GetPointData()->AddArray(colors);

  vtkSmartPointer<vtkTubeFilter> tube
    = vtkSmartPointer<vtkTubeFilter>::New();
  tube->SetInput(polyData);
  tube->SetNumberOfSides(nTv);
  tube->SetVaryRadiusToVaryRadiusByAbsoluteScalar();

  vtkSmartPointer<vtkPolyDataMapper> mapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputConnection(tube->GetOutputPort());
  mapper->ScalarVisibilityOn();
  mapper->SetScalarModeToUsePointFieldData();
  mapper->SelectColorArray("Colors");

  vtkSmartPointer<vtkActor> actor =
    vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);

  vtkSmartPointer<vtkRenderer> renderer =
    vtkSmartPointer<vtkRenderer>::New();
  renderer->AddActor(actor);
  renderer->SetBackground(.2, .3, .4);

  // Make an oblique view
  renderer->GetActiveCamera()->Azimuth(30);
  renderer->GetActiveCamera()->Elevation(30);
  renderer->ResetCamera();

  vtkSmartPointer<vtkRenderWindow> renWin =
    vtkSmartPointer<vtkRenderWindow>::New();
  vtkSmartPointer<vtkRenderWindowInteractor>
    iren = vtkSmartPointer<vtkRenderWindowInteractor>::New();

  iren->SetRenderWindow(renWin);
  renWin->AddRenderer(renderer);
  renWin->SetSize(500, 500);
  renWin->Render();

  vtkSmartPointer<vtkInteractorStyleTrackballCamera> style =
    vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
  iren->SetInteractorStyle(style);

  iren->Start();

  return EXIT_SUCCESS;
}

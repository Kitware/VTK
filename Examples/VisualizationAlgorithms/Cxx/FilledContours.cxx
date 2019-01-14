#include <vtkSmartPointer.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkAppendPolyData.h>
#include <vtkClipPolyData.h>
#include <vtkCleanPolyData.h>
#include <vtkContourFilter.h>

#include <vtkFloatArray.h>
#include <vtkCellData.h>
#include <vtkPointData.h>
#include <vtkScalarsToColors.h>
#include <vtkLookupTable.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkActor.h>

#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>

#include <vector>

int main (int argc, char *argv[])
{
  if (argc < 3)
  {
    std::cerr << "Usage: " << argv[0] << " InputPolyDataFile(.vtp) NumberOfContours" << std::endl;
    return EXIT_FAILURE;
  }

  // Read the file
  vtkSmartPointer<vtkXMLPolyDataReader> reader =
    vtkSmartPointer<vtkXMLPolyDataReader>::New();

  reader->SetFileName( argv[1] );
  reader->Update(); // Update so that we can get the scalar range

  double scalarRange[2];
  reader->GetOutput()->GetPointData()->GetScalars()->GetRange(scalarRange);

  vtkSmartPointer<vtkAppendPolyData> appendFilledContours =
    vtkSmartPointer<vtkAppendPolyData>::New();

  // Check for a reasonable number of contours to avoid excessive
  // computation. Here we arbitrarily pick an upper limit of 1000
  int numberOfContours = atoi(argv[2]);
  if (numberOfContours > 1000)
  {
    std::cout << "ERROR: the number of contours " << numberOfContours << " exceeds 1000" << std::endl;
    return EXIT_FAILURE;
  }
  if (numberOfContours <= 0)
  {
    std::cout << "ERROR: the number of contours " << numberOfContours << " is <= 0" << std::endl;
    return EXIT_FAILURE;
  }

  double delta =
    (scalarRange[1] - scalarRange[0]) /
    static_cast<double> (numberOfContours - 1);

  // Keep the clippers alive
  std::vector<vtkSmartPointer<vtkClipPolyData> > clippersLo;
  std::vector<vtkSmartPointer<vtkClipPolyData> > clippersHi;

  for (int i = 0; i < numberOfContours; i++)
  {
    double valueLo = scalarRange[0] + static_cast<double> (i) * delta;
    double valueHi = scalarRange[0] + static_cast<double> (i + 1) * delta;
    clippersLo.push_back(vtkSmartPointer<vtkClipPolyData>::New());
    clippersLo[i]->SetValue(valueLo);
    if (i == 0)
    {
      clippersLo[i]->SetInputConnection(reader->GetOutputPort());
    }
    else
    {
      clippersLo[i]->SetInputConnection(clippersHi[i - 1]->GetOutputPort(1));
    }
    clippersLo[i]->InsideOutOff();
    clippersLo[i]->Update();

    clippersHi.push_back(vtkSmartPointer<vtkClipPolyData>::New());
    clippersHi[i]->SetValue(valueHi);
    clippersHi[i]->SetInputConnection(clippersLo[i]->GetOutputPort());
    clippersHi[i]->GenerateClippedOutputOn();
    clippersHi[i]->InsideOutOn();
    clippersHi[i]->Update();
    if (clippersHi[i]->GetOutput()->GetNumberOfCells() == 0)
    {
      continue;
    }

    vtkSmartPointer<vtkFloatArray> cd =
      vtkSmartPointer<vtkFloatArray>::New();
    cd->SetNumberOfComponents(1);
    cd->SetNumberOfTuples(clippersHi[i]->GetOutput()->GetNumberOfCells());
    cd->FillComponent(0, valueLo);

    clippersHi[i]->GetOutput()->GetCellData()->SetScalars(cd);
    appendFilledContours->AddInputConnection(clippersHi[i]->GetOutputPort());
  }

  vtkSmartPointer<vtkCleanPolyData> filledContours =
    vtkSmartPointer<vtkCleanPolyData>::New();
  filledContours->SetInputConnection(appendFilledContours->GetOutputPort());

  vtkSmartPointer<vtkLookupTable> lut =
    vtkSmartPointer<vtkLookupTable>::New();
  lut->SetNumberOfTableValues(numberOfContours + 1);
  lut->Build();
  vtkSmartPointer<vtkPolyDataMapper> contourMapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  contourMapper->SetInputConnection(filledContours->GetOutputPort());
  contourMapper->SetScalarRange(scalarRange[0], scalarRange[1]);
  contourMapper->SetScalarModeToUseCellData();
  contourMapper->SetLookupTable(lut);

  vtkSmartPointer<vtkActor> contourActor =
    vtkSmartPointer<vtkActor>::New();
  contourActor->SetMapper(contourMapper);
  contourActor->GetProperty()->SetInterpolationToFlat();

  vtkSmartPointer<vtkContourFilter> contours =
    vtkSmartPointer<vtkContourFilter>::New();
  contours->SetInputConnection(filledContours->GetOutputPort());
  contours->GenerateValues(numberOfContours, scalarRange[0], scalarRange[1]);

  vtkSmartPointer<vtkPolyDataMapper> contourLineMapperer =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  contourLineMapperer->SetInputConnection(contours->GetOutputPort());
  contourLineMapperer->SetScalarRange(scalarRange[0], scalarRange[1]);
  contourLineMapperer->ScalarVisibilityOff();

  vtkSmartPointer<vtkActor> contourLineActor =
    vtkSmartPointer<vtkActor>::New();
  contourLineActor->SetMapper(contourLineMapperer);
  contourLineActor->GetProperty()->SetLineWidth(2);

  // The usual renderer, render window and interactor
  vtkSmartPointer<vtkRenderer> ren1 =
    vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renWin =
    vtkSmartPointer<vtkRenderWindow>::New();
  vtkSmartPointer<vtkRenderWindowInteractor>
    iren = vtkSmartPointer<vtkRenderWindowInteractor>::New();

  ren1->SetBackground(.1, .2, .3);
  renWin->AddRenderer(ren1);
  iren->SetRenderWindow(renWin);

  // Add the actors
  ren1->AddActor(contourActor);
  ren1->AddActor(contourLineActor);

  // Begin interaction
  renWin->Render();
  iren->Start();

  return EXIT_SUCCESS;
}

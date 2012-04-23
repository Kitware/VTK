#include <vtkSmartPointer.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkBandedPolyDataContourFilter.h>

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

  int numberOfContours = atoi(argv[2]);

  vtkSmartPointer<vtkBandedPolyDataContourFilter> bandedContours =
    vtkSmartPointer<vtkBandedPolyDataContourFilter>::New();
  bandedContours->SetInputConnection(reader->GetOutputPort());
  bandedContours->SetScalarModeToValue();
  bandedContours->GenerateContourEdgesOn();
  bandedContours->GenerateValues(
    numberOfContours, scalarRange[0], scalarRange[1]);

  vtkSmartPointer<vtkLookupTable> lut =
    vtkSmartPointer<vtkLookupTable>::New();
  lut->SetNumberOfTableValues(numberOfContours + 1);
  lut->Build();

  vtkSmartPointer<vtkPolyDataMapper> contourMapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  contourMapper->SetInputConnection(bandedContours->GetOutputPort());
  contourMapper->SetScalarRange(scalarRange[0], scalarRange[1]);
  contourMapper->SetScalarModeToUseCellData();
  contourMapper->SetLookupTable(lut);

  vtkSmartPointer<vtkActor> contourActor =
    vtkSmartPointer<vtkActor>::New();
  contourActor->SetMapper(contourMapper);
  contourActor->GetProperty()->SetInterpolationToFlat();

  vtkSmartPointer<vtkPolyDataMapper> contourLineMapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();

  contourLineMapper->SetInputData(bandedContours->GetContourEdgesOutput());
  contourLineMapper->SetScalarRange(scalarRange[0], scalarRange[1]);
  contourLineMapper->ScalarVisibilityOff();

  vtkSmartPointer<vtkActor> contourLineActor =
    vtkSmartPointer<vtkActor>::New();
  contourLineActor->SetMapper(contourLineMapper);
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

#include "vtkNew.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"
#include "vtkXMLPolyDataWriter.h"

int TestXMLToString(int /*argc*/, char* /*argv*/[])
{
  vtkNew<vtkSphereSource> sphere;
  vtkNew<vtkXMLPolyDataWriter> writer;

  writer->WriteToOutputStringOn();

  writer->SetInputConnection(0, sphere->GetOutputPort(0));

  writer->Update();
  writer->Write();

  writer->GetOutputString();

  return 0;
}

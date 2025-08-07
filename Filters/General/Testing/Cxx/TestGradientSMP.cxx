#include "vtkGradientFilter.h"
#include "vtkLineSource.h"
#include "vtkLogger.h"
#include "vtkSMPTools.h"

int TestGradientSMP(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  // Gradient filter with several SMP backends used to generate (occasionally) a segfault.
  // This test is only here to ensure it doesn't crash anymore.
  vtkSMPTools::SetBackend("STDThread");
  vtkNew<vtkLineSource> lineSource;
  lineSource->Update();
  vtkNew<vtkGradientFilter> lineGradient;
  lineGradient->SetInputData(lineSource->GetOutput());
  lineGradient->SetInputArrayToProcess(
    "Texture Coordinates", vtkDataObject::FIELD_ASSOCIATION_POINTS);
  lineGradient->Update();

  const vtkIdType expectedNbPoints = lineSource->GetOutput()->GetNumberOfPoints();
  const vtkIdType actualNbPoints = lineGradient->GetOutput()->GetNumberOfPoints();
  if (expectedNbPoints != actualNbPoints)
  {
    vtkLog(ERROR,
      "Incorrect number of points: expected " << expectedNbPoints << " but got " << actualNbPoints);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

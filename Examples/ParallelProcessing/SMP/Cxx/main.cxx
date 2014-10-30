#include "vtkSMPContourFilterManyPieces.h"
#include "vtkSMPContourGridManyPieces.h"
#include "vtkRTAnalyticSource.h"
#include "vtkDataSetTriangleFilter.h"
#include "vtkNew.h"
#include "vtkUnstructuredGrid.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkSMPTools.h"

int main()
{
//  vtkSMPTools::Initialize(4);

  vtkNew<vtkRTAnalyticSource> source;
  source->SetWholeExtent(-50, 50, -50, 50, -50, 50);

  vtkNew<vtkDataSetTriangleFilter> tf;
  tf->SetInputConnection(source->GetOutputPort());

  tf->Update();

  vtkNew<vtkSMPContourFilterManyPieces> cf;
  cf->SetInputConnection(tf->GetOutputPort());
  cf->SetValue(0, 200);
  cf->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "RTData");
//  cf->SetUseScalarTree(1);

  cf->Update();

  return 0;
}
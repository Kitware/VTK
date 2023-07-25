// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellCenters.h"
#include "vtkCellData.h"
#include "vtkExtractCellsAlongPolyLine.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkImageData.h"
#include "vtkLogger.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyLineSource.h"
#include "vtkRTAnalyticSource.h"
#include "vtkStaticPointLocator.h"
#include "vtkThreshold.h"
#include "vtkUnstructuredGrid.h"

#include <numeric>

namespace
{
constexpr int DIM = 100;
constexpr vtkIdType NUMBER_OF_CELLS = DIM * DIM * DIM;
constexpr vtkIdType NUMBER_OF_LINES = 100;
constexpr vtkIdType NUMBER_OF_LINE_POINTS = NUMBER_OF_LINES + 1;
constexpr vtkIdType NUMBER_OF_OUTPUT_CELLS = 2 * DIM - 1;
constexpr vtkIdType NUMBER_OF_OUTPUT_POINTS = 8 * DIM;

//------------------------------------------------------------------------------
bool TestOutput(vtkImageData* image, vtkUnstructuredGrid* output)
{
  bool retVal = true;

  vtkIdType numberOfOutputPoints = output->GetNumberOfPoints();
  vtkIdType numberOfOutputCells = output->GetNumberOfCells();

  if (numberOfOutputCells != NUMBER_OF_OUTPUT_CELLS)
  {
    vtkLog(ERROR,
      "Wrong number of output cells: " << numberOfOutputCells << " instead of "
                                       << NUMBER_OF_OUTPUT_CELLS);
    retVal = false;
  }

  if (numberOfOutputPoints != NUMBER_OF_OUTPUT_POINTS)
  {
    vtkLog(ERROR,
      "Wrong number of output points: " << numberOfOutputPoints << " instead of "
                                        << NUMBER_OF_OUTPUT_POINTS);
    retVal = false;
  }

  vtkNew<vtkCellCenters> centers;
  centers->SetInputData(output);
  centers->Update();

  auto centersPS = vtkPointSet::SafeDownCast(centers->GetOutputDataObject(0));

  vtkNew<vtkCellCenters> imageCenters;
  imageCenters->SetInputData(image);
  imageCenters->Update();

  auto imageCentersPS = vtkPointSet::SafeDownCast(imageCenters->GetOutputDataObject(0));

  vtkNew<vtkStaticPointLocator> imageLocator;
  imageLocator->SetDataSet(imageCentersPS);

  vtkNew<vtkPoints> centerPointsRef;
  centerPointsRef->SetNumberOfPoints(NUMBER_OF_OUTPUT_CELLS);

  vtkNew<vtkStaticPointLocator> locator;
  locator->SetDataSet(centersPS);
  locator->BuildLocator();

  auto cellIds =
    vtkArrayDownCast<vtkIdTypeArray>(image->GetCellData()->GetAbstractArray("CellIds"));
  auto RTData = vtkArrayDownCast<vtkFloatArray>(image->GetPointData()->GetAbstractArray("RTData"));

  auto outputCellIds =
    vtkArrayDownCast<vtkIdTypeArray>(output->GetCellData()->GetAbstractArray("CellIds"));
  auto outputRTData =
    vtkArrayDownCast<vtkFloatArray>(output->GetPointData()->GetAbstractArray("RTData"));

  for (vtkIdType cellId = 0; cellId < NUMBER_OF_OUTPUT_CELLS; ++cellId)
  {
    double p[3];
    if (cellId < DIM)
    {
      p[0] = 0.5;
      p[1] = 0.5;
      p[2] = 0.5 + cellId;
    }
    else
    {
      p[0] = 0.5;
      p[1] = 1.5 + cellId - DIM;
      p[2] = static_cast<double>(DIM) - 0.5;
    }

    vtkNew<vtkIdList> results;
    locator->FindPointsWithinRadius(1e-6, p, results);

    if (results->GetNumberOfIds() != 1)
    {
      vtkLog(ERROR, "Output geometry is wrong.");
      retVal = false;
      break;
    }

    vtkIdType centerCellId = results->GetId(0);

    imageLocator->FindPointsWithinRadius(1e-6, p, results);
    vtkIdType imageCellId = results->GetId(0);

    if (cellIds->GetValue(imageCellId) != outputCellIds->GetValue(centerCellId))
    {
      vtkLog(ERROR, "Output cell data is wrong.");
      retVal = false;
      break;
    }

    vtkNew<vtkIdList> outputPointIds, imagePointIds;
    output->GetCellPoints(centerCellId, outputPointIds);
    image->GetCellPoints(imageCellId, imagePointIds);

    if (output->GetCellType(centerCellId) == VTK_HEXAHEDRON)
    {
      // Change point order: voxels and hexahedron don't have same connectivity.
      // swap 2nd and 3rd, 6th and 7th ids
      vtkIdType tmp = outputPointIds->GetId(2);
      outputPointIds->SetId(2, outputPointIds->GetId(3));
      outputPointIds->SetId(3, tmp);
      tmp = outputPointIds->GetId(6);
      outputPointIds->SetId(6, outputPointIds->GetId(7));
      outputPointIds->SetId(7, tmp);
    }

    for (vtkIdType id = 0; id < imagePointIds->GetNumberOfIds(); ++id)
    {
      vtkIdType imagePointId = imagePointIds->GetId(id);
      vtkIdType outputPointId = outputPointIds->GetId(id);

      if (RTData->GetValue(imagePointId) != outputRTData->GetValue(outputPointId))
      {
        vtkLog(ERROR, "Output point data is wrong.");
        retVal = false;
        break;
      }

      double imagePoint[3], outputPoint[3];
      output->GetPoint(outputPointId, outputPoint);
      image->GetPoint(imagePointId, imagePoint);

      if (outputPoint[0] != imagePoint[0] || outputPoint[1] != imagePoint[1] ||
        outputPoint[2] != imagePoint[2])
      {
        vtkLog(ERROR, "Output point positions are wrong.");
        retVal = false;
        break;
      }
    }
  }

  return retVal;
}
} // anonymous namespace

//------------------------------------------------------------------------------
int TestExtractCellsAlongPolyLine(int, char*[])
{
  int retVal = EXIT_SUCCESS;

  vtkNew<vtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(0, DIM, 0, DIM, 0, DIM);
  wavelet->Update();

  vtkNew<vtkImageData> image;
  image->ShallowCopy(wavelet->GetOutputDataObject(0));

  vtkNew<vtkIdTypeArray> cellIds;
  cellIds->SetNumberOfValues(NUMBER_OF_CELLS);
  cellIds->SetName("CellIds");

  std::iota(cellIds->GetPointer(0), cellIds->GetPointer(NUMBER_OF_CELLS - 1), 0);
  image->GetCellData()->AddArray(cellIds);

  vtkNew<vtkPolyLineSource> polyLine;
  polyLine->SetNumberOfPoints(NUMBER_OF_LINE_POINTS);
  vtkPoints* polyLinePoints = polyLine->GetPoints();

  double p[3];

  for (vtkIdType pointId = 0; pointId < NUMBER_OF_LINE_POINTS; ++pointId)
  {
    if (pointId < NUMBER_OF_LINE_POINTS / 2)
    {
      p[0] = 0.5;
      p[1] = 0.5;
      p[2] = 0.5 + ((static_cast<double>(pointId) * 2.0 / (NUMBER_OF_LINE_POINTS - 2))) * (DIM - 1);
    }
    else
    {
      p[0] = 0.5;
      p[1] = 0.5 +
        (static_cast<double>(pointId - NUMBER_OF_LINE_POINTS / 2.0) * 2.0 /
          (NUMBER_OF_LINE_POINTS - 2)) *
          (DIM - 1);
      p[2] = static_cast<double>(DIM) - 0.5;
    }
    polyLinePoints->SetPoint(pointId, p);
  }

  vtkNew<vtkExtractCellsAlongPolyLine> extractor;
  extractor->SetSourceConnection(polyLine->GetOutputPort());

  vtkLog(INFO, "Testing for vtkDataSet input... (General case)");

  extractor->SetInputData(image);
  extractor->Update();

  if (!TestOutput(image, extractor->GetOutput(0)))
  {
    retVal = EXIT_FAILURE;
  }

  vtkLog(INFO, "Testing for vtkUnstructuredGrid input...");

  vtkNew<vtkThreshold> threshold;
  threshold->SetInputData(image);

  extractor->SetInputConnection(threshold->GetOutputPort());
  extractor->Update();

  if (!TestOutput(image, extractor->GetOutput(0)))
  {
    retVal = EXIT_FAILURE;
  }

  return retVal;
}

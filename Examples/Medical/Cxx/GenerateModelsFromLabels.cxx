//
// GenerateModelsFromLabels
//   Usage: GenerateModelsFromLabels InputVolume Startlabel Endlabel
//          where
//          InputVolume is a meta file containing a 3 volume of
//            discrete labels.
//          StartLabel is the first label to be processed
//          EndLabel is the last label to be processed
//          NOTE: There can be gaps in the labeling. If a label does
//          not exist in the volume, it will be skipped.
//
//
#include <vtkMetaImageReader.h>
#include <vtkImageAccumulate.h>
#include <vtkDiscreteMarchingCubes.h>
#include <vtkWindowedSincPolyDataFilter.h>
#include <vtkMaskFields.h>
#include <vtkThreshold.h>
#include <vtkGeometryFilter.h>
#include <vtkXMLPolyDataWriter.h>
#include <vtkSmartPointer.h>

#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkUnstructuredGrid.h>
#include <sstream>

int main (int argc, char *argv[])
{
  if (argc < 4)
  {
    cout << "Usage: " << argv[0] << " InputVolume StartLabel EndLabel" << endl;
    return EXIT_FAILURE;
  }

  // Create all of the classes we will need
  vtkSmartPointer<vtkMetaImageReader> reader =
    vtkSmartPointer<vtkMetaImageReader>::New();
  vtkSmartPointer<vtkImageAccumulate> histogram =
    vtkSmartPointer<vtkImageAccumulate>::New();
  vtkSmartPointer<vtkDiscreteMarchingCubes> discreteCubes =
    vtkSmartPointer<vtkDiscreteMarchingCubes>::New();
  vtkSmartPointer<vtkWindowedSincPolyDataFilter> smoother =
    vtkSmartPointer<vtkWindowedSincPolyDataFilter>::New();
  vtkSmartPointer<vtkThreshold> selector =
    vtkSmartPointer<vtkThreshold>::New();
  vtkSmartPointer<vtkMaskFields> scalarsOff =
    vtkSmartPointer<vtkMaskFields>::New();
  vtkSmartPointer<vtkGeometryFilter> geometry =
    vtkSmartPointer<vtkGeometryFilter>::New();
  vtkSmartPointer<vtkXMLPolyDataWriter> writer =
    vtkSmartPointer<vtkXMLPolyDataWriter>::New();

  // Define all of the variables
  unsigned int startLabel = atoi(argv[2]);
  if (startLabel > VTK_SHORT_MAX)
  {
    std::cout << "ERROR: startLabel is larger than " << VTK_SHORT_MAX << std::endl;
    return EXIT_FAILURE;
  }
  unsigned int endLabel = atoi(argv[3]);
  if (endLabel > VTK_SHORT_MAX)
  {
    std::cout << "ERROR: endLabel is larger than " << VTK_SHORT_MAX << std::endl;
    return EXIT_FAILURE;
  }
  std::string filePrefix = "Label";
  unsigned int smoothingIterations = 15;
  double passBand = 0.001;
  double featureAngle = 120.0;

  // Generate models from labels
  // 1) Read the meta file
  // 2) Generate a histogram of the labels
  // 3) Generate models from the labeled volume
  // 4) Smooth the models
  // 5) Output each model into a separate file

  reader->SetFileName(argv[1]);

  histogram->SetInputConnection(reader->GetOutputPort());
  histogram->SetComponentExtent(0, endLabel, 0, 0, 0, 0);
  histogram->SetComponentOrigin(0, 0, 0);
  histogram->SetComponentSpacing(1, 1, 1);
  histogram->Update();

  discreteCubes->SetInputConnection(reader->GetOutputPort());
  discreteCubes->GenerateValues(
    endLabel - startLabel + 1, startLabel, endLabel);

  smoother->SetInputConnection(discreteCubes->GetOutputPort());
  smoother->SetNumberOfIterations(smoothingIterations);
  smoother->BoundarySmoothingOff();
  smoother->FeatureEdgeSmoothingOff();
  smoother->SetFeatureAngle(featureAngle);
  smoother->SetPassBand(passBand);
  smoother->NonManifoldSmoothingOn();
  smoother->NormalizeCoordinatesOn();
  smoother->Update();

  selector->SetInputConnection(smoother->GetOutputPort());
  selector->SetInputArrayToProcess(0, 0, 0,
                                   vtkDataObject::FIELD_ASSOCIATION_CELLS,
                                   vtkDataSetAttributes::SCALARS);

  // Strip the scalars from the output
  scalarsOff->SetInputConnection(selector->GetOutputPort());
  scalarsOff->CopyAttributeOff(vtkMaskFields::POINT_DATA,
                               vtkDataSetAttributes::SCALARS);
  scalarsOff->CopyAttributeOff(vtkMaskFields::CELL_DATA,
                               vtkDataSetAttributes::SCALARS);

  geometry->SetInputConnection(scalarsOff->GetOutputPort());

  writer->SetInputConnection(geometry->GetOutputPort());

  for (unsigned int i = startLabel; i <= endLabel; i++)
  {
    // see if the label exists, if not skip it
    double frequency =
      histogram->GetOutput()->GetPointData()->GetScalars()->GetTuple1(i);
    if (frequency == 0.0)
    {
      continue;
    }

    // select the cells for a given label
    selector->ThresholdBetween(i, i);

    // output the polydata
    std::stringstream ss;
    ss << filePrefix << i << ".vtp";
    cout << argv[0] << " writing " << ss.str() << endl;

    writer->SetFileName(ss.str().c_str());
    writer->Write();

  }
  return EXIT_SUCCESS;
}

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
#include <vtksys/ios/sstream>
 
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
  unsigned int endLabel = atoi(argv[3]);
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
 
  histogram->SetInput(reader->GetOutput());
  histogram->SetComponentExtent(0, endLabel, 0, 0, 0, 0);
  histogram->SetComponentOrigin(0, 0, 0);
  histogram->SetComponentSpacing(1, 1, 1);
  histogram->Update();
 
  discreteCubes->SetInput(reader->GetOutput());
  discreteCubes->GenerateValues(
    endLabel - startLabel + 1, startLabel, endLabel);
 
  smoother->SetInput(discreteCubes->GetOutput());
  smoother->SetNumberOfIterations(smoothingIterations);
  smoother->BoundarySmoothingOff();
  smoother->FeatureEdgeSmoothingOff();
  smoother->SetFeatureAngle(featureAngle);
  smoother->SetPassBand(passBand);
  smoother->NonManifoldSmoothingOn();
  smoother->NormalizeCoordinatesOn();
  smoother->Update();
 
  selector->SetInput(smoother->GetOutput());
  selector->SetInputArrayToProcess(0, 0, 0,
                                   vtkDataObject::FIELD_ASSOCIATION_CELLS,
                                   vtkDataSetAttributes::SCALARS);
 
  // Strip the scalars from the output
  scalarsOff->SetInput(selector->GetOutput());
  scalarsOff->CopyAttributeOff(vtkMaskFields::POINT_DATA,
                               vtkDataSetAttributes::SCALARS);
  scalarsOff->CopyAttributeOff(vtkMaskFields::CELL_DATA,
                               vtkDataSetAttributes::SCALARS);
 
  geometry->SetInput(scalarsOff->GetOutput());
 
  writer->SetInput(geometry->GetOutput());
 
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
    vtksys_stl::stringstream ss;
    ss << filePrefix << i << ".vtp";
    cout << argv[0] << " writing " << ss.str() << endl;
 
    writer->SetFileName(ss.str().c_str());
    writer->Write();
 
    }
  return EXIT_SUCCESS;
}

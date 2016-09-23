/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGDALVectorReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGDALVectorReader.h"

// VTK includes
#include <vtkActor.h>
#include <vtkCellData.h>
#include <vtkCompositePolyDataMapper.h>
#include <vtkDataSetAttributes.h>
#include <vtkDoubleArray.h>
#include <vtkLookupTable.h>
#include <vtkMapper.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkNew.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSmartPointer.h>
#include <vtkTestUtilities.h>

// C++ includes
#include <sstream>

// Main program
int TestGDALVectorReader(int argc, char* argv[])
{
  const char* vectorFileName = vtkTestUtilities::ExpandDataFileName(argc, argv,
                                 "Data/GIS/countries.shp");

  // Create reader to read shape file.
  vtkNew<vtkGDALVectorReader> reader;
  reader->SetFileName(vectorFileName);
  reader->AddFeatureIdsOn();
  delete [] vectorFileName;

  // Test layer information helpers
  reader->UpdateInformation();
  int nl = reader->GetNumberOfLayers();
  for (int i = 0; i < nl; ++i)
  {
    reader->SetActiveLayer(i);
    cout
      << "Layer " << i
      << " Type " << reader->GetActiveLayerType()
      << " FeatureCount " << reader->GetActiveLayerFeatureCount()
      << "\n";
  }
  reader->SetActiveLayer(0); // Read only layer 0, which is the only layer.
  reader->Update();

  // We need a renderer
  vtkNew<vtkRenderer> renderer;

  // Get the data
  vtkSmartPointer<vtkMultiBlockDataSet> mbds = reader->GetOutput();

  // Verify that feature IDs exist as a scalar (assuming first block exists)
  if (mbds && mbds->GetNumberOfBlocks() > 0)
  {
    vtkPolyData* pd = vtkPolyData::SafeDownCast(mbds->GetBlock(0));
    vtkCellData* cd = pd ? pd->GetCellData() : NULL;
    if (cd)
    {
      if (!cd->GetPedigreeIds())
      {
        cerr << "Unable to find pedigree IDs even though AddFeatureIds was ON\n";
        return 1;
      }
    }
  }

  // Create scene
  vtkNew<vtkActor> actor;
  vtkNew<vtkCompositePolyDataMapper> mapper;

  // Create an interesting lookup table
  vtkNew<vtkLookupTable> lut;
  lut->SetTableRange(1.0, 8.0);
  lut->SetValueRange(0.6, 0.9);
  lut->SetHueRange(0.0, 0.8);
  lut->SetSaturationRange(0.0, 0.7);
  lut->SetNumberOfColors(8);
  lut->Build();

  mapper->SetInputDataObject(mbds);
  mapper->SelectColorArray("mapcolor8");
  mapper->SetScalarModeToUseCellFieldData();
  mapper->SetScalarVisibility(1);
  mapper->UseLookupTableScalarRangeOn();
  mapper->SetLookupTable(lut.GetPointer());
  mapper->SetColorModeToMapScalars();
  actor->SetMapper(mapper.GetPointer());
  actor->GetProperty()->SetLineWidth(1.4f);
  renderer->AddActor(actor.GetPointer());

  // Create a render window, and an interactor
  vtkNew<vtkRenderWindow> renderWindow;
  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindow->AddRenderer(renderer.GetPointer());
  renderWindowInteractor->SetRenderWindow(renderWindow.GetPointer());

  //Add the actor to the scene
  renderer->SetBackground(1.0, 1.0, 1.0);
  renderWindow->SetSize(400, 400);
  renderWindow->Render();
  renderer->ResetCamera();
  renderWindow->Render();

  int retVal = vtkRegressionTestImage(renderWindow.GetPointer());

  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
  }

  return !retVal;
}

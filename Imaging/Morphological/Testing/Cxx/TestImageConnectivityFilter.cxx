/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ImageConnectivityFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Test the ImageConnectivityFilter class
//
// The command line arguments are:
// -I        => run in interactive mode

#include "vtkSmartPointer.h"
#include "vtkCamera.h"
#include "vtkImageConnectivityFilter.h"
#include "vtkImageData.h"
#include "vtkImageProperty.h"
#include "vtkImageReader2.h"
#include "vtkImageSlice.h"
#include "vtkImageSliceMapper.h"
#include "vtkInteractorStyleImage.h"
#include "vtkIdTypeArray.h"
#include "vtkIntArray.h"
#include "vtkPoints.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkVersion.h"

#include "vtkTestUtilities.h"

#include <string>

int TestImageConnectivityFilter(int argc, char *argv[])
{
  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  vtkSmartPointer<vtkInteractorStyleImage> style =
    vtkSmartPointer<vtkInteractorStyleImage>::New();
  style->SetInteractionModeToImageSlicing();
  vtkSmartPointer<vtkRenderWindow> renWin =
    vtkSmartPointer<vtkRenderWindow>::New();
  iren->SetRenderWindow(renWin);
  iren->SetInteractorStyle(style);

  // Use a 3D image for the test
  char *temp =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/headsq/quarter");
  std::string fname = temp;
  delete [] temp;

  vtkSmartPointer<vtkImageReader2> reader =
    vtkSmartPointer<vtkImageReader2>::New();
  reader->SetDataByteOrderToLittleEndian();
  reader->SetDataExtent(0, 63, 0, 63, 2, 4);
  reader->SetDataSpacing(3.2, 3.2, 1.5);
  reader->SetFilePrefix(fname.c_str());

  // Create two seed points
  vtkSmartPointer<vtkPoints> seedPoints =
    vtkSmartPointer<vtkPoints>::New();
  seedPoints->InsertNextPoint(25.6, 100.8, 2.25);
  seedPoints->InsertNextPoint(100.8, 100.8, 2.25);
  vtkSmartPointer<vtkUnsignedCharArray> seedScalars =
    vtkSmartPointer<vtkUnsignedCharArray>::New();
  seedScalars->InsertNextValue(2);
  seedScalars->InsertNextValue(5);
  vtkSmartPointer<vtkPolyData> seedData =
    vtkSmartPointer<vtkPolyData>::New();
  seedData->SetPoints(seedPoints);
  seedData->GetPointData()->SetScalars(seedScalars);

  // Generate a grid of renderers for the various tests
  for (int i = 0; i < 9; i++)
  {
    int j = 2 - i / 3;
    int k = i % 3;
    vtkSmartPointer<vtkRenderer> renderer =
      vtkSmartPointer<vtkRenderer>::New();
    vtkCamera *camera = renderer->GetActiveCamera();
    renderer->SetBackground(0.0, 0.0, 0.0);
    renderer->SetViewport(k/3.0, j/3.0, (k + 1)/3.0, (j + 1)/3.0);
    renWin->AddRenderer(renderer);

    vtkSmartPointer<vtkImageConnectivityFilter> connectivity =
      vtkSmartPointer<vtkImageConnectivityFilter>::New();
    connectivity->SetInputConnection(reader->GetOutputPort());

    if (i == 0)
    {
      connectivity->GenerateRegionExtentsOn();
      connectivity->SetScalarRange(800, 1200);
      // No seeds
      // Default extraction mode
      // Default label mode
    }
    else if (i == 1)
    {
      connectivity->SetScalarRange(800, 1200);
      // No seeds
      connectivity->SetExtractionModeToLargestRegion();
      // Default label mode
    }
    else if (i == 2)
    {
      connectivity->SetScalarRange(800, 1200);
      // No seeds
      connectivity->SetSizeRange(10, 99);
      // Default label mode
    }
    else if (i == 3)
    {
      connectivity->SetScalarRange(800, 1200);
      connectivity->SetSeedData(seedData);
      // Default extraction mode
      // Default label mode (use seed scalars)
    }
    else if (i == 4)
    {
      connectivity->SetScalarRange(800, 1200);
      connectivity->SetSeedData(seedData);
      connectivity->SetExtractionModeToAllRegions();
      connectivity->SetLabelModeToSizeRank();
    }
    else if (i == 5)
    {
      // Seeds with no scalars
      connectivity->SetScalarRange(800, 1200);
      seedData->GetPointData()->SetScalars(NULL);
      connectivity->SetSeedData(seedData);
    }
    else if (i == 6)
    {
      connectivity->SetScalarRange(1200, 4095);
    }
    else if (i == 7)
    {
      connectivity->SetScalarRange(0, 800);
    }
    else if (i == 8)
    {
      // use default scalar range
    }

    if (i == 0)
    {
      // Test OutputExtent != InputExtent
      int extent[6] = { 0, 63, 0, 63, 3, 3 };
      connectivity->UpdateExtent(extent);
    }
    else
    {
      // Test updating whole extent
      connectivity->Update();
    }

    // Test getting info about the output regions
    vtkIdTypeArray *sizeArray = connectivity->GetExtractedRegionSizes();
    vtkIdTypeArray *idArray = connectivity->GetExtractedRegionSeedIds();
    vtkIdTypeArray *labelArray = connectivity->GetExtractedRegionLabels();
    vtkIntArray *extentArray = connectivity->GetExtractedRegionExtents();
    vtkIdType rn = connectivity->GetNumberOfExtractedRegions();
    std::cout << "\nTest Case: " << i << std::endl;
    std::cout << "number of regions: " << rn << std::endl;
    for (vtkIdType r = 0; r < rn; r++)
    {
      std::cout << "region: " << r << ","
                << " seed: " << idArray->GetValue(r) << ","
                << " label: " << labelArray->GetValue(r) << ","
                << " size: " << sizeArray->GetValue(r) << ","
                << " extent: [";
      if (connectivity->GetGenerateRegionExtents())
      {
         std::cout << extentArray->GetValue(6*r) << ","
                   << extentArray->GetValue(6*r+1) << ","
                   << extentArray->GetValue(6*r+2) << ","
                   << extentArray->GetValue(6*r+3) << ","
                   << extentArray->GetValue(6*r+4) << ","
                   << extentArray->GetValue(6*r+5);
      }
      std::cout << "]" << std::endl;
    }

    vtkSmartPointer<vtkImageSliceMapper> imageMapper =
      vtkSmartPointer<vtkImageSliceMapper>::New();
    imageMapper->SetInputConnection(connectivity->GetOutputPort());
    imageMapper->BorderOn();
    imageMapper->SliceFacesCameraOn();
    imageMapper->SliceAtFocalPointOn();

    double point[3] = { 100.8, 100.8, 5.25 };
    camera->SetFocalPoint(point);
    point[2] += 500.0;
    camera->SetPosition(point);
    camera->SetViewUp(0.0, 1.0, 0.0);
    camera->ParallelProjectionOn();
    camera->SetParallelScale(3.2*32);

    vtkSmartPointer<vtkImageSlice> image =
      vtkSmartPointer<vtkImageSlice>::New();
    image->SetMapper(imageMapper);
    image->GetProperty()->SetColorWindow(6);
    image->GetProperty()->SetColorLevel(3);
    renderer->AddViewProp(image);
  }

  renWin->SetSize(192, 256);

  iren->Initialize();
  renWin->Render();
  iren->Start();

  return EXIT_SUCCESS;
}

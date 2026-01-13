// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// .NAME Test of vtkDICOMImageReader
// .SECTION Description
//

#include "vtkSmartPointer.h"

#include "vtkDICOMImageReader.h"

#include "vtkImageData.h"
#include "vtkImageViewer2.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

#include <iostream>

int TestDICOMImageReader(int argc, char* argv[])
{

  if (argc <= 1)
  {
    std::cout << "Usage: " << argv[0] << " <dicom file>" << std::endl;
    return 1;
  }

  std::string filename = argv[1];

  vtkSmartPointer<vtkDICOMImageReader> DICOMReader = vtkSmartPointer<vtkDICOMImageReader>::New();

  // Check the image can be read
  if (!DICOMReader->CanReadFile(filename.c_str()))
  {
    std::cerr << "CanReadFile failed for " << filename << "\n";
    return EXIT_FAILURE;
  }

  // Read the input image
  DICOMReader->SetFileName(filename.c_str());
  DICOMReader->Update();

  // Read and display the image properties
  const char* fileExtensions = DICOMReader->GetFileExtensions();
  std::cout << "fileExtensions: " << fileExtensions << std::endl;

  const char* descriptiveName = DICOMReader->GetDescriptiveName();
  std::cout << "descriptiveName: " << descriptiveName << std::endl;

  double* pixelSpacing = DICOMReader->GetPixelSpacing();
  std::cout << "pixelSpacing: " << *pixelSpacing << std::endl;

  int width = DICOMReader->GetWidth();
  std::cout << "width: " << width << std::endl;

  int height = DICOMReader->GetHeight();
  std::cout << "height: " << height << std::endl;

  float* imagePositionPatient = DICOMReader->GetImagePositionPatient();
  std::cout << "imagePositionPatient: " << *imagePositionPatient << std::endl;

  float* imageOrientationPatient = DICOMReader->GetImageOrientationPatient();
  std::cout << "imageOrientationPatient: " << *imageOrientationPatient << std::endl;

  int bitsAllocated = DICOMReader->GetBitsAllocated();
  std::cout << "bitsAllocated: " << bitsAllocated << std::endl;

  int pixelRepresentation = DICOMReader->GetPixelRepresentation();
  std::cout << "pixelRepresentation: " << pixelRepresentation << std::endl;

  int numberOfComponents = DICOMReader->GetNumberOfComponents();
  std::cout << "numberOfComponents: " << numberOfComponents << std::endl;

  const char* transferSyntaxUID = DICOMReader->GetTransferSyntaxUID();
  std::cout << "transferSyntaxUID: " << transferSyntaxUID << std::endl;

  float rescaleSlope = DICOMReader->GetRescaleSlope();
  std::cout << "rescaleSlope: " << rescaleSlope << std::endl;

  float rescaleOffset = DICOMReader->GetRescaleOffset();
  std::cout << "rescaleOffset: " << rescaleOffset << std::endl;

  const char* patientName = DICOMReader->GetPatientName();
  std::cout << "patientName: " << patientName << std::endl;

  const char* studyUID = DICOMReader->GetStudyUID();
  std::cout << "studyUID: " << studyUID << std::endl;

  const char* studyID = DICOMReader->GetStudyID();
  std::cout << "studyID: " << studyID << std::endl;

  float gantryAngle = DICOMReader->GetGantryAngle();
  std::cout << "gantryAngle: " << gantryAngle << std::endl;

  // Display the center slice
  int sliceNumber =
    (DICOMReader->GetOutput()->GetExtent()[5] + DICOMReader->GetOutput()->GetExtent()[4]) / 2;

  // Visualize
  vtkSmartPointer<vtkImageViewer2> imageViewer = vtkSmartPointer<vtkImageViewer2>::New();
  imageViewer->SetInputConnection(DICOMReader->GetOutputPort());
  vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  imageViewer->SetupInteractor(renderWindowInteractor);
  imageViewer->SetSlice(sliceNumber);
  imageViewer->Render();
  imageViewer->GetRenderer()->ResetCamera();
  renderWindowInteractor->Initialize();
  imageViewer->Render();

  renderWindowInteractor->Start();

  return 0;
}

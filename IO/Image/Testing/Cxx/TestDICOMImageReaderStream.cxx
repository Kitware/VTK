// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// .NAME Test of vtkDICOMImageReader
// .SECTION Description
//

#include "vtkSmartPointer.h"

#include "vtkDICOMImageReader.h"

#include "vtkFileResourceStream.h"
#include "vtkImageData.h"
#include "vtkImageViewer2.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

#include <iostream>

int TestDICOMImageReaderStream(int argc, char* argv[])
{

  if (argc <= 1)
  {
    std::cout << "Usage: " << argv[0] << " <dicom file>" << endl;
    return 1;
  }

  std::string filename = argv[1];

  vtkSmartPointer<vtkDICOMImageReader> DICOMReader = vtkSmartPointer<vtkDICOMImageReader>::New();

  vtkNew<vtkFileResourceStream> fileStream;
  fileStream->Open(filename.c_str());

  // Check the image can be read
  if (!DICOMReader->CanReadFile(fileStream))
  {
    std::cerr << "CanReadFile failed for stream\n";
    return EXIT_FAILURE;
  }

  // Read the input image
  DICOMReader->SetStream(fileStream);
  DICOMReader->Update();

  // Read and display the image properties
  const char* fileExtensions = DICOMReader->GetFileExtensions();
  std::cout << "fileExtensions: " << fileExtensions << endl;

  const char* descriptiveName = DICOMReader->GetDescriptiveName();
  std::cout << "descriptiveName: " << descriptiveName << endl;

  double* pixelSpacing = DICOMReader->GetPixelSpacing();
  std::cout << "pixelSpacing: " << *pixelSpacing << endl;

  int width = DICOMReader->GetWidth();
  std::cout << "width: " << width << endl;

  int height = DICOMReader->GetHeight();
  std::cout << "height: " << height << endl;

  float* imagePositionPatient = DICOMReader->GetImagePositionPatient();
  std::cout << "imagePositionPatient: " << *imagePositionPatient << endl;

  float* imageOrientationPatient = DICOMReader->GetImageOrientationPatient();
  std::cout << "imageOrientationPatient: " << *imageOrientationPatient << endl;

  int bitsAllocated = DICOMReader->GetBitsAllocated();
  std::cout << "bitsAllocated: " << bitsAllocated << endl;

  int pixelRepresentation = DICOMReader->GetPixelRepresentation();
  std::cout << "pixelRepresentation: " << pixelRepresentation << endl;

  int numberOfComponents = DICOMReader->GetNumberOfComponents();
  std::cout << "numberOfComponents: " << numberOfComponents << endl;

  const char* transferSyntaxUID = DICOMReader->GetTransferSyntaxUID();
  std::cout << "transferSyntaxUID: " << transferSyntaxUID << endl;

  float rescaleSlope = DICOMReader->GetRescaleSlope();
  std::cout << "rescaleSlope: " << rescaleSlope << endl;

  float rescaleOffset = DICOMReader->GetRescaleOffset();
  std::cout << "rescaleOffset: " << rescaleOffset << endl;

  const char* patientName = DICOMReader->GetPatientName();
  std::cout << "patientName: " << patientName << endl;

  const char* studyUID = DICOMReader->GetStudyUID();
  std::cout << "studyUID: " << studyUID << endl;

  const char* studyID = DICOMReader->GetStudyID();
  std::cout << "studyID: " << studyID << endl;

  float gantryAngle = DICOMReader->GetGantryAngle();
  std::cout << "gantryAngle: " << gantryAngle << endl;

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

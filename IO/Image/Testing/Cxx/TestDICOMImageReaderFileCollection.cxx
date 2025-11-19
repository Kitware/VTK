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
#include "vtkTestUtilities.h"

#include <iostream>

int TestDICOMImageReaderFileCollection(int argc, char* argv[])
{

  char* dirName = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dicom/collection");
  std::string directoryName = dirName;
  delete[] dirName;

  vtkSmartPointer<vtkDICOMImageReader> DICOMReader = vtkSmartPointer<vtkDICOMImageReader>::New();

  // Read the input files
  DICOMReader->SetDirectoryName(directoryName.c_str());
  std::cout << "Directory name: " << DICOMReader->GetDirectoryName() << std::endl;

  DICOMReader->Update();

  // Read and display the image properties
  const char* fileExtensions = DICOMReader->GetFileExtensions();
  std::cout << "File extensions: " << fileExtensions << std::endl;

  const char* descriptiveName = DICOMReader->GetDescriptiveName();
  std::cout << "Descriptive name: " << descriptiveName << std::endl;

  double* pixelSpacing = DICOMReader->GetPixelSpacing();
  std::cout << "Pixel spacing: " << *pixelSpacing << std::endl;

  int width = DICOMReader->GetWidth();
  std::cout << "Image width: " << width << std::endl;

  int height = DICOMReader->GetHeight();
  std::cout << "Image height: " << height << std::endl;

  float* imagePositionPatient = DICOMReader->GetImagePositionPatient();
  std::cout << "Image position patient: " << *imagePositionPatient << std::endl;

  float* imageOrientationPatient = DICOMReader->GetImageOrientationPatient();
  std::cout << "Image orientation patient: " << *imageOrientationPatient << std::endl;

  int bitsAllocated = DICOMReader->GetBitsAllocated();
  std::cout << "Bits allocated: " << bitsAllocated << std::endl;

  int pixelRepresentation = DICOMReader->GetPixelRepresentation();
  std::cout << "Pixel representation: " << pixelRepresentation << std::endl;

  int numberOfComponents = DICOMReader->GetNumberOfComponents();
  std::cout << "Number of components: " << numberOfComponents << std::endl;

  const char* transferSyntaxUID = DICOMReader->GetTransferSyntaxUID();
  std::cout << "Transfer syntax UID: " << transferSyntaxUID << std::endl;

  float rescaleSlope = DICOMReader->GetRescaleSlope();
  std::cout << "Rescale slope: " << rescaleSlope << std::endl;

  float rescaleOffset = DICOMReader->GetRescaleOffset();
  std::cout << "Rescale offset: " << rescaleOffset << std::endl;

  const char* patientName = DICOMReader->GetPatientName();
  std::cout << "Patient name: " << patientName << std::endl;

  const char* studyUID = DICOMReader->GetStudyUID();
  std::cout << "Study UID: " << studyUID << std::endl;

  const char* studyID = DICOMReader->GetStudyID();
  std::cout << "Study ID: " << studyID << std::endl;

  float gantryAngle = DICOMReader->GetGantryAngle();
  std::cout << "Gantry angle: " << gantryAngle << std::endl;

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

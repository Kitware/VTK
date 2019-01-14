/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDICOMImageReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of vtkDICOMImageReader
// .SECTION Description
//


#include "vtkSmartPointer.h"

#include "vtkDICOMImageReader.h"

#include "vtkImageData.h"
#include "vtkImageViewer2.h"
#include "vtkRenderer.h"
#include "vtkRenderWindowInteractor.h"


int TestDICOMImageReader(int argc, char *argv[])
{

  if ( argc <= 1 )
  {
    cout << "Usage: " << argv[0] << " <dicom file>" << endl;
    return 1;
  }

  std::string filename = argv[1];

  vtkSmartPointer<vtkDICOMImageReader> DICOMReader =
    vtkSmartPointer<vtkDICOMImageReader>::New();

  // Check the image can be read
  if (!DICOMReader->CanReadFile(filename.c_str()))
  {
    cerr << "CanReadFile failed for " << filename.c_str() << "\n";
    exit(1);
  }

  // Read the input image
  DICOMReader->SetFileName(filename.c_str());
  DICOMReader->Update();

  // Read and display the image properties
  const char* fileExtensions = DICOMReader-> GetFileExtensions();
  cout << "fileExtensions: " << fileExtensions << endl;

  const char* descriptiveName = DICOMReader->GetDescriptiveName();
  cout << "descriptiveName: " << descriptiveName << endl;

  double* pixelSpacing = DICOMReader->GetPixelSpacing();
  cout << "pixelSpacing: " << *pixelSpacing << endl;

  int width = DICOMReader->GetWidth();
  cout << "width: " << width << endl;

  int height = DICOMReader->GetHeight();
  cout << "height: " << height << endl;

  float* imagePositionPatient = DICOMReader->GetImagePositionPatient();
  cout << "imagePositionPatient: " << *imagePositionPatient << endl;

  float* imageOrientationPatient = DICOMReader->GetImageOrientationPatient();
  cout << "imageOrientationPatient: " << *imageOrientationPatient << endl;

  int bitsAllocated = DICOMReader->GetBitsAllocated();
  cout << "bitsAllocated: " << bitsAllocated << endl;

  int pixelRepresentation = DICOMReader->GetPixelRepresentation();
  cout << "pixelRepresentation: " << pixelRepresentation << endl;

  int numberOfComponents = DICOMReader->GetNumberOfComponents();
  cout << "numberOfComponents: " << numberOfComponents << endl;

  const char* transferSyntaxUID = DICOMReader->GetTransferSyntaxUID();
  cout << "transferSyntaxUID: " << transferSyntaxUID << endl;

  float rescaleSlope = DICOMReader->GetRescaleSlope();
  cout << "rescaleSlope: " << rescaleSlope << endl;

  float rescaleOffset = DICOMReader->GetRescaleOffset();
  cout << "rescaleOffset: " << rescaleOffset << endl;

  const char* patientName = DICOMReader->GetPatientName();
  cout << "patientName: " << patientName << endl;

  const char* studyUID = DICOMReader->GetStudyUID();
  cout << "studyUID: " << studyUID << endl;

  const char* studyID = DICOMReader->GetStudyID();
  cout << "studyID: " << studyID << endl;

  float gantryAngle = DICOMReader->GetGantryAngle();
  cout << "gantryAngle: " << gantryAngle << endl;


  // Display the center slice
  int sliceNumber =
    (DICOMReader->GetOutput()->GetExtent()[5] +
     DICOMReader->GetOutput()->GetExtent()[4]) / 2;

  // Visualize
  vtkSmartPointer<vtkImageViewer2> imageViewer =
    vtkSmartPointer<vtkImageViewer2>::New();
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

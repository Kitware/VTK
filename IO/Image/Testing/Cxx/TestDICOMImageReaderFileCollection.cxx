/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDICOMImageReaderFileCollection.cxx

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
#include "vtkTestUtilities.h"


int TestDICOMImageReaderFileCollection(int argc, char *argv[])
{

  if ( argc <= 1 )
  {
    cout << "Usage: " << argv[0] << " <dicom folder>" << endl;
    return 1;
  }

  char* dirName = vtkTestUtilities::ExpandDataFileName( argc, argv, "Data/dicom/collection" );
  std::string directoryName = dirName;
  delete [] dirName;

  vtkSmartPointer<vtkDICOMImageReader> DICOMReader =
    vtkSmartPointer<vtkDICOMImageReader>::New();

  // Read the input files
  DICOMReader->SetDirectoryName(directoryName.c_str());
  cout << "Directory name: " << DICOMReader->GetDirectoryName() << endl;

  DICOMReader->Update();

  // Read and display the image properties
  const char* fileExtensions = DICOMReader->GetFileExtensions();
  cout << "File extensions: " << fileExtensions << endl;

  const char* descriptiveName = DICOMReader->GetDescriptiveName();
  cout << "Descriptive name: " << descriptiveName << endl;

  double* pixelSpacing = DICOMReader->GetPixelSpacing();
  cout << "Pixel spacing: " << *pixelSpacing << endl;

  int width = DICOMReader->GetWidth();
  cout << "Image width: " << width << endl;

  int height = DICOMReader->GetHeight();
  cout << "Image height: " << height << endl;

  float* imagePositionPatient = DICOMReader->GetImagePositionPatient();
  cout << "Image position patient: " << *imagePositionPatient << endl;

  float* imageOrientationPatient = DICOMReader->GetImageOrientationPatient();
  cout << "Image orientation patient: " << *imageOrientationPatient << endl;

  int bitsAllocated = DICOMReader->GetBitsAllocated();
  cout << "Bits allocated: " << bitsAllocated << endl;

  int pixelRepresentation = DICOMReader->GetPixelRepresentation();
  cout << "Pixel representation: " << pixelRepresentation << endl;

  int numberOfComponents = DICOMReader->GetNumberOfComponents();
  cout << "Number of components: " << numberOfComponents << endl;

  const char* transferSyntaxUID = DICOMReader->GetTransferSyntaxUID();
  cout << "Transfer syntax UID: " << transferSyntaxUID << endl;

  float rescaleSlope = DICOMReader->GetRescaleSlope();
  cout << "Rescale slope: " << rescaleSlope << endl;

  float rescaleOffset = DICOMReader->GetRescaleOffset();
  cout << "Rescale offset: " << rescaleOffset << endl;

  const char* patientName = DICOMReader->GetPatientName();
  cout << "Patient name: " << patientName << endl;

  const char* studyUID = DICOMReader->GetStudyUID();
  cout << "Study UID: " << studyUID << endl;

  const char* studyID = DICOMReader->GetStudyID();
  cout << "Study ID: " << studyID << endl;

  float gantryAngle = DICOMReader->GetGantryAngle();
  cout << "Gantry angle: " << gantryAngle << endl;


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

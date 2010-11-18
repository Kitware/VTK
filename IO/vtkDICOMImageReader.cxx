/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDICOMImageReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDICOMImageReader.h"

#include "vtkDirectory.h"
#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkErrorCode.h"

#include <vtkstd/vector>
#include <vtkstd/string>

#include <sys/stat.h>

#include "DICOMAppHelper.h"
#include "DICOMParser.h"

vtkStandardNewMacro(vtkDICOMImageReader);

class vtkDICOMImageReaderVector : public vtkstd::vector<vtkstd::string>
{

};

//----------------------------------------------------------------------------
vtkDICOMImageReader::vtkDICOMImageReader()
{
  this->Parser = new DICOMParser();
  this->AppHelper = new DICOMAppHelper();
  this->DirectoryName = NULL;
  this->PatientName = NULL;
  this->StudyUID = NULL;
  this->StudyID = NULL;
  this->TransferSyntaxUID = NULL;
  this->DICOMFileNames = new vtkDICOMImageReaderVector();
}

//----------------------------------------------------------------------------
vtkDICOMImageReader::~vtkDICOMImageReader()
{
  delete this->Parser;
  delete this->AppHelper;
  delete this->DICOMFileNames;

  if (this->DirectoryName)
    {
    delete [] this->DirectoryName;
    }
  if (this->PatientName)
    {
    delete [] this->PatientName;
    }
  if (this->StudyUID)
    {
    delete [] this->StudyUID;
    }
  if (this->StudyID)
    {
    delete [] this->StudyID;
    }
  if (this->TransferSyntaxUID)
    {
    delete [] this->TransferSyntaxUID;
    }
}

//----------------------------------------------------------------------------
void vtkDICOMImageReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (this->DirectoryName)
    {
    os << "DirectoryName : " << this->DirectoryName << "\n";
    }
  else
    {
    os << "DirectoryName : (NULL)" << "\n";
    }
  if (this->FileName)
    {
    os << "FileName : " << this->FileName << "\n";
    }
  else
    {
    os << "FileName : (NULL)" << "\n";
    }

}

//----------------------------------------------------------------------------
int vtkDICOMImageReader::CanReadFile(const char* fname)
{
  bool canOpen = this->Parser->OpenFile((const char*) fname);
  if (!canOpen)
    {
    vtkErrorMacro("DICOMParser couldn't open : " << fname);
    return 0;
    }
  bool canRead = this->Parser->IsDICOMFile();
  if (canRead)
    {
    return 1;
    }
  else
    {
    vtkErrorMacro("DICOMParser couldn't parse : " << fname);
    return 0;
    }
}

//----------------------------------------------------------------------------
void vtkDICOMImageReader::ExecuteInformation()
{
  if (this->FileName == NULL && this->DirectoryName == NULL)
    {
    return;
    }

  if (this->FileName)
    {
    struct stat fs;
    if ( stat(this->FileName, &fs) )
      {
      vtkErrorMacro("Unable to open file " << this->FileName );
      return;
      }

    this->DICOMFileNames->clear();
    this->AppHelper->Clear();
    this->Parser->ClearAllDICOMTagCallbacks();

    this->Parser->OpenFile(this->FileName);
    this->AppHelper->RegisterCallbacks(this->Parser);

    this->Parser->ReadHeader();
    this->SetupOutputInformation(1);
    }
  else if (this->DirectoryName)
    {
    vtkDirectory* dir = vtkDirectory::New();
    int opened = dir->Open(this->DirectoryName);
    if (!opened)
      {
      vtkErrorMacro("Couldn't open " << this->DirectoryName);
      dir->Delete();
      return;
      }
    int numFiles = dir->GetNumberOfFiles();

    vtkDebugMacro( << "There are " << numFiles << " files in the directory.");

    this->DICOMFileNames->clear();
    this->AppHelper->Clear();

    for (int i = 0; i < numFiles; i++)
      {
      if (strcmp(dir->GetFile(i), ".") == 0 ||
          strcmp(dir->GetFile(i), "..") == 0)
        {
        continue;
        }

      vtkstd::string fileString = this->DirectoryName;
      fileString += "/";
      fileString += dir->GetFile(i);

      int val = this->CanReadFile(fileString.c_str());

      if (val == 1)
        {
        vtkDebugMacro( << "Adding " << fileString.c_str() << " to DICOMFileNames.");
        this->DICOMFileNames->push_back(fileString);
        }
      else
        {
        vtkDebugMacro( << fileString.c_str() << " - DICOMParser CanReadFile returned : " << val);
        }

      }
    vtkstd::vector<vtkstd::string>::iterator iter;

    for (iter = this->DICOMFileNames->begin();
         iter != this->DICOMFileNames->end();
         iter++)
      {
      const char* fn = iter->c_str();
      vtkDebugMacro( << "Trying : " << fn);

      bool couldOpen = this->Parser->OpenFile(fn);
      if (!couldOpen)
        {
        dir->Delete();
        return;
        }

      //
      this->Parser->ClearAllDICOMTagCallbacks();
      this->AppHelper->RegisterCallbacks(this->Parser);

      this->Parser->ReadHeader();

      vtkDebugMacro( << "File name : " << fn );
      vtkDebugMacro( << "Slice number : " << this->AppHelper->GetSliceNumber());
      }

    vtkstd::vector<vtkstd::pair<float, vtkstd::string> > sortedFiles;

    this->AppHelper->GetImagePositionPatientFilenamePairs(sortedFiles, false);
    this->SetupOutputInformation(static_cast<int>(sortedFiles.size()));

    //this->AppHelper->OutputSeries();

    if (sortedFiles.size() > 0)
      {
      this->DICOMFileNames->clear();
      vtkstd::vector<vtkstd::pair<float, vtkstd::string> >::iterator siter;
      for (siter = sortedFiles.begin();
           siter != sortedFiles.end();
           siter++)
        {
        vtkDebugMacro(<< "Sorted filename : " << (*siter).second.c_str());
        vtkDebugMacro(<< "Adding file " << (*siter).second.c_str() << " at slice : " << (*siter).first);
        this->DICOMFileNames->push_back((*siter).second);
        }
      }
    else
      {
      vtkErrorMacro( << "Couldn't get sorted files. Slices may be in wrong order!");
      }
    dir->Delete();
    }

}

//----------------------------------------------------------------------------
void vtkDICOMImageReader::ExecuteData(vtkDataObject *output)
{
  vtkImageData *data = this->AllocateOutputData(output);

  if (!this->FileName && this->DICOMFileNames->size() == 0)
    {
    vtkErrorMacro( << "Either a filename was not specified or the specified directory does not contain any DICOM images.");
    this->SetErrorCode( vtkErrorCode::NoFileNameError );
    return;
    }

  data->GetPointData()->GetScalars()->SetName("DICOMImage");

  this->ComputeDataIncrements();

  if (this->FileName)
    {
    vtkDebugMacro( << "Single file : " << this->FileName);
    this->Parser->ClearAllDICOMTagCallbacks();
    this->Parser->OpenFile(this->FileName);
    this->AppHelper->Clear();
    this->AppHelper->RegisterCallbacks(this->Parser);
    this->AppHelper->RegisterPixelDataCallback(this->Parser);

    this->Parser->ReadHeader();

    void* imgData = NULL;
    DICOMParser::VRTypes dataType;
    unsigned long imageDataLength;

    this->AppHelper->GetImageData(imgData, dataType, imageDataLength);
    if( !imageDataLength )
      {
      vtkErrorMacro( << "There was a problem retrieving data from: " << this->FileName );
      this->SetErrorCode( vtkErrorCode::FileFormatError );
      return;
      }

    void* buffer = data->GetScalarPointer();
    if (buffer == NULL)
      {
      vtkErrorMacro(<< "No memory allocated for image data!");
      return;
      }
    // DICOM stores the upper left pixel as the first pixel in an
    // image. VTK stores the lower left pixel as the first pixel in
    // an image.  Need to flip the data.
    vtkIdType rowLength;
    rowLength = this->DataIncrements[1];
    unsigned char *b = (unsigned char *)buffer;
    unsigned char *iData = (unsigned char *)imgData;
    iData += (imageDataLength - rowLength); // beginning of last row
    for (int i=0; i < this->AppHelper->GetHeight(); ++i)
      {
      memcpy(b, iData, rowLength);
      b += rowLength;
      iData -= rowLength;
      }
    }
  else if (this->DICOMFileNames->size() > 0)
    {
    vtkDebugMacro( << "Multiple files (" << static_cast<int>(this->DICOMFileNames->size()) << ")");
    this->Parser->ClearAllDICOMTagCallbacks();
    this->AppHelper->Clear();
    this->AppHelper->RegisterCallbacks(this->Parser);
    this->AppHelper->RegisterPixelDataCallback(this->Parser);

    void* buffer = data->GetScalarPointer();
    if (buffer == NULL)
      {
      vtkErrorMacro(<< "No memory allocated for image data!");
      return;
      }

    vtkstd::vector<vtkstd::string>::iterator fiter;

    int count = 0;
    int numFiles = static_cast<int>(this->DICOMFileNames->size());

    for (fiter = this->DICOMFileNames->begin();
         fiter != this->DICOMFileNames->end();
         fiter++)
      {
      count++;
      const char *file = fiter->c_str();
      vtkDebugMacro( << "File : " << file );
      this->Parser->OpenFile( file );
      this->Parser->ReadHeader();

      void* imgData = NULL;
      DICOMParser::VRTypes dataType;
      unsigned long imageDataLengthInBytes;

      this->AppHelper->GetImageData(imgData, dataType, imageDataLengthInBytes);
      if( !imageDataLengthInBytes )
        {
        vtkErrorMacro( << "There was a problem retrieving data from: " << file );
        this->SetErrorCode( vtkErrorCode::FileFormatError );
        return;
        }

      // DICOM stores the upper left pixel as the first pixel in an
      // image. VTK stores the lower left pixel as the first pixel in
      // an image.  Need to flip the data.
      vtkIdType rowLength;
      rowLength = this->DataIncrements[1];
      unsigned char *b = (unsigned char *)buffer;
      unsigned char *iData = (unsigned char *)imgData;
      iData += (imageDataLengthInBytes - rowLength); // beginning of last row
      for (int i=0; i < this->AppHelper->GetHeight(); ++i)
        {
        memcpy(b, iData, rowLength);
        b += rowLength;
        iData -= rowLength;
        }
      buffer = ((char*) buffer) + imageDataLengthInBytes;

      this->UpdateProgress(float(count)/float(numFiles));
      int len = static_cast<int> (strlen((const char*) (*fiter).c_str()));
      char* filename = new char[len+1];
      strcpy(filename, (const char*) (*fiter).c_str());
      this->SetProgressText(filename);
      delete[] filename;
      }
    }
}

//----------------------------------------------------------------------------
void vtkDICOMImageReader::SetupOutputInformation(int num_slices)
{
  int width = this->AppHelper->GetWidth();
  int height = this->AppHelper->GetHeight();
  int bit_depth = this->AppHelper->GetBitsAllocated();
  int num_comp = this->AppHelper->GetNumberOfComponents();

  this->DataExtent[0] = 0;
  this->DataExtent[1] = width - 1;
  this->DataExtent[2] = 0;
  this->DataExtent[3] = height - 1;
  this->DataExtent[4] = 0;
  this->DataExtent[5] = num_slices - 1;

  bool isFloat = this->AppHelper->RescaledImageDataIsFloat();

  bool sign = this->AppHelper->RescaledImageDataIsSigned();

  if (isFloat)
    {
    this->SetDataScalarTypeToFloat();
    }
  else if (bit_depth <= 8)
    {
    this->SetDataScalarTypeToUnsignedChar();
    }
  else
    {
    if (sign)
      {
      this->SetDataScalarTypeToShort();
      }
    else
      {
      this->SetDataScalarTypeToUnsignedShort();
      }
    }
  this->SetNumberOfScalarComponents(num_comp);

  this->GetPixelSpacing();

  this->vtkImageReader2::ExecuteInformation();
}

//----------------------------------------------------------------------------
void vtkDICOMImageReader::SetDirectoryName(const char* dn)
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this <<
                "): setting DirectoryName to " << (dn ? dn : "(null)") );
  if ( this->DirectoryName == NULL && dn == NULL)
    {
    return;
    }
  if (this->FileName)
    {
    delete [] this->FileName;
    this->FileName = NULL;
    }
  if ( this->DirectoryName && dn && (!strcmp(this->DirectoryName,dn)))
    {
    return;
    }
  if (this->DirectoryName)
    {
    delete [] this->DirectoryName;
    }
  if (dn)
    {
    this->DirectoryName = new char[strlen(dn)+1];
    strcpy(this->DirectoryName,dn);
    }
   else
    {
    this->DirectoryName = NULL;
    }
  this->Modified();
}

//----------------------------------------------------------------------------
double* vtkDICOMImageReader::GetPixelSpacing()
{
  vtkstd::vector<vtkstd::pair<float, vtkstd::string> > sortedFiles;

  this->AppHelper->GetImagePositionPatientFilenamePairs(sortedFiles, false);

  float* spacing = this->AppHelper->GetPixelSpacing();
  this->DataSpacing[0] = spacing[0];
  this->DataSpacing[1] = spacing[1];

  if (sortedFiles.size() > 1)
    {
    vtkstd::pair<float, vtkstd::string> p1 = sortedFiles[0];
    vtkstd::pair<float, vtkstd::string> p2 = sortedFiles[1];
    this->DataSpacing[2] = fabs(p1.first - p2.first);
    }
  else
    {
    this->DataSpacing[2] = spacing[2];
    }

  return this->DataSpacing;
}

//----------------------------------------------------------------------------
int vtkDICOMImageReader::GetWidth()
{
  return this->AppHelper->GetWidth();
}

//----------------------------------------------------------------------------
int vtkDICOMImageReader::GetHeight()
{
  return this->AppHelper->GetHeight();
}

//----------------------------------------------------------------------------
float* vtkDICOMImageReader::GetImagePositionPatient()
{
  return this->AppHelper->GetImagePositionPatient();
}

//----------------------------------------------------------------------------
float* vtkDICOMImageReader::GetImageOrientationPatient()
{
  return this->AppHelper->GetImageOrientationPatient();
}

//----------------------------------------------------------------------------
int vtkDICOMImageReader::GetBitsAllocated()
{
  return this->AppHelper->GetBitsAllocated();
}

//----------------------------------------------------------------------------
int vtkDICOMImageReader::GetPixelRepresentation()
{
  return this->AppHelper->GetPixelRepresentation();
}

//----------------------------------------------------------------------------
int vtkDICOMImageReader::GetNumberOfComponents()
{
  return this->AppHelper->GetNumberOfComponents();
}

//----------------------------------------------------------------------------
const char* vtkDICOMImageReader::GetTransferSyntaxUID()
{
  vtkstd::string tmp = this->AppHelper->GetTransferSyntaxUID();

  if (this->TransferSyntaxUID)
    {
    delete [] this->TransferSyntaxUID;
    }
  this->TransferSyntaxUID = new char[tmp.length()+1];
  strcpy(this->TransferSyntaxUID, tmp.c_str());
  this->TransferSyntaxUID[tmp.length()] = '\0';

  return this->TransferSyntaxUID;
}

//----------------------------------------------------------------------------
float vtkDICOMImageReader::GetRescaleSlope()
{
  return this->AppHelper->GetRescaleSlope();
}

//----------------------------------------------------------------------------
float vtkDICOMImageReader::GetRescaleOffset()
{
  return this->AppHelper->GetRescaleOffset();
}

//----------------------------------------------------------------------------
const char* vtkDICOMImageReader::GetPatientName()
{
  vtkstd::string tmp = this->AppHelper->GetPatientName();

  if (this->PatientName)
    {
    delete [] this->PatientName;
    }
  this->PatientName = new char[tmp.length()+1];
  strcpy(this->PatientName, tmp.c_str());
  this->PatientName[tmp.length()] = '\0';

  return this->PatientName;
}

//----------------------------------------------------------------------------
const char* vtkDICOMImageReader::GetStudyUID()
{
  vtkstd::string tmp = this->AppHelper->GetStudyUID();

  if (this->StudyUID)
    {
    delete [] this->StudyUID;
    }
  this->StudyUID = new char[tmp.length()+1];
  strcpy(this->StudyUID, tmp.c_str());
  this->StudyUID[tmp.length()] = '\0';

  return this->StudyUID;
}

//----------------------------------------------------------------------------
const char* vtkDICOMImageReader::GetStudyID()
{
  vtkstd::string tmp = this->AppHelper->GetStudyID();

  if (this->StudyID)
    {
    delete [] this->StudyID;
    }
  this->StudyID = new char[tmp.length()+1];
  strcpy(this->StudyID, tmp.c_str());
  this->StudyID[tmp.length()] = '\0';

  return this->StudyID;
}

//----------------------------------------------------------------------------
float vtkDICOMImageReader::GetGantryAngle()
{
  return this->AppHelper->GetGantryAngle();
}

//----------------------------------------------------------------------------
int vtkDICOMImageReader::GetNumberOfDICOMFileNames()
{
  return static_cast<int>(this->DICOMFileNames->size());
}

//----------------------------------------------------------------------------
const char* vtkDICOMImageReader::GetDICOMFileName(int index)
{
  if(index >= 0 && index < this->GetNumberOfDICOMFileNames())
    {
    return (*this->DICOMFileNames)[index].c_str();
    }
  return 0;
}


/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageWriter.h"

#include "vtkCommand.h"
#include "vtkErrorCode.h"
#include "vtkInformation.h"
#include "vtkInformationExecutivePortKey.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkImageData.h"

#include <vtksys/SystemTools.hxx>

vtkStandardNewMacro(vtkImageWriter);

//----------------------------------------------------------------------------
vtkImageWriter::vtkImageWriter()
{
  this->FilePrefix = NULL;
  this->FilePattern = NULL;
  this->FileName = NULL;
  this->InternalFileName = NULL;
  this->FileNumber = 0;
  this->FileDimensionality = 2;

  this->FilePattern = new char[strlen("%s.%d") + 1];
  strcpy(this->FilePattern, "%s.%d");

  this->FileLowerLeft = 0;

  this->MinimumFileNumber = this->MaximumFileNumber = 0;
  this->FilesDeleted = 0;
  this->SetNumberOfOutputPorts(0);
}



//----------------------------------------------------------------------------
vtkImageWriter::~vtkImageWriter()
{
  // get rid of memory allocated for file names
  delete [] this->FilePrefix;
  this->FilePrefix = NULL;
  delete [] this->FilePattern;
  this->FilePattern = NULL;
  delete [] this->FileName;
  this->FileName = NULL;
}


//----------------------------------------------------------------------------
void vtkImageWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "FileName: " <<
    (this->FileName ? this->FileName : "(none)") << "\n";
  os << indent << "FilePrefix: " <<
    (this->FilePrefix ? this->FilePrefix : "(none)") << "\n";
  os << indent << "FilePattern: " <<
    (this->FilePattern ? this->FilePattern : "(none)") << "\n";

  os << indent << "FileDimensionality: " << this->FileDimensionality << "\n";
}


//----------------------------------------------------------------------------
vtkImageData *vtkImageWriter::GetInput()
{
  if (this->GetNumberOfInputConnections(0) < 1)
  {
    return 0;
  }
  return vtkImageData::SafeDownCast(
    this->GetExecutive()->GetInputData(0, 0));
}
//----------------------------------------------------------------------------

int vtkImageWriter::RequestData(
  vtkInformation* vtkNotUsed( request ),
  vtkInformationVector** inputVector,
  vtkInformationVector* vtkNotUsed( outputVector) )
{
  this->SetErrorCode(vtkErrorCode::NoError);

  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkImageData *input =
    vtkImageData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Error checking
  if (input == NULL )
  {
    vtkErrorMacro(<<"Write:Please specify an input!");
    return 0;
  }
  if ( !this->FileName && !this->FilePattern)
  {
    vtkErrorMacro(<<"Write:Please specify either a FileName or a file prefix and pattern");
    this->SetErrorCode(vtkErrorCode::NoFileNameError);
    return 0;
  }

  // Make sure the file name is allocated
  this->InternalFileName =
    new char[(this->FileName ? strlen(this->FileName) : 1) +
            (this->FilePrefix ? strlen(this->FilePrefix) : 1) +
            (this->FilePattern ? strlen(this->FilePattern) : 1) + 10];

  // Fill in image information.
  int *wExt = inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
  this->FileNumber = wExt[4];
  this->MinimumFileNumber = this->MaximumFileNumber = this->FileNumber;
  this->FilesDeleted = 0;

  // Write
  this->InvokeEvent(vtkCommand::StartEvent);
  this->UpdateProgress(0.0);
  this->RecursiveWrite(2, input, inInfo, NULL);

  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
  {
    this->DeleteFiles();
  }

  this->UpdateProgress(1.0);
  this->InvokeEvent(vtkCommand::EndEvent);

  delete [] this->InternalFileName;
  this->InternalFileName = NULL;

  return 1;
}

//----------------------------------------------------------------------------
// Writes all the data from the input.
void vtkImageWriter::Write()
{
  // we always write, even if nothing has changed, so send a modified
  this->Modified();
  this->UpdateWholeExtent();
}

//----------------------------------------------------------------------------
// Breaks region into pieces with correct dimensionality.
void vtkImageWriter::RecursiveWrite(int axis,
                                    vtkImageData *cache,
                                    vtkInformation* inInfo,
                                    ofstream *file)
{
  vtkImageData    *data;
  int             fileOpenedHere = 0;

  // if we need to open another slice, do it
  if (!file && (axis + 1) == this->FileDimensionality)
  {
    // determine the name
    if (this->FileName)
    {
      sprintf(this->InternalFileName,"%s",this->FileName);
    }
    else
    {
      if (this->FilePrefix)
      {
        sprintf(this->InternalFileName, this->FilePattern,
                this->FilePrefix, this->FileNumber);
      }
      else
      {
        sprintf(this->InternalFileName, this->FilePattern,this->FileNumber);
      }
      if (this->FileNumber < this->MinimumFileNumber)
      {
        this->MinimumFileNumber = this->FileNumber;
      }
      else if (this->FileNumber > this->MaximumFileNumber)
      {
        this->MaximumFileNumber = this->FileNumber;
      }
    }
    // Open the file
#ifdef _WIN32
    file = new ofstream(this->InternalFileName, ios::out | ios::binary);
#else
    file = new ofstream(this->InternalFileName, ios::out);
#endif
    fileOpenedHere = 1;
    if (file->fail())
    {
      vtkErrorMacro("RecursiveWrite: Could not open file " <<
                    this->InternalFileName);
      this->SetErrorCode(vtkErrorCode::CannotOpenFileError);
      delete file;
      return;
    }

    // Subclasses can write a header with this method call.
    int* wExt = vtkStreamingDemandDrivenPipeline::GetWholeExtent(inInfo);
    this->WriteFileHeader(file, cache, wExt);
    file->flush();
    if (file->fail())
    {
      file->close();
      delete file;
      this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
      return;
    }
    ++this->FileNumber;
  }

  // Propagate the update extent so we can determine pipeline size
  vtkStreamingDemandDrivenPipeline* inputExec =
    vtkStreamingDemandDrivenPipeline::SafeDownCast(
      vtkExecutive::PRODUCER()->GetExecutive(inInfo));
  int inputOutputPort = vtkExecutive::PRODUCER()->GetPort(inInfo);
  inputExec->PropagateUpdateExtent(inputOutputPort);

  // just get the data and write it out
#ifndef NDEBUG
  int *ext = inInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
#endif
  vtkDebugMacro("Getting input extent: " << ext[0] << ", " <<
                ext[1] << ", " << ext[2] << ", " << ext[3] << ", " <<
                ext[4] << ", " << ext[5] << endl);
  inputExec->Update(inputOutputPort);
  data = cache;
  this->RecursiveWrite(axis,cache,data,inInfo,file);
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
  {
    this->DeleteFiles();
    return;
  }
  if (file && fileOpenedHere)
  {
    this->WriteFileTrailer(file,cache);
    file->flush();
    if (file->fail())
    {
      this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
    }
    file->close();
    delete file;
  }
  return;
}


//----------------------------------------------------------------------------
// same idea as the previous method, but it knows that the data is ready
void vtkImageWriter::RecursiveWrite(int axis,
                                    vtkImageData *cache,
                                    vtkImageData *data,
                                    vtkInformation* inInfo,
                                    ofstream *file)
{
  int idx, min, max;

  int* wExt = vtkStreamingDemandDrivenPipeline::GetWholeExtent(inInfo);
  // if the file is already open then just write to it
  if (file)
  {
    this->WriteFile(file,data,
                    inInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()),
                    wExt);
    file->flush();
    if (file->fail())
    {
      file->close();
      delete file;
      this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
    }
    return;
  }

  // if we need to open another slice, do it
  if (!file && (axis + 1) == this->FileDimensionality)
  {
    // determine the name
    if (this->FileName)
    {
      sprintf(this->InternalFileName,"%s",this->FileName);
    }
    else
    {
      if (this->FilePrefix)
      {
        sprintf(this->InternalFileName, this->FilePattern,
                this->FilePrefix, this->FileNumber);
      }
      else
      {
        sprintf(this->InternalFileName, this->FilePattern,this->FileNumber);
      }
      if (this->FileNumber < this->MinimumFileNumber)
      {
        this->MinimumFileNumber = this->FileNumber;
      }
      else if (this->FileNumber > this->MaximumFileNumber)
      {
        this->MaximumFileNumber = this->FileNumber;
      }
    }
    // Open the file
#ifdef _WIN32
    file = new ofstream(this->InternalFileName, ios::out | ios::binary);
#else
    file = new ofstream(this->InternalFileName, ios::out);
#endif
    if (file->fail())
    {
      vtkErrorMacro("RecursiveWrite: Could not open file " <<
                    this->InternalFileName);
      this->SetErrorCode(vtkErrorCode::CannotOpenFileError);
      delete file;
      return;
    }

    // Subclasses can write a header with this method call.
    this->WriteFileHeader(file, cache, wExt);
    file->flush();
    if (file->fail())
    {
      file->close();
      delete file;
      this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
      return;
    }
    this->WriteFile(
      file,data,
      inInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()),wExt);
    file->flush();
    if (file->fail())
    {
      file->close();
      delete file;
      this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
      return;
    }
    ++this->FileNumber;
    this->WriteFileTrailer(file,cache);
    file->flush();
    if (file->fail())
    {
      this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
    }
    file->close();
    delete file;
    return;
  }

  // if the current region is too high a dimension forthe file
  // the we will split the current axis
  int* updateExtent = inInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
  cache->GetAxisUpdateExtent(axis, min, max,
                             updateExtent);

  int axisUpdateExtent[6];
  // if it is the y axis then flip by default
  if (axis == 1 && !this->FileLowerLeft)
  {
    for(idx = max; idx >= min; idx--)
    {
      cache->SetAxisUpdateExtent(axis, idx, idx, updateExtent, axisUpdateExtent);
      inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), axisUpdateExtent, 6);
      if (this->ErrorCode != vtkErrorCode::OutOfDiskSpaceError)
      {
        this->RecursiveWrite(axis - 1, cache, data, inInfo, file);
      }
      else
      {
        this->DeleteFiles();
      }
    }
  }
  else
  {
    for(idx = min; idx <= max; idx++)
    {
      cache->SetAxisUpdateExtent(axis, idx, idx, updateExtent, axisUpdateExtent);
      inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
        axisUpdateExtent, 6);
      if (this->ErrorCode != vtkErrorCode::OutOfDiskSpaceError)
      {
        this->RecursiveWrite(axis - 1, cache, data, inInfo, file);
      }
      else
      {
        this->DeleteFiles();
      }
    }
  }

  // restore original extent
  cache->SetAxisUpdateExtent(axis, min, max, updateExtent, axisUpdateExtent);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
    axisUpdateExtent, 6);
}


//----------------------------------------------------------------------------
template <class T>
unsigned long vtkImageWriterGetSize(T*)
{
  return sizeof(T);
}

//----------------------------------------------------------------------------
// Writes a region in a file.  Subclasses can override this method
// to produce a header. This method only hanldes 3d data (plus components).
void vtkImageWriter::WriteFile(ofstream *file, vtkImageData *data,
                               int extent[6], int wExtent[6])
{
  int idxY, idxZ;
  int rowLength; // in bytes
  void *ptr;
  unsigned long count = 0;
  unsigned long target;
  float progress = this->Progress;
  float area;

  // Make sure we actually have data.
  if ( !data->GetPointData()->GetScalars())
  {
    vtkErrorMacro(<< "Could not get data from input.");
    return;
  }

  // take into consideration the scalar type
  switch (data->GetScalarType())
  {
    vtkTemplateMacro(
      rowLength = vtkImageWriterGetSize(static_cast<VTK_TT*>(0))
      );
    default:
      vtkErrorMacro(<< "Execute: Unknown output ScalarType");
      return;
  }
  rowLength *= data->GetNumberOfScalarComponents();
  rowLength *= (extent[1] - extent[0] + 1);

  area = (float) ((extent[5] - extent[4] + 1)*
                  (extent[3] - extent[2] + 1)*
                  (extent[1] - extent[0] + 1)) /
         (float) ((wExtent[5] -wExtent[4] + 1)*
                  (wExtent[3] -wExtent[2] + 1)*
                  (wExtent[1] -wExtent[0] + 1));

  target = (unsigned long)((extent[5]-extent[4]+1)*
                           (extent[3]-extent[2]+1)/(50.0*area));
  target++;

  int ystart = extent[3];
  int yend = extent[2] - 1;
  int yinc = -1;
  if (this->FileLowerLeft)
  {
    ystart = extent[2];
    yend = extent[3]+1;
    yinc = 1;
  }

  for (idxZ = extent[4]; idxZ <= extent[5]; ++idxZ)
  {
    for (idxY = ystart; idxY != yend; idxY = idxY + yinc)
    {
      if (!(count%target))
      {
        this->UpdateProgress(progress + count/(50.0*target));
      }
      count++;
      ptr = data->GetScalarPointer(extent[0], idxY, idxZ);
      if ( ! file->write((char *)ptr, rowLength))
      {
        return;
      }
    }
  }
}

void vtkImageWriter::DeleteFiles()
{
  if (this->FilesDeleted)
  {
    return;
  }
  int i;
  char *fileName;

  vtkErrorMacro("Ran out of disk space; deleting file(s) already written");

  if (this->FileName)
  {
    vtksys::SystemTools::RemoveFile(this->FileName);
  }
  else
  {
    if (this->FilePrefix)
    {
      fileName =
        new char[strlen(this->FilePrefix) + strlen(this->FilePattern) + 10];

      for (i = this->MinimumFileNumber; i <= this->MaximumFileNumber; i++)
      {
        sprintf(fileName, this->FilePattern, this->FilePrefix, i);
        vtksys::SystemTools::RemoveFile(fileName);
      }
      delete [] fileName;
    }
    else
    {
      fileName = new char[strlen(this->FilePattern) + 10];

      for (i = this->MinimumFileNumber; i <= this->MaximumFileNumber; i++)
      {
        sprintf(fileName, this->FilePattern, i);
        vtksys::SystemTools::RemoveFile(fileName);
      }
      delete [] fileName;
    }
  }
  this->FilesDeleted = 1;
}

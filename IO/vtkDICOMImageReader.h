
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDICOMImageReader.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkDICOMImageReader - Reads DICOM images
// .SECTION Description
// .SECTION See Also
// vtkBMPReader vtkPNMReader vtkTIFFReader

#ifndef __vtkDICOMImageReader_h
#define __vtkDICOMImageReader_h

#include "vtkImageReader2.h"

//BTX
class vtkDICOMImageReaderVector;
class DICOMParser;
class DICOMAppHelper;
//ETX

class VTK_IO_EXPORT vtkDICOMImageReader : public vtkImageReader2
{
 public:
  // Description:
  // Static method for construction.
  static vtkDICOMImageReader *New();
  vtkTypeRevisionMacro(vtkDICOMImageReader,vtkImageReader2);

  // Description:
  // Prints the ivars.
  void PrintSelf(ostream& os, vtkIndent indent);   
  
  // Description:
  // Set the filename for the file to read. If this method is used,
  // the reader will only read a single file.
  void SetFileName(const char* fn)
  {
    this->DirectoryName = NULL;
    this->vtkImageReader2::SetFileName(fn);
  }

  // Description:
  // Returns the filename.
  vtkGetStringMacro(FileName);

  // Description:
  // Set the directory name for the reader to look in for DICOM
  // files. If this method is used, the reader will try to find 
  // all the DICOM files in a directory. It will select the subset 
  // corresponding to the first series UID it stumbles across and
  // it will try to build an ordered volume from them based on
  // the slice number. The volume building will be upgraded to
  // something more sophisticated in the future.
  void SetDirectoryName(const char* dn);
  
  // Description:
  // Returns the directory name.
  vtkGetStringMacro(DirectoryName);

protected:
  //
  // Setup the volume size
  //
  void SetupOutputInformation(int num_slices);

  //
  // Can I read the file?
  // 
  virtual int CanReadFile(const char* fname);

  //
  // What file extensions are supported?
  // 
  virtual const char* GetFileExensions()
  {
    return ".dcm";
  }

  // Description: 
  // Return a descriptive name for the file format that might be useful in a GUI.
  virtual const char* GetDescriptiveName()
  {
    return "DICOM";
  }
  
  virtual void ExecuteInformation();
  virtual void ExecuteData(vtkDataObject *out);

  //
  // Constructor 
  //
  vtkDICOMImageReader();

  //
  // Destructor
  // 
  virtual ~vtkDICOMImageReader();

  //
  // Instance of the parser used to parse the file.
  //
  DICOMParser* Parser;

  //
  // Instance of the callbacks that get the data from the file.
  //
  DICOMAppHelper* AppHelper;
  
  //
  // vtkDICOMImageReaderVector wants to be a PIMPL and it will be, but not quite yet.
  //
  vtkDICOMImageReaderVector* DICOMFileNames;
  char* DirectoryName;

private:
  vtkDICOMImageReader(const vtkDICOMImageReader&);  // Not implemented.
  void operator=(const vtkDICOMImageReader&);  // Not implemented.

};

#endif

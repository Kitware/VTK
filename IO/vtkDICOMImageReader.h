
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

#include "DICOMParser.h"
#include "DICOMAppHelper.h"
#include "vtkImageReader2.h"

//BTX
class myvector;
//ETX

class VTK_IO_EXPORT vtkDICOMImageReader : public vtkImageReader2
{
 public:
  static vtkDICOMImageReader *New();
  vtkTypeRevisionMacro(vtkDICOMImageReader,vtkImageReader2);
  void PrintSelf(ostream& os, vtkIndent indent);   
  
  void SetFileName(const char* fn)
    {
    this->DirectoryName = NULL;
    this->vtkImageReader2::SetFileName(fn);
    }
  //vtkSetStringMacro(DirectoryName);
  void SetDirectoryName(const char* dn)
    {
    if (dn == NULL)
      {
      return;
      }
    int len = strlen(dn);
    if (this->DirectoryName != NULL)
      {
      delete [] this->DirectoryName;
      }
    this->DirectoryName = new char[len+1];
    strcpy(this->DirectoryName, dn);
    //this->DirectoryName = dirName;
    this->FileName = NULL;
    this->Modified();
    }
  //vtkGetStringMacro(DirectoryName);
  const char* GetDirectoryName()
    {
    return this->DirectoryName;
    }

 protected:
  void SetupOutputInformation(int num_slices);
  virtual int CanReadFile(const char* fname);

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

  
  vtkDICOMImageReader();
  ~vtkDICOMImageReader();

  DICOMParser* Parser;
  DICOMAppHelper* AppHelper;
  
  myvector* DICOMFileNames;
  char* DirectoryName;

};

#endif

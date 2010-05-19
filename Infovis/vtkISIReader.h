/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkISIReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
// .NAME vtkISIReader - reader for ISI files
//
// .SECTION Description
// ISI is a tagged format for expressing bibliographic citations.  Data is
// structured as a collection of records with each record composed of
// one-to-many fields.  See
//
// http://isibasic.com/help/helpprn.html#dialog_export_format
//
// for details.  vtkISIReader will convert an ISI file into a vtkTable, with
// the set of table columns determined dynamically from the contents of the
// file.

#ifndef __vtkISIReader_h
#define __vtkISIReader_h

#include "vtkTableAlgorithm.h"

class vtkTable;

class VTK_INFOVIS_EXPORT vtkISIReader : public vtkTableAlgorithm
{
public:
  static vtkISIReader* New();
  vtkTypeMacro(vtkISIReader,vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/get the file to load
  vtkGetStringMacro(FileName);
  vtkSetStringMacro(FileName);
  
  // Description:
  // Set/get the delimiter to be used for concatenating field data (default: ";")
  vtkGetStringMacro(Delimiter);
  vtkSetStringMacro(Delimiter);

  // Description:
  // Set/get the maximum number of records to read from the file (zero = unlimited)
  vtkGetMacro(MaxRecords,int);
  vtkSetMacro(MaxRecords,int);

 protected:
  vtkISIReader();
  ~vtkISIReader();

  int RequestData(
    vtkInformation*, 
    vtkInformationVector**, 
    vtkInformationVector*);

  char* FileName;
  char* Delimiter;
  int MaxRecords;

private:
  vtkISIReader(const vtkISIReader&); // Not implemented
  void operator=(const vtkISIReader&);   // Not implemented
};

#endif

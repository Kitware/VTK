/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPImageDataReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLPImageDataReader - Read PVTK XML ImageData files.
// .SECTION Description
// vtkXMLPImageDataReader reads the PVTK XML ImageData file format.
// This reads the parallel format's summary file and then uses
// vtkXMLImageDataReader to read data from the individual ImageData
// piece files.  Streaming is supported.  The standard extension for
// this reader's file format is "pvti".

// .SECTION See Also
// vtkXMLImageDataReader

#ifndef __vtkXMLPImageDataReader_h
#define __vtkXMLPImageDataReader_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLPStructuredDataReader.h"

class vtkImageData;

class VTKIOXML_EXPORT vtkXMLPImageDataReader : public vtkXMLPStructuredDataReader
{
public:
  vtkTypeMacro(vtkXMLPImageDataReader,vtkXMLPStructuredDataReader);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkXMLPImageDataReader *New();

  // Description:
  // Get the reader's output.
  vtkImageData *GetOutput();
  vtkImageData *GetOutput(int idx);

  // For the specified port, copy the information this reader sets up in
  // SetupOutputInformation to outInfo
  virtual void CopyOutputInformation(vtkInformation *outInfo, int port);

protected:
  vtkXMLPImageDataReader();
  ~vtkXMLPImageDataReader();

  double Origin[3];
  double Spacing[3];

  vtkImageData* GetPieceInput(int index);

  void SetupEmptyOutput();
  const char* GetDataSetName();
  void SetOutputExtent(int* extent);
  void GetPieceInputExtent(int index, int* extent);
  int ReadPrimaryElement(vtkXMLDataElement* ePrimary);

  // Setup the output's information.
  void SetupOutputInformation(vtkInformation *outInfo);

  vtkXMLDataReader* CreatePieceReader();
  virtual int FillOutputPortInformation(int, vtkInformation*);

private:
  vtkXMLPImageDataReader(const vtkXMLPImageDataReader&);  // Not implemented.
  void operator=(const vtkXMLPImageDataReader&);  // Not implemented.
};

#endif

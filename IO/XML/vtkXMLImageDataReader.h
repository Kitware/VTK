/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLImageDataReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkXMLImageDataReader
 * @brief   Read VTK XML ImageData files.
 *
 * vtkXMLImageDataReader reads the VTK XML ImageData file format.  One
 * image data file can be read to produce one output.  Streaming is
 * supported.  The standard extension for this reader's file format is
 * "vti".  This reader is also used to read a single piece of the
 * parallel file format.
 *
 * @sa
 * vtkXMLPImageDataReader
*/

#ifndef vtkXMLImageDataReader_h
#define vtkXMLImageDataReader_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLStructuredDataReader.h"

class vtkImageData;

class VTKIOXML_EXPORT vtkXMLImageDataReader : public vtkXMLStructuredDataReader
{
public:
  vtkTypeMacro(vtkXMLImageDataReader,vtkXMLStructuredDataReader);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  static vtkXMLImageDataReader *New();

  //@{
  /**
   * Get the reader's output.
   */
  vtkImageData *GetOutput();
  vtkImageData *GetOutput(int idx);
  //@}

  /**
   * For the specified port, copy the information this reader sets up in
   * SetupOutputInformation to outInfo
   */
  void CopyOutputInformation(vtkInformation *outInfo, int port) VTK_OVERRIDE;

protected:
  vtkXMLImageDataReader();
  ~vtkXMLImageDataReader() VTK_OVERRIDE;

  double Origin[3];
  double Spacing[3];
  int PieceExtent[6];

  const char* GetDataSetName() VTK_OVERRIDE;
  void SetOutputExtent(int* extent) VTK_OVERRIDE;

  int ReadPrimaryElement(vtkXMLDataElement* ePrimary) VTK_OVERRIDE;

  // Setup the output's information.
  void SetupOutputInformation(vtkInformation *outInfo) VTK_OVERRIDE;

  int FillOutputPortInformation(int, vtkInformation*) VTK_OVERRIDE;



private:
  vtkXMLImageDataReader(const vtkXMLImageDataReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkXMLImageDataReader&) VTK_DELETE_FUNCTION;
};

#endif

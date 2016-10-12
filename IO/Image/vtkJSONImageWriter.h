/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkJSONImageWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkJSONImageWriter
 * @brief   Writes vtkImageData to a JSON file.
 *
 * vtkJSONImageWriter writes a JSON file which will describe the
 * data inside a vtkImageData.
*/

#ifndef vtkJSONImageWriter_h
#define vtkJSONImageWriter_h

#include "vtkIOImageModule.h" // For export macro
#include "vtkImageAlgorithm.h"

class VTKIOIMAGE_EXPORT vtkJSONImageWriter : public vtkImageAlgorithm
{
public:
  static vtkJSONImageWriter *New();
  vtkTypeMacro(vtkJSONImageWriter,vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * Specify file name for the image file.
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Specify ArrayName to export. By default NULL which will dump ALL arrays.
   */
  vtkSetStringMacro(ArrayName);
  vtkGetStringMacro(ArrayName);
  //@}

  //@{
  /**
   * Specify Slice in Z to export. By default -1 which will dump the full 3D domain.
   */
  vtkSetMacro(Slice, int);
  vtkGetMacro(Slice, int);
  //@}

  /**
   * The main interface which triggers the writer to start.
   */
  virtual void Write();

protected:
  vtkJSONImageWriter();
  ~vtkJSONImageWriter();

  char *FileName;
  char *ArrayName;
  int Slice;

  virtual int RequestData(vtkInformation *request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);

private:
  vtkJSONImageWriter(const vtkJSONImageWriter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkJSONImageWriter&) VTK_DELETE_FUNCTION;
};

#endif

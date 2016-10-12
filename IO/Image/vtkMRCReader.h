/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMRCReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkMRCReader
 * @brief   read MRC image files
 *
 *
 * A reader to load MRC images.  See http://bio3d.colorado.edu/imod/doc/mrc_format.txt
 * for the file format specification.
*/

#ifndef vtkMRCReader_h
#define vtkMRCReader_h

#include "vtkImageAlgorithm.h"
#include "vtkIOImageModule.h" // For export macro

class vtkInformation;
class vtkInformationVector;

class VTKIOIMAGE_EXPORT vtkMRCReader : public vtkImageAlgorithm
{
public:
  static vtkMRCReader* New();
  vtkTypeMacro(vtkMRCReader, vtkImageAlgorithm)

  void PrintSelf(ostream& stream, vtkIndent indent);

  // .Description
  // Get/Set the file to read
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

protected:
  vtkMRCReader();
  virtual ~vtkMRCReader();

  virtual int RequestInformation(vtkInformation* request,
                                 vtkInformationVector** inputVector,
                                 vtkInformationVector* outputVector);
  virtual void ExecuteDataWithInformation(vtkDataObject *output,
                                          vtkInformation* outInfo);

  char* FileName;

private:
  vtkMRCReader(const vtkMRCReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkMRCReader&) VTK_DELETE_FUNCTION;
  class vtkInternal;
  vtkInternal* Internals;

};

#endif

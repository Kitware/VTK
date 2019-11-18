/*=========================================================================
  Copyright (c) GeometryFactory
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkSEPReader
 * @brief Stanford Exploration Project files reader.
 *
 * This reader takes a .H file that points to a .H@ file and contains
 * all the information to interpret the raw data in the  .H@ file.
 * The only supported data_format are xdr_float and native_float,
 * with a esize of 4.
 */

#ifndef vtkSEPReader_h
#define vtkSEPReader_h

#include <vtkImageReader.h>

#include <string> //for string

class VTKIOIMAGE_EXPORT vtkSEPReader : public vtkImageReader
{
public:
  static vtkSEPReader* New();
  vtkTypeMacro(vtkSEPReader, vtkImageReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Check if the given file is a .H file
   */
  int CanReadFile(const char* fname) override;

  const char* GetFileExtensions() override { return ".H"; }

protected:
  vtkSEPReader();
  ~vtkSEPReader() override = default;

  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int ReadHeader();

  std::string DataFile;

private:
  vtkSEPReader(const vtkSEPReader&) = delete;
  void operator=(const vtkSEPReader&) = delete;
};

#endif

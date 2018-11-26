/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataArrayHelper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkDataArrayHelper
 * @brief   Helper to manipulate vtkDataArray for metadata extraction and read/write
 */

#ifndef vtkDataArrayHelper_h
#define vtkDataArrayHelper_h

#include "vtkIOWebModule.h" // For export macro
#include <string>           // used as return

class vtkDataArray;

class VTKIOWEB_EXPORT vtkDataArrayHelper
{
public:
  static void ComputeMD5(const unsigned char* content, int size, std::string& hash);
  static std::string GetShortType(vtkDataArray* input, bool& needConversion);
  static std::string GetUID(vtkDataArray*, bool& needConversion);
  static bool WriteArray(vtkDataArray*, const char* filePath);

protected:
  vtkDataArrayHelper();
  ~vtkDataArrayHelper();

private:
  vtkDataArrayHelper(const vtkDataArrayHelper&) = delete;
  void operator=(const vtkDataArrayHelper&) = delete;
};

#endif
// VTK-HeaderTest-Exclude: vtkDataArrayHelper.h

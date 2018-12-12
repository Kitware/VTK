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
  //@{
  /**
   * Compute a MD5 digest of a void/(const unsigned char) pointer to compute a
   *  string hash
   */
  static void ComputeMD5(const unsigned char* content, int size, std::string& hash);
  //@}

  //@{
  /**
   * Compute the target JavaScript typed array name for the given vtkDataArray
   * (Uin8, Uint16, Uin32, Int8, Int16, Int32, Float32, Float64) or
   * "xxx" if no match found
   *
   * Since Uint64 and Int64 does not exist in JavaScript, the needConversion
   * argument will be set to true and Uint32/Int32 will be returned instead.
   */
  static std::string GetShortType(vtkDataArray* input, bool& needConversion);
  //@}

  //@{
  /**
   * Return a Unique identifier for that array
   * (i.e.: Float32_356_13f880891af7b77262c49cae09a41e28 )
   */
  static std::string GetUID(vtkDataArray*, bool& needConversion);
  //@}

  //@{
  /**
   * Write the content of the vtkDataArray to disk based on the filePath
   * provided without any extra information. Just the raw data will be
   * written.
   *
   * If vtkDataArray is a Uint64 or Int64, the data will be converted
   * to Uint32 or Int32 before being written.
   */
  static bool WriteArray(vtkDataArray*, const char* filePath);
  //@}

protected:
  vtkDataArrayHelper();
  ~vtkDataArrayHelper();

private:
  vtkDataArrayHelper(const vtkDataArrayHelper&) = delete;
  void operator=(const vtkDataArrayHelper&) = delete;
};

#endif
// VTK-HeaderTest-Exclude: vtkDataArrayHelper.h

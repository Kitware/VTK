/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkJSONDataSetWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkJSONDataSetWriter
 * @brief   write vtkDataSet using a vtkArchiver with a JSON meta file along
 *          with all the binary arrays written as standalone binary files.
 *          The generated format can be used by vtk.js using the reader below
 *          https://kitware.github.io/vtk-js/examples/HttpDataSetReader.html
 *
 * vtkJSONDataSetWriter writes vtkImageData / vtkPolyData into a set of files
 * representing each arrays that compose the dataset along with a JSON meta file
 * that describe what they are and how they should be assembled into an actual
 * vtkDataSet.
 *
 *
 * @warning
 * This writer assume LittleEndian by default. Additional work should be done to
 * properly
 * handle endianness.
 *
 *
 * @sa
 * vtkArchiver
 */

#ifndef vtkJSONDataSetWriter_h
#define vtkJSONDataSetWriter_h

#include "vtkIOExportModule.h" // For export macro

#include "vtkWriter.h"

#include <string> // std::string used as parameters in a few methods

class vtkDataSet;
class vtkDataArray;
class vtkDataSetAttributes;
class vtkArchiver;

class VTKIOEXPORT_EXPORT vtkJSONDataSetWriter : public vtkWriter
{
public:
  using vtkWriter::Write;

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
   * Return a Unique identifier for any invalid string
   */
  std::string GetValidString(const char*);
  //@}

  //@{
  /**
   * Write the contents of the vtkDataArray to disk based on the filePath
   * provided without any extra information. Just the raw data will be
   * written.
   *
   * If vtkDataArray is a Uint64 or Int64, the data will be converted
   * to Uint32 or Int32 before being written.
   */
  bool WriteArrayContents(vtkDataArray*, const char* relativeFilePath);
  //@}

  //@{
  /**
   * For backwards compatiblity, this static method writes a data array's
   * contents directly to a file.
   */
  static bool WriteArrayAsRAW(vtkDataArray*, const char* filePath);
  //@}

  static vtkJSONDataSetWriter* New();
  vtkTypeMacro(vtkJSONDataSetWriter, vtkWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Specify file name of vtk data file to write.
   * This correspond to the root directory of the data to write.
   * DEPRECATED: use the Archive's API instead.
   */
  VTK_LEGACY(void SetFileName(const char*));
  VTK_LEGACY(virtual char* GetFileName());
  //@}

  //@{
  /**
   * Get the input to this writer.
   */
  vtkDataSet* GetInput();
  vtkDataSet* GetInput(int port);
  //@}

  //@{
  /**
   * Specify the Scene Archiver object
   */
  virtual void SetArchiver(vtkArchiver*);
  vtkGetObjectMacro(Archiver, vtkArchiver);
  //@}

  void Write(vtkDataSet*);

  bool IsDataSetValid() { return this->ValidDataSet; }

protected:
  vtkJSONDataSetWriter();
  ~vtkJSONDataSetWriter() override;

  void WriteData() final;
  std::string WriteArray(vtkDataArray*, const char* className, const char* arrayName = nullptr);
  std::string WriteDataSetAttributes(vtkDataSetAttributes* fields, const char* className);

  vtkArchiver* Archiver;
  bool ValidDataSet;
  int ValidStringCount;

  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkJSONDataSetWriter(const vtkJSONDataSetWriter&) = delete;
  void operator=(const vtkJSONDataSetWriter&) = delete;
};

#endif

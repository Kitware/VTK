/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHttpDataSetWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkHttpDataSetWriter
 * @brief   write vtkDataSet into a directory with a JSON meta file along
 *          with all the binary arrays written as standalone binary files.
 *          The generated format can be used by vtk.js using the reader below
 *          https://kitware.github.io/vtk-js/examples/HttpDataSetReader.html
 *
 * vtkHttpDataSetWriter writes vtkImageData / vtkPolyData into a set of files
 * representing each arrays that compose the dataset along with a JSON meta file
 * that describe what they are and how they should be assembled into an actual vtkDataSet.
 *
 *
 * @warning
 * This writer assume LittleEndian by default. Additional work should be done to properly
 * handle endianness.
 */

#ifndef vtkHttpDataSetWriter_h
#define vtkHttpDataSetWriter_h

#include "vtkIOWebModule.h" // For export macro
#include "vtkWriter.h"

#include <string> // For string parameter

class vtkDataSet;
class vtkDataArray;
class vtkDataSetAttributes;

class VTKIOWEB_EXPORT vtkHttpDataSetWriter : public vtkWriter
{
public:
  static vtkHttpDataSetWriter* New();
  vtkTypeMacro(vtkHttpDataSetWriter, vtkWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Specify file name of vtk data file to write.
   * This correspond to the root directory of the data to write.
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Get the input to this writer.
   */
  vtkDataSet* GetInput();
  vtkDataSet* GetInput(int port);
  //@}

  bool IsDataSetValid() { return this->ValidDataSet; }

protected:
  vtkHttpDataSetWriter();
  ~vtkHttpDataSetWriter() override;

  void WriteData() override;
  std::string WriteArray(vtkDataArray*, const char* className, const char* arrayName = nullptr);
  std::string WriteDataSetAttributes(vtkDataSetAttributes* fields, const char* className);

  char* FileName;
  bool ValidDataSet;

  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkHttpDataSetWriter(const vtkHttpDataSetWriter&) = delete;
  void operator=(const vtkHttpDataSetWriter&) = delete;
};

#endif

// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkXMLImageDataWriter
 * @brief   Write VTK XML ImageData files.
 *
 * vtkXMLImageDataWriter writes the VTK XML ImageData file format.
 * One image data input can be written into one file in any number of
 * streamed pieces.  The standard extension for this writer's file
 * format is "vti".  This writer is also used to write a single piece
 * of the parallel file format.
 *
 * @sa
 * vtkXMLPImageDataWriter
 */

#ifndef vtkXMLImageDataWriter_h
#define vtkXMLImageDataWriter_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLStructuredDataWriter.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkImageData;

class VTKIOXML_EXPORT vtkXMLImageDataWriter : public vtkXMLStructuredDataWriter
{
public:
  static vtkXMLImageDataWriter* New();
  vtkTypeMacro(vtkXMLImageDataWriter, vtkXMLStructuredDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Get/Set the writer's input.
   */
  vtkImageData* GetInput();

  /**
   * Get the default file extension for files written by this writer.
   */
  const char* GetDefaultFileExtension() override;

protected:
  vtkXMLImageDataWriter();
  ~vtkXMLImageDataWriter() override;

  // see algorithm for more info
  int FillInputPortInformation(int port, vtkInformation* info) override;

  void WritePrimaryElementAttributes(ostream& os, vtkIndent indent) override;
  void GetInputExtent(int* extent) override;
  const char* GetDataSetName() override;

private:
  vtkXMLImageDataWriter(const vtkXMLImageDataWriter&) = delete;
  void operator=(const vtkXMLImageDataWriter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif

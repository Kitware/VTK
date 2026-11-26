// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkPSEPReader
 * @brief   Read SEPfiles efficiently from parallel file systems (and reasonably well elsewhere).
 *
 * vtkPSEPReader is a subclass of vtkSEPReader that will read SEP format
 * header information of the image before reading the data.  This means that the
 * reader will automatically set information like file dimensions.
 *
 */

#ifndef vtkPSEPReader_h
#define vtkPSEPReader_h

#include "vtkIOMPIImageModule.h" // For export macro
#include "vtkSEPReader.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkMultiProcessController;

class VTKIOMPIIMAGE_EXPORT vtkPSEPReader : public vtkSEPReader
{
public:
  vtkTypeMacro(vtkPSEPReader, vtkSEPReader);
  static vtkPSEPReader* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/set the multi process controller to use for coordinated reads.  By
   * default, set to the global controller.
   */
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  virtual void SetController(vtkMultiProcessController*);
  ///@}

protected:
  vtkPSEPReader();
  ~vtkPSEPReader() override;

  /**
   * Override reading header to read it only on rank 0 and then broadcast it to other ranks
   */
  bool ReadHeader() override;

private:
  vtkPSEPReader(const vtkPSEPReader&) = delete;
  void operator=(const vtkPSEPReader&) = delete;

  vtkMultiProcessController* Controller = nullptr;
};

VTK_ABI_NAMESPACE_END
#endif // vtkPSEPReader_h

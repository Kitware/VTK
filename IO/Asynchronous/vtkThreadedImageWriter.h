// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class    vtkThreadedImageWriter
 * @brief    class used to compress/write images using threads to prevent
 *           locking while encoding data.
 *
 * @details  This writer allow to encode an image data based on its file
 *           extension: tif, tiff, bpm, png, jpg, jpeg, vti, Z, ppm, raw
 *
 * @author   Patricia Kroll Fasel @ LANL
 */

#ifndef vtkThreadedImageWriter_h
#define vtkThreadedImageWriter_h

#include "vtkIOAsynchronousModule.h" // For export macro
#include "vtkObject.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkImageData;

class VTKIOASYNCHRONOUS_EXPORT vtkThreadedImageWriter : public vtkObject
{
public:
  static vtkThreadedImageWriter* New();
  vtkTypeMacro(vtkThreadedImageWriter, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Need to be called at least once before using the class.
   * Then it should be called again after any change on the
   * thread count or if Finalize() was called.
   *
   * This method will wait for any running thread to terminate and start
   * a new pool with the given number of threads.
   */
  void Initialize();

  /**
   * Push an image into the threaded writer. It is not safe to modify the image
   * after this point.
   * You may run into thread safety issues. Typically, the caller code will
   * simply release reference to the data and stop using it.
   */
  void EncodeAndWrite(vtkImageData* image, VTK_FILEPATH const char* fileName);

  /**
   * Define the number of worker thread to use.
   * Initialize() need to be called after any thread count change.
   */
  void SetMaxThreads(vtkTypeUInt32);
  vtkGetMacro(MaxThreads, vtkTypeUInt32);

  /**
   * This method will wait for any running thread to terminate.
   */
  void Finalize();

protected:
  vtkThreadedImageWriter();
  ~vtkThreadedImageWriter() override;

private:
  vtkThreadedImageWriter(const vtkThreadedImageWriter&) = delete;
  void operator=(const vtkThreadedImageWriter&) = delete;

  class vtkInternals;
  vtkInternals* Internals;
  vtkTypeUInt32 MaxThreads;
};

VTK_ABI_NAMESPACE_END
#endif

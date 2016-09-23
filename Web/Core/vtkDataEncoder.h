/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataEncoder.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkDataEncoder
 * @brief   class used to compress/encode images using threads.
 *
 * vtkDataEncoder is used to compress and encode images using threads.
 * Multiple images can be pushed into the encoder for compression and encoding.
 * We use a vtkTypeUInt32 as the key to identify different image pipes. The
 * images in each pipe will be processed in parallel threads. The latest
 * compressed and encoded image can be accessed using GetLatestOutput().
 *
 * vtkDataEncoder uses a thread-pool to do the compression and encoding in
 * parallel.  Note that images may not come out of the vtkDataEncoder in the
 * same order as they are pushed in, if an image pushed in at N-th location
 * takes longer to compress and encode than that pushed in at N+1-th location or
 * if it was pushed in before the N-th location was even taken up for encoding
 * by the a thread in the thread pool.
*/

#ifndef vtkDataEncoder_h
#define vtkDataEncoder_h

#include "vtkObject.h"
#include "vtkWebCoreModule.h" // needed for exports
#include "vtkSmartPointer.h" // needed for vtkSmartPointer

class vtkUnsignedCharArray;
class vtkImageData;

class VTKWEBCORE_EXPORT vtkDataEncoder : public vtkObject
{
public:
  static vtkDataEncoder* New();
  vtkTypeMacro(vtkDataEncoder, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Re-initializes the encoder. This will abort any on going encoding threads
   * and clear internal data-structures.
   */
  void Initialize();

  /**
   * Push an image into the encoder. It is not safe to modify the image
   * after this point, including changing the reference counts for it.
   * You may run into thread safety issues. Typically,
   * the caller code will simply release reference to the data and stop using
   * it. vtkDataEncoder takes over the reference for the image and will call
   * vtkObject::UnRegister() on it when it's done.
   */
  void PushAndTakeReference(vtkTypeUInt32 key, vtkImageData* &data, int quality);

  /**
   * Get access to the most-recent fully encoded result corresponding to the
   * given key, if any. This methods returns true if the \c data obtained is the
   * result from the most recent Push() for the key, if any. If this method
   * returns false, it means that there's some image either being processed on
   * pending processing.
   */
  bool GetLatestOutput(vtkTypeUInt32 key,vtkSmartPointer<vtkUnsignedCharArray>& data);

  /**
   * Flushes the encoding pipe and blocks till the most recently pushed image
   * for the particular key has been processed. This call will block. Once this
   * method returns, caller can use GetLatestOutput(key) to access the processed
   * output.
   */
  void Flush(vtkTypeUInt32 key);

  /**
   * Take an image data and synchronously convert it to a base-64 encoded png.
   */
  const char* EncodeAsBase64Png(vtkImageData* img, int compressionLevel=5);

  /**
   * Take an image data and synchronously convert it to a base-64 encoded jpg.
   */
  const char* EncodeAsBase64Jpg(vtkImageData* img, int quality=50);

protected:
  vtkDataEncoder();
  ~vtkDataEncoder();

private:
  vtkDataEncoder(const vtkDataEncoder&) VTK_DELETE_FUNCTION;
  void operator=(const vtkDataEncoder&) VTK_DELETE_FUNCTION;

  class vtkInternals;
  vtkInternals* Internals;

};

#endif

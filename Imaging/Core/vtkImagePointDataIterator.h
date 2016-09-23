/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImagePointDataIterator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImagePointDataIterator
 * @brief   iterate over point data in an image.
 *
 * This class will iterate over an image.  For each position, it will
 * provide the (I,J,K) index, the point Id, and if a stencil is supplied,
 * it will report whether the point is inside or outside of the stencil.
 * <p>For efficiency, this class iterates over spans rather than points.
 * Each span is one image row or partial row, defined by a start position
 * and a size.  Within a span, only the X index and the point Id will change.
 * The vtkImagePointDataIterator and related iterators are the preferred
 * method of iterating over image data within the VTK image filters.
 * @sa
 * vtkImageData vtkImageStencilData vtkImageProgressIterator
*/

#ifndef vtkImagePointDataIterator_h
#define vtkImagePointDataIterator_h

#include "vtkSystemIncludes.h"
#include "vtkImagingCoreModule.h" // for export macro

class vtkDataArray;
class vtkImageData;
class vtkImageStencilData;
class vtkAlgorithm;

class VTKIMAGINGCORE_EXPORT vtkImagePointDataIterator
{
public:
  /**
   * Default constructor, its use must be followed by Initialize().
   */
  vtkImagePointDataIterator();

  /**
   * Create an iterator for the given image, with several options.
   * If a stencil is provided, then the iterator's IsInStencil() method
   * reports whether each span is inside the stencil.  If an extent is
   * provided, it iterates over the extent and ignores the rest of the
   * image (the provided extent must be within the image extent).  If
   * a pointer to the algorithm is provided and threadId is set to zero,
   * then progress events will provided for the algorithm.
   */
  vtkImagePointDataIterator(vtkImageData *image,
                            const int extent[6] = 0,
                            vtkImageStencilData *stencil=0,
                            vtkAlgorithm *algorithm=0,
                            int threadId=0)
  {
    this->Initialize(image, extent, stencil, algorithm, threadId);
  }

  /**
   * Initialize an iterator.  See constructor for more details.
   */
  void Initialize(vtkImageData *image, const int extent[6] = 0,
                  vtkImageStencilData *stencil=0,
                  vtkAlgorithm *algorithm=0, int threadId=0);

  /**
   * Move the iterator to the beginning of the next span.
   * A span is a contiguous region of the image over which nothing but
   * the point Id and the X index changes.
   */
  void NextSpan();

  /**
   * Test if the iterator has completed iterating over the entire extent.
   */
  bool IsAtEnd()
  {
    return (this->Id == this->End);
  }

  /**
   * Check if the iterator is within the region specified by the stencil.
   * This is updated when NextSpan() is called.
   */
  bool IsInStencil()
  {
    return this->InStencil;
  }

  //@{
  /**
   * Get the index at the beginning of the current span.
   */
  void GetIndex(int result[3])
  {
    result[0] = this->Index[0];
    result[1] = this->Index[1];
    result[2] = this->Index[2];
  }
  //@}

  /**
   * Get the index at the beginning of the current span.
   */
  const int *GetIndex()
  {
    return this->Index;
  }

  /**
   * Get the point Id at the beginning of the current span.
   */
  vtkIdType GetId()
  {
    return this->Id;
  }

  /**
   * Get the end of the span.
   */
  vtkIdType SpanEndId()
  {
    return this->SpanEnd;
  }

  /**
   * Get a void pointer and pixel increment for the given point Id.
   * The pixel increment is the number of scalar components.
   */
  static void *GetVoidPointer(vtkImageData *image,
                              vtkIdType i=0,
                              int *pixelIncrement=0);

  /**
   * Get a void pointer and pixel increment for the given point Id.
   * The array must be the same size as the image.  The pixel increment
   * that is returned will be the number of components for the array.
   */
  static void *GetVoidPointer(vtkDataArray *array,
                              vtkIdType i=0,
                              int *pixelIncrement=0);

protected:

  /**
   * Set all the state variables for the stencil span that includes idX.
   */
  void SetSpanState(int idX);

  /**
   * Report the progress and do an abort check, for compatibility with
   * existing image filters.  If an algorithm was provided to the constructor,
   * then this is called every time that one row of the image is completed.
   */
  void ReportProgress();

  vtkIdType  Id;                // the current point Id
  vtkIdType  SpanEnd;           // end of current span
  vtkIdType  RowEnd;            // end of current row
  vtkIdType  SliceEnd;          // end of current slice
  vtkIdType  End;               // end of data

  // Increments
  vtkIdType  RowIncrement;      // to same position in next row
  vtkIdType  SliceIncrement;    // to same position in next slice
  vtkIdType  RowEndIncrement;   // from end of row to start of next row
  vtkIdType  SliceEndIncrement; // from end of slice to start of next slice

  // The extent, adjusted for the stencil
  int        Extent[6];

  // Index-related items
  int        Index[3];
  int        StartY;

  // Stencil-related items
  bool       HasStencil;
  bool       InStencil;
  int        SpanSliceEndIncrement;
  int        SpanSliceIncrement;
  int        SpanIndex;
  int       *SpanCountPointer;
  int      **SpanListPointer;

  // Progress-related items
  vtkAlgorithm *Algorithm;
  vtkIdType  Count;
  vtkIdType  Target;
  int        ThreadId;
};

#endif
// VTK-HeaderTest-Exclude: vtkImagePointDataIterator.h

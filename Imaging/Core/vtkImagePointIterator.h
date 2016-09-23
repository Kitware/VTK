/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImagePointIterator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImagePointIterator
 * @brief   iterate over all data points in an image.
 *
 * This class will iterate over an image.  For each position, it provides
 * the (x,y,z) position, the (I,J,K) index, and the point Id.  If a stencil
 * is provided, then it also reports, for each point, whether the point is
 * inside the stencil.
 * <p>The iterator can go through the image point-by-point or span-by-span.
 * The Next() method advances to the next point, while the NextSpan() method
 * skips to the beginning of the next span, where a span is defined as a
 * start position and point count within an image row.
 * @sa
 * vtkImageData vtkImageStencilData vtkImageProgressIterator
*/

#ifndef vtkImagePointIterator_h
#define vtkImagePointIterator_h

#include "vtkImagePointDataIterator.h"

class VTKIMAGINGCORE_EXPORT vtkImagePointIterator :
  public vtkImagePointDataIterator
{
public:
  /**
   * Default constructor, its use must be followed by Initialize().
   */
  vtkImagePointIterator();

  /**
   * Create an iterator for the given image, with several options.
   * If a stencil is provided, then the iterator's IsInStencil() method
   * reports whether each span is inside the stencil.  If an extent is
   * provided, it iterates over the extent and ignores the rest of the
   * image (the provided extent must be within the image extent).  If
   * a pointer to the algorithm is provided, then progress events will
   * provided by the algorithm if threadId is zero.
   */
  vtkImagePointIterator(vtkImageData *image,
                        const int extent[6] = 0,
                        vtkImageStencilData *stencil=0,
                        vtkAlgorithm *algorithm=0,
                        int threadId=0);

  /**
   * Initialize an iterator.  See constructor for more details.
   */
  void Initialize(vtkImageData *image,
                  const int extent[6] = 0,
                  vtkImageStencilData *stencil=0,
                  vtkAlgorithm *algorithm=0,
                  int threadId=0);

  //@{
  /**
   * Move the iterator to the beginning of the next span.
   * A span is a contiguous region of the image over which nothing but
   * the point Id and the X index changes.
   */
  void NextSpan()
  {
    this->vtkImagePointDataIterator::NextSpan();
    this->UpdatePosition();
  }
  //@}

  //@{
  /**
   * Move to the next position (rather than directly to the next span).
   * This will automatically advance to the next span if the end of the
   * current span is reached.
   */
  void Next()
  {
    if (++(this->Id) == this->SpanEnd)
    {
      this->NextSpan();
    }
    else
    {
      this->Index[0]++;
      this->Position[0] = this->Origin[0] + this->Index[0]*this->Spacing[0];
    }
  }
  //@}

  /**
   * Test if the iterator has completed iterating over the entire extent.
   */
  bool IsAtEnd()
  {
    return this->vtkImagePointDataIterator::IsAtEnd();
  }

  /**
   * Get the current position.
   */
  double *GetPosition()
  {
    return this->Position;
  }

  //@{
  /**
   * Get the current position and place it in the provided array.
   */
  void GetPosition(double x[3])
  {
    x[0] = this->Position[0];
    x[1] = this->Position[1];
    x[2] = this->Position[2];
  }
  //@}

  //@{
  /**
   * Get the current position and place it in the provided array.
   */
  void GetPosition(float x[3])
  {
    x[0] = this->Position[0];
    x[1] = this->Position[1];
    x[2] = this->Position[2];
  }
  //@}

protected:

  //@{
  /**
   * Helper method to update the position coordinate from the index.
   */
  void UpdatePosition()
  {
    this->Position[0] = this->Origin[0] + this->Index[0]*this->Spacing[0];
    this->Position[1] = this->Origin[1] + this->Index[1]*this->Spacing[1];
    this->Position[2] = this->Origin[2] + this->Index[2]*this->Spacing[2];
  }
  //@}

  double Origin[3];
  double Spacing[3];
  double Position[3];
};

#endif
// VTK-HeaderTest-Exclude: vtkImagePointIterator.h

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageOpenClose3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageOpenClose3D
 * @brief   Will perform opening or closing.
 *
 * vtkImageOpenClose3D performs opening or closing by having two
 * vtkImageErodeDilates in series.  The size of operation
 * is determined by the method SetKernelSize, and the operator is an ellipse.
 * OpenValue and CloseValue determine how the filter behaves.  For binary
 * images Opening and closing behaves as expected.
 * Close value is first dilated, and then eroded.
 * Open value is first eroded, and then dilated.
 * Degenerate two dimensional opening/closing can be achieved by setting the
 * one axis the 3D KernelSize to 1.
 * Values other than open value and close value are not touched.
 * This enables the filter to processes segmented images containing more than
 * two tags.
*/

#ifndef vtkImageOpenClose3D_h
#define vtkImageOpenClose3D_h


#include "vtkImagingMorphologicalModule.h" // For export macro
#include "vtkImageAlgorithm.h"

class vtkImageDilateErode3D;

class VTKIMAGINGMORPHOLOGICAL_EXPORT vtkImageOpenClose3D : public vtkImageAlgorithm
{
public:
  //@{
  /**
   * Default open value is 0, and default close value is 255.
   */
  static vtkImageOpenClose3D *New();
  vtkTypeMacro(vtkImageOpenClose3D,vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  //@}

  /**
   * This method considers the sub filters MTimes when computing this objects
   * modified time.
   */
  vtkMTimeType GetMTime() VTK_OVERRIDE;

  //@{
  /**
   * Turn debugging output on. (in sub filters also)
   */
  void DebugOn() VTK_OVERRIDE;
  void DebugOff() VTK_OVERRIDE;
  //@}

  /**
   * Pass modified message to sub filters.
   */
  void Modified() VTK_OVERRIDE;

  // Forward Source messages to filter1

  /**
   * Selects the size of gaps or objects removed.
   */
  void SetKernelSize(int size0, int size1, int size2);

  //@{
  /**
   * Determines the value that will opened.
   * Open value is first eroded, and then dilated.
   */
  void SetOpenValue(double value);
  double GetOpenValue();
  //@}

  //@{
  /**
   * Determines the value that will closed.
   * Close value is first dilated, and then eroded
   */
  void SetCloseValue(double value);
  double GetCloseValue();
  //@}

  //@{
  /**
   * Needed for Progress functions
   */
  vtkGetObjectMacro(Filter0, vtkImageDilateErode3D);
  vtkGetObjectMacro(Filter1, vtkImageDilateErode3D);
  //@}

  /**
   * see vtkAlgorithm for details
   */
  int ProcessRequest(vtkInformation*,
                             vtkInformationVector**,
                             vtkInformationVector*) VTK_OVERRIDE;

  /**
   * Override to send the request to internal pipeline.
   */
  int
  ComputePipelineMTime(vtkInformation* request,
                       vtkInformationVector** inInfoVec,
                       vtkInformationVector* outInfoVec,
                       int requestFromOutputPort,
                       vtkMTimeType* mtime) VTK_OVERRIDE;

protected:
  vtkImageOpenClose3D();
  ~vtkImageOpenClose3D() VTK_OVERRIDE;

  vtkImageDilateErode3D *Filter0;
  vtkImageDilateErode3D *Filter1;

  void ReportReferences(vtkGarbageCollector*) VTK_OVERRIDE;
private:
  vtkImageOpenClose3D(const vtkImageOpenClose3D&) VTK_DELETE_FUNCTION;
  void operator=(const vtkImageOpenClose3D&) VTK_DELETE_FUNCTION;
};

#endif




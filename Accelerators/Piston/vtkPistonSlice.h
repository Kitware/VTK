/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPistonSlice.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPistonSlice
 * @brief    A filter that slices on the GPU
 *
 * This filter uses LANL's Piston library to slice on the GPU.
*/

#ifndef vtkPistonSlice_h
#define vtkPistonSlice_h

#include "vtkPistonAlgorithm.h"

class vtkPlane;

class VTKACCELERATORSPISTON_EXPORT vtkPistonSlice : public vtkPistonAlgorithm
{
public:
  vtkTypeMacro(vtkPistonSlice,vtkPistonAlgorithm);
  static vtkPistonSlice *New();
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * An offset from the plane to slice at.
   */
  vtkSetMacro(Offset, float);
  vtkGetMacro(Offset, float);
  //@}

  /**
   * Set the clipping plane.
   */
  void SetClippingPlane( vtkPlane * plane );

protected:
  vtkPistonSlice();
  ~vtkPistonSlice();

  virtual int ComputePipelineMTime(
    vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector, int requestFromOutputPort,
    vtkMTimeType* mtime);

  /**
   * Method that does the actual calculation.
   */
  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);

  float Offset;
  vtkPlane* Plane;

private:
  vtkPistonSlice(const vtkPistonSlice&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPistonSlice&) VTK_DELETE_FUNCTION;

};

#endif

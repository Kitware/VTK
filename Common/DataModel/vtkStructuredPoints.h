/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredPoints.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkStructuredPoints
 * @brief   A subclass of ImageData.
 *
 * StructuredPoints is a subclass of ImageData that requires the data extent
 * to exactly match the update extent. Normall image data allows that the
 * data extent may be larger than the update extent.
 * StructuredPoints also defines the origin differently that vtkImageData.
 * For structured points the origin is the location of first point.
 * Whereas images define the origin as the location of point 0, 0, 0.
 * Image Origin is stored in ivar, and structured points
 * have special methods for setting/getting the origin/extents.
*/

#ifndef vtkStructuredPoints_h
#define vtkStructuredPoints_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkImageData.h"


class VTKCOMMONDATAMODEL_EXPORT vtkStructuredPoints : public vtkImageData
{
public:
  static vtkStructuredPoints *New();
  vtkTypeMacro(vtkStructuredPoints,vtkImageData);

  /**
   * To simplify filter superclasses,
   */
  int GetDataObjectType() override {return VTK_STRUCTURED_POINTS;}

protected:
  vtkStructuredPoints();
  ~vtkStructuredPoints() override {}
private:
  vtkStructuredPoints(const vtkStructuredPoints&) = delete;
  void operator=(const vtkStructuredPoints&) = delete;
};

#endif



// VTK-HeaderTest-Exclude: vtkStructuredPoints.h

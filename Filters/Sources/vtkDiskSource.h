/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDiskSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkDiskSource
 * @brief   create a disk with hole in center
 *
 * vtkDiskSource creates a polygonal disk with a hole in the center. The
 * disk has zero height. The user can specify the inner and outer radius
 * of the disk, and the radial and circumferential resolution of the
 * polygonal representation.
 * @sa
 * vtkLinearExtrusionFilter
*/

#ifndef vtkDiskSource_h
#define vtkDiskSource_h

#include "vtkFiltersSourcesModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class VTKFILTERSSOURCES_EXPORT vtkDiskSource : public vtkPolyDataAlgorithm
{
public:
  static vtkDiskSource *New();
  vtkTypeMacro(vtkDiskSource,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Specify inner radius of hole in disc.
   */
  vtkSetClampMacro(InnerRadius,double,0.0,VTK_DOUBLE_MAX)
  vtkGetMacro(InnerRadius,double);
  //@}

  //@{
  /**
   * Specify outer radius of disc.
   */
  vtkSetClampMacro(OuterRadius,double,0.0,VTK_DOUBLE_MAX)
  vtkGetMacro(OuterRadius,double);
  //@}

  //@{
  /**
   * Set the number of points in radius direction.
   */
  vtkSetClampMacro(RadialResolution,int,1,VTK_INT_MAX)
  vtkGetMacro(RadialResolution,int);
  //@}

  //@{
  /**
   * Set the number of points in circumferential direction.
   */
  vtkSetClampMacro(CircumferentialResolution,int,3,VTK_INT_MAX)
  vtkGetMacro(CircumferentialResolution,int);
  //@}

  //@{
  /**
   * Set/get the desired precision for the output points.
   * vtkAlgorithm::SINGLE_PRECISION - Output single-precision floating point.
   * vtkAlgorithm::DOUBLE_PRECISION - Output double-precision floating point.
   */
  vtkSetMacro(OutputPointsPrecision,int);
  vtkGetMacro(OutputPointsPrecision,int);
  //@}

protected:
  vtkDiskSource();
  ~vtkDiskSource() override {}

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;
  double InnerRadius;
  double OuterRadius;
  int RadialResolution;
  int CircumferentialResolution;
  int OutputPointsPrecision;

private:
  vtkDiskSource(const vtkDiskSource&) = delete;
  void operator=(const vtkDiskSource&) = delete;
};

#endif

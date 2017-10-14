/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkSectorSource.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSectorSource
 * @brief   create a sector of a disk
 *
 * vtkSectorSource creates a sector of a polygonal disk. The
 * disk has zero height. The user can specify the inner and outer radius
 * of the disk, the z-coordinate, and the radial and
 * circumferential resolution of the polygonal representation.
 * @sa
 * vtkLinearExtrusionFilter
*/

#ifndef vtkSectorSource_h
#define vtkSectorSource_h

#include "vtkFiltersModelingModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class VTKFILTERSMODELING_EXPORT vtkSectorSource : public vtkPolyDataAlgorithm
{
public:
  static vtkSectorSource *New();
  vtkTypeMacro(vtkSectorSource,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Specify inner radius of the sector.
   */
  vtkSetClampMacro(InnerRadius,double,0.0,VTK_DOUBLE_MAX)
    vtkGetMacro(InnerRadius,double);
  //@}

  //@{
  /**
   * Specify outer radius of the sector.
   */
  vtkSetClampMacro(OuterRadius,double,0.0,VTK_DOUBLE_MAX)
    vtkGetMacro(OuterRadius,double);
  //@}

  //@{
  /**
   * Specify the z coordinate of the sector.
   */
  vtkSetClampMacro(ZCoord,double,0.0,VTK_DOUBLE_MAX)
    vtkGetMacro(ZCoord,double);
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
   * Set the start angle of the sector.
   */
  vtkSetClampMacro(StartAngle,double,0.0,VTK_DOUBLE_MAX)
    vtkGetMacro(StartAngle,double);
  //@}

  //@{
  /**
   * Set the end angle of the sector.
   */
  vtkSetClampMacro(EndAngle,double,0.0,VTK_DOUBLE_MAX)
    vtkGetMacro(EndAngle,double);
  //@}

protected:
  vtkSectorSource();
  ~vtkSectorSource() override {}

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;
  double InnerRadius;
  double OuterRadius;
  double ZCoord;
  int RadialResolution;
  int CircumferentialResolution;
  double StartAngle;
  double EndAngle;

private:
  vtkSectorSource(const vtkSectorSource&) = delete;
  void operator=(const vtkSectorSource&) = delete;
};

#endif

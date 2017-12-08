/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEarthSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkEarthSource
 * @brief   create the continents of the Earth as a sphere
 *
 * vtkEarthSource creates a spherical rendering of the geographical shapes
 * of the major continents of the earth. The OnRatio determines
 * how much of the data is actually used. The radius defines the radius
 * of the sphere at which the continents are placed. Obtains data from
 * an imbedded array of coordinates.
*/

#ifndef vtkEarthSource_h
#define vtkEarthSource_h

#include "vtkFiltersHybridModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class VTKFILTERSHYBRID_EXPORT vtkEarthSource : public vtkPolyDataAlgorithm
{
public:
  static vtkEarthSource *New();
  vtkTypeMacro(vtkEarthSource,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set radius of earth.
   */
  vtkSetClampMacro(Radius,double,0.0,VTK_FLOAT_MAX);
  vtkGetMacro(Radius,double);
  //@}

  //@{
  /**
   * Turn on every nth entity. This controls how much detail the model
   * will have. The maximum ratio is sixteen. (The smaller OnRatio, the more
   * detail there is.)
   */
  vtkSetClampMacro(OnRatio,int,1,16);
  vtkGetMacro(OnRatio,int);
  //@}

  //@{
  /**
   * Turn on/off drawing continents as filled polygons or as wireframe outlines.
   * Warning: some graphics systems will have trouble with the very large, concave
   * filled polygons. Recommend you use OutlienOn (i.e., disable filled polygons)
   * for now.
   */
  vtkSetMacro(Outline,vtkTypeBool);
  vtkGetMacro(Outline,vtkTypeBool);
  vtkBooleanMacro(Outline,vtkTypeBool);
  //@}

protected:
  vtkEarthSource();
  ~vtkEarthSource() override {}

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;

  double Radius;
  int OnRatio;
  vtkTypeBool Outline;
private:
  vtkEarthSource(const vtkEarthSource&) = delete;
  void operator=(const vtkEarthSource&) = delete;
};

#endif











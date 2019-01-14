/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTextureMapToCylinder.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkTextureMapToCylinder
 * @brief   generate texture coordinates by mapping points to cylinder
 *
 * vtkTextureMapToCylinder is a filter that generates 2D texture coordinates
 * by mapping input dataset points onto a cylinder. The cylinder can either be
 * user specified or generated automatically. (The cylinder is generated
 * automatically by computing the axis of the cylinder.)  Note that the
 * generated texture coordinates for the s-coordinate ranges from (0-1)
 * (corresponding to angle of 0->360 around axis), while the mapping of
 * the t-coordinate is controlled by the projection of points along the axis.
 *
 * To specify a cylinder manually, you must provide two points that
 * define the axis of the cylinder. The length of the axis will affect the
 * t-coordinates.
 *
 * A special ivar controls how the s-coordinate is generated. If PreventSeam
 * is set to true, the s-texture varies from 0->1 and then 1->0 (corresponding
 * to angles of 0->180 and 180->360).
 *
 * @warning
 * Since the resulting texture s-coordinate will lie between (0,1), and the
 * origin of the texture coordinates is not user-controllable, you may want
 * to use the class vtkTransformTexture to linearly scale and shift the origin
 * of the texture coordinates.
 *
 * @sa
 * vtkTextureMapToPlane vtkTextureMapToSphere
 * vtkTransformTexture vtkThresholdTextureCoords
*/

#ifndef vtkTextureMapToCylinder_h
#define vtkTextureMapToCylinder_h

#include "vtkFiltersTextureModule.h" // For export macro
#include "vtkDataSetAlgorithm.h"

class VTKFILTERSTEXTURE_EXPORT vtkTextureMapToCylinder : public vtkDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkTextureMapToCylinder,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Create object with cylinder axis parallel to z-axis (points (0,0,-0.5)
   * and (0,0,0.5)). The PreventSeam ivar is set to true. The cylinder is
   * automatically generated.
   */
  static vtkTextureMapToCylinder *New();

  //@{
  /**
   * Specify the first point defining the cylinder axis,
   */
  vtkSetVector3Macro(Point1,double);
  vtkGetVectorMacro(Point1,double,3);
  //@}

  //@{
  /**
   * Specify the second point defining the cylinder axis,
   */
  vtkSetVector3Macro(Point2,double);
  vtkGetVectorMacro(Point2,double,3);
  //@}

  //@{
  /**
   * Turn on/off automatic cylinder generation. This means it automatically
   * finds the cylinder center and axis.
   */
  vtkSetMacro(AutomaticCylinderGeneration,vtkTypeBool);
  vtkGetMacro(AutomaticCylinderGeneration,vtkTypeBool);
  vtkBooleanMacro(AutomaticCylinderGeneration,vtkTypeBool);
  //@}

  //@{
  /**
   * Control how the texture coordinates are generated. If PreventSeam is
   * set, the s-coordinate ranges from 0->1 and 1->0 corresponding to the
   * angle variation from 0->180 and 180->0. Otherwise, the s-coordinate
   * ranges from 0->1 from 0->360 degrees.
   */
  vtkSetMacro(PreventSeam,vtkTypeBool);
  vtkGetMacro(PreventSeam,vtkTypeBool);
  vtkBooleanMacro(PreventSeam,vtkTypeBool);
  //@}

protected:
  vtkTextureMapToCylinder();
  ~vtkTextureMapToCylinder() override {}

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;

  double Point1[3];
  double Point2[3];
  vtkTypeBool AutomaticCylinderGeneration;
  vtkTypeBool PreventSeam;

private:
  vtkTextureMapToCylinder(const vtkTextureMapToCylinder&) = delete;
  void operator=(const vtkTextureMapToCylinder&) = delete;
};

#endif



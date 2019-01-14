/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThresholdTextureCoords.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkThresholdTextureCoords
 * @brief   compute 1D, 2D, or 3D texture coordinates based on scalar threshold
 *
 * vtkThresholdTextureCoords is a filter that generates texture coordinates for
 * any input dataset type given a threshold criterion. The criterion can take
 * three forms: 1) greater than a particular value (ThresholdByUpper());
 * 2) less than a particular value (ThresholdByLower(); or 3) between two
 * values (ThresholdBetween(). If the threshold criterion is satisfied,
 * the "in" texture coordinate will be set (this can be specified by the
 * user). If the threshold criterion is not satisfied the "out" is set.
 *
 * @warning
 * There is a texture map - texThres.vtk - that can be used in conjunction
 * with this filter. This map defines a "transparent" region for texture
 * coordinates 0<=r<0.5, and an opaque full intensity map for texture
 * coordinates 0.5<r<=1.0. There is a small transition region for r=0.5.
 *
 * @sa
 * vtkThreshold vtkThresholdPoints vtkTextureMapToPlane vtkTextureMapToSphere
 * vtkTextureMapToCylinder
*/

#ifndef vtkThresholdTextureCoords_h
#define vtkThresholdTextureCoords_h

#include "vtkFiltersTextureModule.h" // For export macro
#include "vtkDataSetAlgorithm.h"

class VTKFILTERSTEXTURE_EXPORT vtkThresholdTextureCoords : public vtkDataSetAlgorithm
{
public:
  static vtkThresholdTextureCoords *New();
  vtkTypeMacro(vtkThresholdTextureCoords,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Criterion is cells whose scalars are less than lower threshold.
   */
  void ThresholdByLower(double lower);

  /**
   * Criterion is cells whose scalars are less than upper threshold.
   */
  void ThresholdByUpper(double upper);

  /**
   * Criterion is cells whose scalars are between lower and upper thresholds.
   */
  void ThresholdBetween(double lower, double upper);

  //@{
  /**
   * Return the upper and lower thresholds.
   */
  vtkGetMacro(UpperThreshold,double);
  vtkGetMacro(LowerThreshold,double);
  //@}

  //@{
  /**
   * Set the desired dimension of the texture map.
   */
  vtkSetClampMacro(TextureDimension,int,1,3);
  vtkGetMacro(TextureDimension,int);
  //@}

  //@{
  /**
   * Set the texture coordinate value for point satisfying threshold criterion.
   */
  vtkSetVector3Macro(InTextureCoord,double);
  vtkGetVectorMacro(InTextureCoord,double,3);
  //@}

  //@{
  /**
   * Set the texture coordinate value for point NOT satisfying threshold
   * criterion.
   */
  vtkSetVector3Macro(OutTextureCoord,double);
  vtkGetVectorMacro(OutTextureCoord,double,3);
  //@}

protected:
  vtkThresholdTextureCoords();
  ~vtkThresholdTextureCoords() override {}

  // Usual data generation method
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;

  double LowerThreshold;
  double UpperThreshold;

  int TextureDimension;

  double InTextureCoord[3];
  double OutTextureCoord[3];

  int (vtkThresholdTextureCoords::*ThresholdFunction)(double s);

  int Lower(double s) {return ( s <= this->LowerThreshold ? 1 : 0 );};
  int Upper(double s) {return ( s >= this->UpperThreshold ? 1 : 0 );};
  int Between(double s) {return ( s >= this->LowerThreshold ?
                               ( s <= this->UpperThreshold ? 1 : 0 ) : 0 );};
private:
  vtkThresholdTextureCoords(const vtkThresholdTextureCoords&) = delete;
  void operator=(const vtkThresholdTextureCoords&) = delete;
};

#endif

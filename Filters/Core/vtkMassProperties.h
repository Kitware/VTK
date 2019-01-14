/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMassProperties.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkMassProperties
 * @brief   estimate volume, area, shape index of triangle mesh
 *
 * vtkMassProperties estimates the volume, the surface area, and the
 * normalized shape index of a triangle mesh.  The algorithm
 * implemented here is based on the discrete form of the divergence
 * theorem.  The general assumption here is that the model is of
 * closed surface.  For more details see the following reference
 * (Alyassin A.M. et al, "Evaluation of new algorithms for the
 * interactive measurement of surface area and volume", Med Phys 21(6)
 * 1994.).
 *
 * @warning
 * Currently only triangles are processed. Use vtkTriangleFilter to convert
 * any strips or polygons to triangles. If multiple closed objects are
 * defined consider using vtkMultiObjectMassProperties. Alternatively,
 * vtkPolyDataConnectivityFilter can be used to extract connected regions
 * (i.e., objects) one at a time, and then each object can be processed by
 * this filter.
 *
 * @sa
 * vtkTriangleFilter vtkMultiObjectMassProperties
*/

#ifndef vtkMassProperties_h
#define vtkMassProperties_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class VTKFILTERSCORE_EXPORT vtkMassProperties : public vtkPolyDataAlgorithm
{
public:
  /**
   * Constructs with initial values of zero.
   */
  static vtkMassProperties *New();

  vtkTypeMacro(vtkMassProperties,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Compute and return the volume.
   */
  double GetVolume() {this->Update(); return this->Volume;}

  /**
   * Compute and return the projected volume.
   * Typically you should compare this volume to the value returned by GetVolume
   * if you get an error (GetVolume()-GetVolumeProjected())*10000 that is greater
   * than GetVolume() this should identify a problem:
   * * Either the polydata is not closed
   * * Or the polydata contains triangle that are flipped
   */
  double GetVolumeProjected() {this->Update(); return this->VolumeProjected;}

  /**
   * Compute and return the volume projected on to each axis aligned plane.
   */
  double GetVolumeX() {this->Update(); return this->VolumeX;}
  double GetVolumeY() {this->Update(); return this->VolumeY;}
  double GetVolumeZ() {this->Update(); return this->VolumeZ;}

  /**
   * Compute and return the weighting factors for the maximum unit
   * normal component (MUNC).
   */
  double GetKx() {this->Update(); return this->Kx;}
  double GetKy() {this->Update(); return this->Ky;}
  double GetKz() {this->Update(); return this->Kz;}

  /**
   * Compute and return the area.
   */
  double GetSurfaceArea() {this->Update(); return this->SurfaceArea;}

  /**
   * Compute and return the min cell area.
   */
  double GetMinCellArea() {this->Update(); return this->MinCellArea;}

  /**
   * Compute and return the max cell area.
   */
  double GetMaxCellArea() {this->Update(); return this->MaxCellArea;}

  /**
   * Compute and return the normalized shape index. This characterizes the
   * deviation of the shape of an object from a sphere. A sphere's NSI
   * is one. This number is always >= 1.0.
   */
  double GetNormalizedShapeIndex()
    {this->Update(); return this->NormalizedShapeIndex;}

protected:
  vtkMassProperties();
  ~vtkMassProperties() override;

  int RequestData(vtkInformation* request,
                  vtkInformationVector** inputVector,
                  vtkInformationVector* outputVector) override;

  double  SurfaceArea;
  double  MinCellArea;
  double  MaxCellArea;
  double  Volume;
  double  VolumeProjected; // == Projected area of triangles * average z values
  double  VolumeX;
  double  VolumeY;
  double  VolumeZ;
  double  Kx;
  double  Ky;
  double  Kz;
  double  NormalizedShapeIndex;

private:
  vtkMassProperties(const vtkMassProperties&) = delete;
  void operator=(const vtkMassProperties&) = delete;
};

#endif

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridAxisReflection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkHyperTreeGridAxisReflection
 * @brief   Reflect a hyper tree grid
 *
 *
 * This filter reflect the cells of a hyper tree grid with respect to
 * one of the planes parallel to the bounding box of the data set.
 *
 * @sa
 * vtkHyperTreeGrid vtkHyperTreeGridAlgorithm vtkReflectionFilter
 *
 * @par Thanks:
 * This class was written by Philippe Pebay based on a idea of Guenole
 * Harel and Jacques-Bernard Lekien, 2016
 * This work was supported by Commissariat a l'Energie Atomique (CEA/DIF)
*/

#ifndef vtkHyperTreeGridAxisReflection_h
#define vtkHyperTreeGridAxisReflection_h

#include "vtkFiltersHyperTreeModule.h" // For export macro
#include "vtkHyperTreeGridAlgorithm.h"

class vtkHyperTreeGrid;

class VTKFILTERSHYPERTREE_EXPORT vtkHyperTreeGridAxisReflection : public vtkHyperTreeGridAlgorithm
{
public:
  static vtkHyperTreeGridAxisReflection* New();
  vtkTypeMacro( vtkHyperTreeGridAxisReflection, vtkHyperTreeGridAlgorithm );
  void PrintSelf( ostream&, vtkIndent ) override;

  /**
   * Specify unique identifiers of available reflection planes.
   */
  enum AxisReflectionPlane
  {
    USE_X_MIN = 0,
    USE_Y_MIN = 1,
    USE_Z_MIN = 2,
    USE_X_MAX = 3,
    USE_Y_MAX = 4,
    USE_Z_MAX = 5,
    USE_X = 6,
    USE_Y = 7,
    USE_Z = 8
  };

  //@{
  /**
   * Set the normal of the plane to use as mirror.
   */
  vtkSetClampMacro(Plane, int, 0, 8);
  vtkGetMacro(Plane, int);
  void SetPlaneToX() { this->SetPlane( USE_X ); };
  void SetPlaneToY() { this->SetPlane( USE_Y ); };
  void SetPlaneToZ() { this->SetPlane( USE_Z ); };
  void SetPlaneToXMin() { this->SetPlane( USE_X_MIN ); };
  void SetPlaneToYMin() { this->SetPlane( USE_Y_MIN ); };
  void SetPlaneToZMin() { this->SetPlane( USE_Z_MIN ); };
  void SetPlaneToXMax() { this->SetPlane( USE_X_MAX ); };
  void SetPlaneToYMax() { this->SetPlane( USE_Y_MAX ); };
  void SetPlaneToZMax() { this->SetPlane( USE_Z_MAX ); };
  //@}

  //@{
  /**
   * If the reflection plane is set to X, Y or Z, this variable
   * is use to set the position of the plane.
   */
  vtkSetMacro(Center, double);
  vtkGetMacro(Center, double);
  //@}

protected:
  vtkHyperTreeGridAxisReflection();
  ~vtkHyperTreeGridAxisReflection() override;

  /**
   * For this algorithm the output is a vtkHyperTreeGrid instance
   */
  int FillOutputPortInformation( int, vtkInformation* ) override;

  /**
   * Main routine to extract cells based on reflectioned value
   */
  int ProcessTrees( vtkHyperTreeGrid*, vtkDataObject* ) override;

  /**
   * Required type of plane reflection
   */
  int Plane;

  /**
   * Position of the plane relative to given axis
   * Only used if the reflection plane is X, Y or Z
   */
  double Center;

private:
  vtkHyperTreeGridAxisReflection(const vtkHyperTreeGridAxisReflection&) = delete;
  void operator=(const vtkHyperTreeGridAxisReflection&) = delete;
};

#endif /* vtkHyperTreeGridAxisReflection */

/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkPStructuredGridConnectivity.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
// .NAME vtkPUnstructuredGridGhostDataGenerator -- Builds ghost zones for a
//  distributed unstructured grid dataset.
//
// .SECTION Description
//  This filter uses internally the vtkPUnstructuredGridConnectivity helper
//  class to construct ghost zones for a distributed unstructured grid.
//
// .SECTION Caveats
//  <ul>
//    <li> The code currently assumes one grid per rank. </li>
//    <li> GlobalID information must be provided as a PointData array
//         with the name, "GlobalID" </li>
//    <li> The grid must be globally conforming, i.e., no hanging nodes. </li>
//    <li> Only topologically face-adjacent ghost cells are considered. </li>
//    <li> PointData and CellData must match across partitions/processes. </li>
//  </ul>
//
// .SECTION See Also
//  vtkPUnstructuredGridConnectivity

#ifndef VTKPUNSTRUCTUREDGRIDGHOSTDATAGENERATOR_H_
#define VTKPUNSTRUCTUREDGRIDGHOSTDATAGENERATOR_H_

#include "vtkFiltersParallelGeometryModule.h" // For export macro
#include "vtkUnstructuredGridAlgorithm.h"

// Forward Declarations
class vtkIndent;
class vtkInformation;
class vtkInformationVector;
class vtkPUnstructuredGridConnectivity;
class vtkUnstructuredGrid;
class vtkMultiProcessController;

class VTKFILTERSPARALLELGEOMETRY_EXPORT vtkPUnstructuredGridGhostDataGenerator:
  public vtkUnstructuredGridAlgorithm
{
public:
  static vtkPUnstructuredGridGhostDataGenerator* New();
  vtkTypeMacro(vtkPUnstructuredGridGhostDataGenerator,vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkPUnstructuredGridGhostDataGenerator();
  virtual ~vtkPUnstructuredGridGhostDataGenerator();

  // Standard VTK pipeline routines
  virtual int FillInputPortInformation(int port,vtkInformation *info);
  virtual int FillOutputPortInformation(int port, vtkInformation *info);
  virtual int RequestData(
      vtkInformation *rqst, vtkInformationVector **inputVector,
      vtkInformationVector* outputVector );

  vtkPUnstructuredGridConnectivity* GhostZoneBuilder;
  vtkMultiProcessController* Controller;
private:
  vtkPUnstructuredGridGhostDataGenerator(const vtkPUnstructuredGridGhostDataGenerator&); // Not implemented
  void operator=(const vtkPUnstructuredGridGhostDataGenerator&); // Not implemented
};

#endif /* VTKPUNSTRUCTUREDGRIDGHOSTDATAGENERATOR_H_ */

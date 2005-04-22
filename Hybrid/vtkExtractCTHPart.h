/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractCTHPart.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkExtractCTHPart - Generates surface of an CTH volume fraction.
// .SECTION Description
// vtkExtractCTHPart is a filter that is specialized for creating 
// visualization of a CTH simulation.  First it converts the cell data
// to point data.  It contours the selected volume fraction at a value
// of 0.5.  The user has the option of clipping the part with a plane.
// Clipped surfaces of the part are generated.

#ifndef __vtkExtractCTHPart_h
#define __vtkExtractCTHPart_h

//#include "vtkPolyDataAlgorithm.h"
#include "vtkHierarchicalDataSetAlgorithm.h"
class vtkPlane;
class vtkDataArray;
class vtkDoubleArray;
class vtkRectilinearGrid;

class vtkExtractCTHPartInternal;
class vtkHierarchicalDataSet;
class vtkPolyData;
class vtkUniformGrid;
class vtkDataSet;

class VTK_HYBRID_EXPORT vtkExtractCTHPart : public vtkHierarchicalDataSetAlgorithm
{
public:
  vtkTypeRevisionMacro(vtkExtractCTHPart,vtkHierarchicalDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object with initial range (0,1) and single contour value
  // of 0.0.
  static vtkExtractCTHPart *New();

  // Description:
  // Names of cell volume fraction arrays to extract.
  void RemoveAllVolumeArrayNames();
  void AddVolumeArrayName(char* arrayName);
  int GetNumberOfVolumeArrayNames();
  const char* GetVolumeArrayName(int idx);

  // Description:
  // Set, get or maninpulate the implicit clipping plane.
  void SetClipPlane(vtkPlane *clipPlane);
  vtkGetObjectMacro(ClipPlane, vtkPlane);

  // Description:
  // Look at clip plane to compute MTime.
  unsigned long GetMTime();    

protected:
  vtkExtractCTHPart();
  ~vtkExtractCTHPart();

  void SetOutputData(int idx, vtkHierarchicalDataSet* d);
  
  int RequestInformation(vtkInformation *request,
                         vtkInformationVector **inputVector,
                         vtkInformationVector *outputVector);
  
  int RequestData(vtkInformation *, vtkInformationVector **,
                  vtkInformationVector *);
  
  // Description:
  // the input is a hierarchy of vtkUniformGrid or one level of
  // vtkRectilinearGrid. The output is a hierarchy of vtkPolyData.
  
  
  void ExecutePart(const char *arrayName,
                   int partIndex,
                   vtkHierarchicalDataSet *input,
                   vtkHierarchicalDataSet *output,
                   int needPartIndex);
  
  void ExecutePartOnUniformGrid(const char *arrayName,
                                vtkUniformGrid *input,
                                vtkPolyData *output);
  
  void ExecutePartOnRectilinearGrid(const char *arrayName,
                                    vtkRectilinearGrid *input,
                                    vtkPolyData *output);
  
  void ExecuteCellDataToPointData(vtkDataArray *cellVolumeFraction, 
                                  vtkDoubleArray *pointVolumeFraction,
                                  int *dims);
  
  int FillInputPortInformation(int port,
                               vtkInformation *info);
  
  vtkPlane *ClipPlane;
  vtkExtractCTHPartInternal* Internals;
private:
  vtkExtractCTHPart(const vtkExtractCTHPart&);  // Not implemented.
  void operator=(const vtkExtractCTHPart&);  // Not implemented.
};
#endif

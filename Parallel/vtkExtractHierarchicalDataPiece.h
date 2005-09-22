/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractHierarchicalDataPiece.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkExtractHierarchicalDataPiece
// .SECTION Description
// vtkExtractHierarchicalDataPiece returns the appropriate piece of each
// sub-dataset in the vtkHierarchicalDataSet by requesting it from process 0.
// This filter can handle sub-datasets of type vtkImageData, vtkPolyData,
// vtkRectilinearGrid, vtkStructuredGrid, and vtkUnstructuredGrid; it does
// not handle sub-grids of type vtkHierarchicalDataSet.

#ifndef __vtkExtractHierarchicalDataPiece_h
#define __vtkExtractHierarchicalDataPiece_h

#include "vtkHierarchicalDataSetAlgorithm.h"

class vtkImageData;
class vtkPolyData;
class vtkRectilinearGrid;
class vtkStructuredGrid;
class vtkUnstructuredGrid;

class VTK_PARALLEL_EXPORT vtkExtractHierarchicalDataPiece : public vtkHierarchicalDataSetAlgorithm
{
public:
  static vtkExtractHierarchicalDataPiece* New();
  vtkTypeRevisionMacro(vtkExtractHierarchicalDataPiece, vtkHierarchicalDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  
protected:
  vtkExtractHierarchicalDataPiece() {}
  ~vtkExtractHierarchicalDataPiece() {}

  virtual int RequestData(vtkInformation*,
                          vtkInformationVector**,
                          vtkInformationVector*);

  void ExtractImageData(vtkImageData *imageData,
                        vtkHierarchicalDataSet *output,
                        int piece, int numberOfPieces, int ghostLevel,
                        unsigned int level);
  void ExtractPolyData(vtkPolyData *polyData,
                       vtkHierarchicalDataSet *output,
                       int piece, int numberOfPieces, int ghostLevel,
                       unsigned int level);
  void ExtractRectilinearGrid(vtkRectilinearGrid *rGrid,
                              vtkHierarchicalDataSet *output,
                              int piece, int numberOfPieces, int ghostLevel,
                              unsigned int level);
  void ExtractStructuredGrid(vtkStructuredGrid *sGrid,
                             vtkHierarchicalDataSet *output,
                             int piece, int numberOfPieces, int ghostLevel,
                             unsigned int level);
  void ExtractUnstructuredGrid(vtkUnstructuredGrid *uGrid,
                               vtkHierarchicalDataSet *output,
                               int piece, int numberOfPieces, int ghostLevel,
                               unsigned int level);

private:
  vtkExtractHierarchicalDataPiece(const vtkExtractHierarchicalDataPiece&); // Not implemented.
  void operator=(const vtkExtractHierarchicalDataPiece&); // Not implemented.
};

#endif

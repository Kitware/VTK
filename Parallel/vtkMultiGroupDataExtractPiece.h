/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiGroupDataExtractPiece.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMultiGroupDataExtractPiece
// .SECTION Description
// vtkMultiGroupDataExtractPiece returns the appropriate piece of each
// sub-dataset in the vtkMultiGroupDataSet.
// This filter can handle sub-datasets of type vtkImageData, vtkPolyData,
// vtkRectilinearGrid, vtkStructuredGrid, and vtkUnstructuredGrid; it does
// not handle sub-grids of type vtkMultiGroupDataSet.

#ifndef __vtkMultiGroupDataExtractPiece_h
#define __vtkMultiGroupDataExtractPiece_h

#include "vtkMultiGroupDataSetAlgorithm.h"

class vtkImageData;
class vtkPolyData;
class vtkRectilinearGrid;
class vtkStructuredGrid;
class vtkUnstructuredGrid;

class VTK_PARALLEL_EXPORT vtkMultiGroupDataExtractPiece : public vtkMultiGroupDataSetAlgorithm
{
public:
  static vtkMultiGroupDataExtractPiece* New();
  vtkTypeRevisionMacro(vtkMultiGroupDataExtractPiece, vtkMultiGroupDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  
protected:
  vtkMultiGroupDataExtractPiece() {}
  ~vtkMultiGroupDataExtractPiece() {}

  virtual int RequestData(vtkInformation*,
                          vtkInformationVector**,
                          vtkInformationVector*);

  void ExtractImageData(vtkImageData *imageData,
                        vtkMultiGroupDataSet *output,
                        int piece, int numberOfPieces, int ghostLevel,
                        unsigned int group);
  void ExtractPolyData(vtkPolyData *polyData,
                       vtkMultiGroupDataSet *output,
                       int piece, int numberOfPieces, int ghostLevel,
                       unsigned int group);
  void ExtractRectilinearGrid(vtkRectilinearGrid *rGrid,
                              vtkMultiGroupDataSet *output,
                              int piece, int numberOfPieces, int ghostLevel,
                              unsigned int group);
  void ExtractStructuredGrid(vtkStructuredGrid *sGrid,
                             vtkMultiGroupDataSet *output,
                             int piece, int numberOfPieces, int ghostLevel,
                             unsigned int group);
  void ExtractUnstructuredGrid(vtkUnstructuredGrid *uGrid,
                               vtkMultiGroupDataSet *output,
                               int piece, int numberOfPieces, int ghostLevel,
                               unsigned int group);

private:
  vtkMultiGroupDataExtractPiece(const vtkMultiGroupDataExtractPiece&); // Not implemented.
  void operator=(const vtkMultiGroupDataExtractPiece&); // Not implemented.
};

#endif

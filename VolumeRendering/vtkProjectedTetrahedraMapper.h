/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProjectedTetrahedraMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
 * Copyright 2003 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

// .NAME vtkProjectedTetrahedraMapper - Unstructured grid volume renderer.
//
// .SECTION Description
// vtkProjectedTetrahedraMapper is an implementation of the classic
// Projected Tetrahedra algorithm presented by Shirley and Tuchman in "A
// Polygonal Approximation to Direct Scalar Volume Rendering" in Computer
// Graphics, December 1990.
//
// .SECTION Bugs
// This mapper relies highly on the implementation of the OpenGL pipeline.
// A typically hardware driver has lots of options and some settings can
// cause this mapper to produce artifacts.
//

#ifndef __vtkProjectedTetrahedraMapper_h
#define __vtkProjectedTetrahedraMapper_h

#include "vtkUnstructuredGridVolumeMapper.h"

class vtkVisibilitySort;
class vtkUnsignedCharArray;
class vtkFloatArray;

class VTK_VOLUMERENDERING_EXPORT vtkProjectedTetrahedraMapper : public vtkUnstructuredGridVolumeMapper
{
public:
  vtkTypeRevisionMacro(vtkProjectedTetrahedraMapper,
                       vtkUnstructuredGridVolumeMapper);
  static vtkProjectedTetrahedraMapper *New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  virtual void SetVisibilitySort(vtkVisibilitySort *sort);
  vtkGetObjectMacro(VisibilitySort, vtkVisibilitySort);

  // Description:
  // Control how the filter works with scalar point data and cell attribute
  // data.  By default (ScalarModeToDefault), the filter will use point data,
  // and if no point data is available, then cell data is used. Alternatively
  // you can explicitly set the filter to use point data
  // (ScalarModeToUsePointData) or cell data (ScalarModeToUseCellData).
  // You can also choose to get the scalars from an array in point field
  // data (ScalarModeToUsePointFieldData) or cell field data
  // (ScalarModeToUseCellFieldData).  If scalars are coming from a field
  // data array, you must call SelectColorArray before you call
  // GetColors.
  vtkSetMacro(ScalarMode,int);
  vtkGetMacro(ScalarMode,int);
  void SetScalarModeToDefault() {
    this->SetScalarMode(VTK_SCALAR_MODE_DEFAULT);};
  void SetScalarModeToUsePointData() {
    this->SetScalarMode(VTK_SCALAR_MODE_USE_POINT_DATA);};
  void SetScalarModeToUseCellData() {
    this->SetScalarMode(VTK_SCALAR_MODE_USE_CELL_DATA);};
  void SetScalarModeToUsePointFieldData() {
    this->SetScalarMode(VTK_SCALAR_MODE_USE_POINT_FIELD_DATA);};
  void SetScalarModeToUseCellFieldData() {
    this->SetScalarMode(VTK_SCALAR_MODE_USE_CELL_FIELD_DATA);};
  
  // Description:
  // When ScalarMode is set to UsePointFileData or UseCellFieldData,
  // you can specify which array to use for coloring using these methods.
  // The transfer function in the vtkVolumeProperty (attached to the calling
  // vtkVolume) will decide how to convert vectors to colors.
  virtual void SelectScalarArray(int arrayNum); 
  virtual void SelectScalarArray(const char* arrayName); 
  
  // Description:
  // Get the array name or number and component to color by.
  virtual char* GetArrayName() { return this->ArrayName; }
  virtual int GetArrayId() { return this->ArrayId; }
  virtual int GetArrayAccessMode() { return this->ArrayAccessMode; }

  // Description:
  // Return the method for obtaining scalar data.
  const char *GetScalarModeAsString();

  virtual void Render(vtkRenderer *renderer, vtkVolume *volume);

  virtual void ReleaseGraphicsResources(vtkWindow *window);

  static void MapScalarsToColors(vtkDataArray *colors, vtkVolume *volume,
                                 vtkDataArray *scalars);

protected:
  vtkProjectedTetrahedraMapper();
  ~vtkProjectedTetrahedraMapper();

  vtkUnsignedCharArray *Colors;
  int UsingCellColors;

  vtkFloatArray *TransformedPoints;

  float MaxCellSize;
  vtkTimeStamp InputAnalyzedTime;
  vtkTimeStamp OpacityTextureTime;
  vtkTimeStamp ColorsMappedTime;

  unsigned int OpacityTexture;

  vtkVisibilitySort *VisibilitySort;

  int   ScalarMode;
  char *ArrayName;
  int   ArrayId;
  int   ArrayAccessMode;

  vtkVolume *LastVolume;

  virtual void ProjectTetrahedra(vtkRenderer *renderer, vtkVolume *volume);

  // Description:
  // The visibility sort will probably make a reference loop by holding a
  // reference to the input.
  virtual void ReportReferences(vtkGarbageCollector *collector);

private:
  vtkProjectedTetrahedraMapper(const vtkProjectedTetrahedraMapper &);  // Not Implemented.
  void operator=(const vtkProjectedTetrahedraMapper &);  // Not Implemented.
};

#endif //__vtkProjectedTetrahedraMapper_h

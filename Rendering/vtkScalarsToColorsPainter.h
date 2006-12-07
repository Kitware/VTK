/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkScalarsToColorsPainter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkScalarsToColorsPainter - painter that converts scalars to 
// colors. It enable/disables coloring state depending on the ScalarMode.
// .SECTION Description
// This is a painter that converts scalars to 
// colors. It enable/disables coloring state depending on the ScalarMode.

#ifndef __vtkScalarsToColorsPainter_h
#define __vtkScalarsToColorsPainter_h

#include "vtkPolyDataPainter.h"

class vtkDataArray;
class vtkImageData;
class vtkInformationDoubleVectorKey;
class vtkInformationIntegerKey;
class vtkInformationObjectBaseKey;
class vtkInformationStringKey;
class vtkPolyData;
class vtkScalarsToColors;

class VTK_RENDERING_EXPORT vtkScalarsToColorsPainter : 
  public vtkPolyDataPainter
{
public:
  static vtkScalarsToColorsPainter* New();
  vtkTypeRevisionMacro(vtkScalarsToColorsPainter, vtkPolyDataPainter);
  void PrintSelf(ostream& os, vtkIndent);

  // Description:
  // Control whether the mapper sets the lookuptable range based on its
  // own ScalarRange, or whether it will use the LookupTable ScalarRange
  // regardless of it's own setting. By default the Mapper is allowed to set
  // the LookupTable range, but users who are sharing LookupTables between
  // mappers/actors will probably wish to force the mapper to use the
  // LookupTable unchanged.
  static vtkInformationIntegerKey* USE_LOOKUP_TABLE_SCALAR_RANGE();

  // Description:
  // Specify range in terms of scalar minimum and maximum (smin,smax). These
  // values are used to map scalars into lookup table. Has no effect when
  // UseLookupTableScalarRange is true.
  static vtkInformationDoubleVectorKey* SCALAR_RANGE();

  // Description:
  // Control how the painter works with scalar point data and cell attribute
  // data. See vtkMapper::ScalarMode for more details.
  static vtkInformationIntegerKey* SCALAR_MODE();

  // Description:
  // Control how the scalar data is mapped to colors. By default 
  // (ColorModeToDefault), unsigned char scalars are treated as colors, 
  // and NOT mapped through the lookup table, while everything else is. 
  // Setting ColorModeToMapScalars means that all scalar data will be mapped 
  // through the lookup table.
  static vtkInformationIntegerKey* COLOR_MODE();
  
  // Description:
  // By default, vertex color is used to map colors to a surface.
  // Colors are interpolated after being mapped.
  // This option avoids color interpolation by using a one dimensional
  // texture map for the colors.
  static vtkInformationIntegerKey* INTERPOLATE_SCALARS_BEFORE_MAPPING();
  
  // Description:
  // Specify a lookup table for the mapper to use.
  static vtkInformationObjectBaseKey* LOOKUP_TABLE();
  void SetLookupTable(vtkScalarsToColors *lut);
  vtkScalarsToColors *GetLookupTable();
  
  // Description:
  // Create default lookup table. Generally used to create one when none
  // is available with the scalar data.
  virtual void CreateDefaultLookupTable();
  
  // Description:
  // Turn on/off flag to control whether scalar data is used to color objects.
  static vtkInformationIntegerKey* SCALAR_VISIBILITY();

  // Description:
  // Controls what data array is used to generate colors.
  static vtkInformationIntegerKey* ARRAY_ACCESS_MODE();
  static vtkInformationIntegerKey* ARRAY_ID();
  static vtkInformationStringKey* ARRAY_NAME();
  static vtkInformationIntegerKey* ARRAY_COMPONENT();
  
  // Description:
  // Set the light-model color mode. 
  static vtkInformationIntegerKey* SCALAR_MATERIAL_MODE();

protected:
  vtkScalarsToColorsPainter();
  virtual ~vtkScalarsToColorsPainter(); 
 
  void MapScalarsToTexture(vtkDataArray* scalars, double alpha);

  // Description:
  // Called just before RenderInternal(). We build the Color array here.
  virtual void PrepareForRendering(vtkRenderer* renderer, vtkActor* actor);

  // Description:
  // Generates the colors, if needed.
  virtual void MapScalars(double alpha);

  // Description:
  // Called before RenderInternal() if the Information has been changed
  // since the last time this method was called.
  virtual void ProcessInformation(vtkInformation*);

  // Description:
  // Subclasses need to override this to return the output of the pipeline.
  virtual vtkPolyData *GetOutputData() { return this->OutputData; }

  // Description:
  // Take part in garbage collection.
  virtual void ReportReferences(vtkGarbageCollector *collector);
  
  // Methods to set the ivars. These are purposefully protected.
  // The only means of affecting these should be using teh vtkInformation 
  // object.
  vtkSetMacro(UseLookupTableScalarRange,int);
  vtkSetVector2Macro(ScalarRange,double);
  vtkSetMacro(ScalarMode, int);
  vtkSetMacro(ColorMode, int);
  vtkSetMacro(InterpolateScalarsBeforeMapping,int);
  vtkSetMacro(ScalarVisibility,int);
  vtkSetMacro(ScalarMaterialMode,int);
  vtkSetMacro(ArrayAccessMode, int);
  vtkSetMacro(ArrayId, int);
  vtkSetStringMacro(ArrayName);
  vtkSetMacro(ArrayComponent, int);

  vtkPolyData* OutputData;

  int ArrayAccessMode;
  int ArrayComponent;
  int ArrayId;
  char* ArrayName;
  
  vtkScalarsToColors *LookupTable;
  vtkImageData* ColorTextureMap;
  int ColorMode;
  int InterpolateScalarsBeforeMapping;
  int ScalarMode;
  int ScalarMaterialMode;
  double ScalarRange[2];
  int ScalarVisibility;
  int UseLookupTableScalarRange;

  vtkTimeStamp OutputUpdateTime;
  
private:
  vtkScalarsToColorsPainter(const vtkScalarsToColorsPainter&); // Not implemented.
  void operator=(const vtkScalarsToColorsPainter&); // Not implemented.
};

#endif


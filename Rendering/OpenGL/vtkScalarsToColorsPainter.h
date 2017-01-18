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
/**
 * @class   vtkScalarsToColorsPainter
 * @brief   painter that converts scalars to
 * colors. It enable/disables coloring state depending on the ScalarMode.
 *
 * This is a painter that converts scalars to
 * colors. It enable/disables coloring state depending on the ScalarMode.
 * This painter is composite dataset enabled.
*/

#ifndef vtkScalarsToColorsPainter_h
#define vtkScalarsToColorsPainter_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkPainter.h"
#include "vtkSmartPointer.h" // needed for vtkSmartPointer.
class vtkDataArray;
class vtkImageData;
class vtkInformationDoubleVectorKey;
class vtkInformationIntegerKey;
class vtkInformationObjectBaseKey;
class vtkInformationStringKey;
class vtkDataSet;
class vtkScalarsToColors;

class VTKRENDERINGOPENGL_EXPORT vtkScalarsToColorsPainter : public vtkPainter
{
public:
  static vtkScalarsToColorsPainter* New();
  vtkTypeMacro(vtkScalarsToColorsPainter, vtkPainter);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Control whether the mapper sets the lookuptable range based on its
   * own ScalarRange, or whether it will use the LookupTable ScalarRange
   * regardless of it's own setting. By default the Mapper is allowed to set
   * the LookupTable range, but users who are sharing LookupTables between
   * mappers/actors will probably wish to force the mapper to use the
   * LookupTable unchanged.
   */
  static vtkInformationIntegerKey* USE_LOOKUP_TABLE_SCALAR_RANGE();

  /**
   * Specify range in terms of scalar minimum and maximum (smin,smax). These
   * values are used to map scalars into lookup table. Has no effect when
   * UseLookupTableScalarRange is true.
   */
  static vtkInformationDoubleVectorKey* SCALAR_RANGE();

  /**
   * Control how the painter works with scalar point data and cell attribute
   * data. See vtkMapper::ScalarMode for more details.
   */
  static vtkInformationIntegerKey* SCALAR_MODE();

  /**
   * Control how the scalar data is mapped to colors. By default
   * (ColorModeToDefault), unsigned char scalars are treated as colors,
   * and NOT mapped through the lookup table, while everything else is.
   * Setting ColorModeToMapScalars means that all scalar data will be mapped
   * through the lookup table.
   */
  static vtkInformationIntegerKey* COLOR_MODE();

  /**
   * By default, vertex color is used to map colors to a surface.
   * Colors are interpolated after being mapped.
   * This option avoids color interpolation by using a one dimensional
   * texture map for the colors.
   */
  static vtkInformationIntegerKey* INTERPOLATE_SCALARS_BEFORE_MAPPING();

  //@{
  /**
   * Specify a lookup table for the mapper to use.
   */
  static vtkInformationObjectBaseKey* LOOKUP_TABLE();
  void SetLookupTable(vtkScalarsToColors *lut);
  vtkScalarsToColors *GetLookupTable();
  //@}

  /**
   * Create default lookup table. Generally used to create one when none
   * is available with the scalar data.
   */
  virtual void CreateDefaultLookupTable();

  /**
   * Turn on/off flag to control whether scalar data is used to color objects.
   */
  static vtkInformationIntegerKey* SCALAR_VISIBILITY();

  //@{
  /**
   * Controls what data array is used to generate colors.
   */
  static vtkInformationIntegerKey* ARRAY_ACCESS_MODE();
  static vtkInformationIntegerKey* ARRAY_ID();
  static vtkInformationStringKey*  ARRAY_NAME();
  static vtkInformationIntegerKey* ARRAY_COMPONENT();
  static vtkInformationIntegerKey* FIELD_DATA_TUPLE_ID();
  //@}

  /**
   * Set the light-model color mode.
   */
  static vtkInformationIntegerKey* SCALAR_MATERIAL_MODE();

  /**
   * For alpha blending, we sometime premultiply the colors
   * with alpha and change the alpha blending function.
   * This call returns whether we are premultiplying or using
   * the default blending function.
   * Currently this checks if the actor has a texture, if not
   * it returns true.
   * TODO: It is possible to make this decision
   * dependent on key passed down from a painter upstream
   * that makes a more informed decision for alpha blending
   * depending on extensions available, for example.
   */
  virtual int GetPremultiplyColorsWithAlpha(vtkActor* actor);

  /**
   * Subclasses need to override this to return the output of the pipeline.
   */
  vtkDataObject *GetOutput() VTK_OVERRIDE;

  /**
   * Return the texture size limit. Subclasses need to override this
   * to return the actual correct texture size limit.  Here it is
   * hardcoded to 1024.
   */
  virtual vtkIdType GetTextureSizeLimit();

protected:
  vtkScalarsToColorsPainter();
  ~vtkScalarsToColorsPainter() VTK_OVERRIDE;

  /**
   * Create a new shallow-copied clone for data with no scalars.
   */
  virtual vtkDataObject* NewClone(vtkDataObject* data);

  /**
   * Create texture coordinates for the output assuming a texture for the
   * lookuptable has already been created correctly.
   * this->LookupTable is the lookuptable used.
   */
  void MapScalarsToTexture(vtkDataSet* output,
    vtkDataArray* scalars, vtkDataSet* input);

  /**
   * Called just before RenderInternal(). We build the Color array here.
   */
  void PrepareForRendering(vtkRenderer* renderer, vtkActor* actor) VTK_OVERRIDE;

  /**
   * Generates the colors, if needed.
   * If multiply_with_alpha is set, the colors are multiplied with
   * alpha.
   */
  virtual void MapScalars(vtkDataSet* output,
    double alpha, int multiply_with_alpha,
    vtkDataSet* input);

  /**
   * Called before RenderInternal() if the Information has been changed
   * since the last time this method was called.
   */
  void ProcessInformation(vtkInformation*) VTK_OVERRIDE;

  /**
   * Take part in garbage collection.
   */
  void ReportReferences(vtkGarbageCollector *collector) VTK_OVERRIDE;

  /**
   * Returns if we can use texture maps for scalar coloring. Note this doesn't
   * say we "will" use scalar coloring. It says, if we do use scalar coloring,
   * we will use a 1D texture.
   * When rendering multiblock datasets, if any 2 blocks provide different
   * lookup tables for the scalars, then also we cannot use textures. This case
   * can be handled if required.
   */
  int CanUseTextureMapForColoring(vtkDataObject* input);


  /**
   * Should not be called if CanUseTextureMapForColoring() returns 0.
   */
  void UpdateColorTextureMap(double alpha, int multiply_with_alpha);

  // Methods to set the ivars. These are purposefully protected.
  // The only means of affecting these should be using the vtkInformation
  // object.
  vtkSetMacro(UseLookupTableScalarRange, int);
  vtkSetVector2Macro(ScalarRange, double);
  vtkSetMacro(ScalarMode, int);
  vtkSetMacro(ColorMode, int);
  vtkSetMacro(InterpolateScalarsBeforeMapping, int);
  vtkSetMacro(ScalarVisibility, int);
  vtkSetMacro(ScalarMaterialMode, int);
  vtkSetMacro(ArrayAccessMode, int);
  vtkSetMacro(ArrayComponent, int);
  vtkSetMacro(ArrayId, int);
  vtkSetStringMacro(ArrayName);
  vtkSetMacro(FieldDataTupleId, vtkIdType);

  vtkDataObject* OutputData;

  int ArrayAccessMode;
  int ArrayComponent;
  int ArrayId;
  char* ArrayName;
  vtkIdType FieldDataTupleId;

  vtkScalarsToColors *LookupTable;
  // Lookup table provided via the scalars. This gets preference over the one
  // set on the mapper by the user.
  vtkSmartPointer<vtkScalarsToColors> ScalarsLookupTable;
  vtkSmartPointer<vtkImageData> ColorTextureMap;
  int ColorMode;
  int InterpolateScalarsBeforeMapping;
  int ScalarMode;
  int ScalarMaterialMode;
  double LastUsedAlpha; // Essential to ensure alpha changes work correctly
                        // for composite datasets.
  int LastUsedMultiplyWithAlpha;
  double ScalarRange[2];
  int ScalarVisibility;
  int UseLookupTableScalarRange;

  vtkTimeStamp OutputUpdateTime;

  // This is set when MapScalars decides to use vertex colors for atleast on
  // dataset in the current pass.
  int UsingScalarColoring;

private:
  vtkScalarsToColorsPainter(const vtkScalarsToColorsPainter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkScalarsToColorsPainter&) VTK_DELETE_FUNCTION;

};

#endif

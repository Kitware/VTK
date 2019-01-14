/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLookupTableWithEnabling.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkLookupTableWithEnabling
 * @brief   A lookup table that allows for an
 * optional array to be provided that specifies which scalars to "enable" and
 * which to "disable".
 *
 *
 * vtkLookupTableWithEnabling "disables" or "grays out" output colors
 * based on whether the given value in EnabledArray is "0" or not.
 *
 *
 * @warning
 * You must set the EnabledArray before MapScalars() is called.
 * Indices of EnabledArray must map directly to those of the array passed
 * to MapScalars().
 *
*/

#ifndef vtkLookupTableWithEnabling_h
#define vtkLookupTableWithEnabling_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkLookupTable.h"

class vtkDataArray;

class VTKRENDERINGCORE_EXPORT vtkLookupTableWithEnabling : public vtkLookupTable
{
public:
  static vtkLookupTableWithEnabling *New();

  vtkTypeMacro(vtkLookupTableWithEnabling,vtkLookupTable);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * This must be set before MapScalars() is called.
   * Indices of this array must map directly to those in the scalars array
   * passed to MapScalars().
   * Values of 0 in the array indicate the color should be desaturatated.
   */
  vtkGetObjectMacro(EnabledArray,vtkDataArray);
  virtual void SetEnabledArray(vtkDataArray *enabledArray);
  //@}

  /**
   * Map a set of scalars through the lookup table.
   */
  void MapScalarsThroughTable2(void *input, unsigned char *output,
                               int inputDataType, int numberOfValues,
                               int inputIncrement, int outputIncrement) override;

  /**
   * A convenience method for taking a color and desaturating it.
   */
  virtual void DisableColor(unsigned char r, unsigned char g, unsigned char b,
                   unsigned char *rd, unsigned char *gd, unsigned char *bd);

protected:
  vtkLookupTableWithEnabling(int sze=256, int ext=256);
  ~vtkLookupTableWithEnabling() override;

  vtkDataArray *EnabledArray;

private:
  vtkLookupTableWithEnabling(const vtkLookupTableWithEnabling&) = delete;
  void operator=(const vtkLookupTableWithEnabling&) = delete;
};


#endif




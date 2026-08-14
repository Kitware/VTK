// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkLookupTableManager
 * @brief   Keeps one lookup table per array name.
 *
 * vtkLookupTableManager is a registry of vtkScalarsToColors objects keyed by
 * array name.  Asking for the table of an array that has none creates one from
 * a vtkColorSeries color scheme, so every representation coloring by
 * "Temperature" ends up sharing a single table, and with it a single range and
 * a single set of colors.
 *
 * A manager can be shared: set the same one on several views to keep their
 * color maps in sync.  A view that is not given one creates its own on first
 * use, which keeps views independent by default.
 *
 * The manager does not own the range of any table.  Ranges belong to the
 * tables themselves and are set by whoever knows the data.
 *
 * @sa vtkScivisView vtkColorSeries vtkLookupTable
 */

#ifndef vtkLookupTableManager_h
#define vtkLookupTableManager_h

#include "vtkObject.h"
#include "vtkViewsScivisModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkLookupTable;
class vtkScalarsToColors;

class VTKVIEWSSCIVIS_EXPORT vtkLookupTableManager : public vtkObject
{
public:
  static vtkLookupTableManager* New();
  vtkTypeMacro(vtkLookupTableManager, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * The tables handed out here stay reachable and mutable by their users, so
   * the modified time reported is the latest of this object's own and those of
   * the tables it holds.
   */
  vtkMTimeType GetMTime() override;

  /**
   * Get the lookup table for @a arrayName, creating one from
   * DefaultColorScheme if this is the first time the array is asked for.
   * Returns the same table on every subsequent call, and null for a null or
   * empty name.
   */
  vtkScalarsToColors* GetLookupTable(const char* arrayName);

  /**
   * Whether @a arrayName already has a lookup table.  Unlike GetLookupTable(),
   * this does not create one.
   */
  bool HasLookupTable(const char* arrayName);

  /**
   * Use @a lut for @a arrayName in place of whatever table the array had.
   * Passing null removes the entry, the same as RemoveLookupTable().
   */
  void SetLookupTable(const char* arrayName, vtkScalarsToColors* lut);

  /**
   * Forget the lookup table of @a arrayName.  The next request for that array
   * builds a fresh table.
   */
  void RemoveLookupTable(const char* arrayName);

  /**
   * Forget every lookup table.
   */
  void RemoveAllLookupTables();

  ///@{
  /**
   * The tables currently held, in name order.  GetLookupTableName() returns
   * null for an index outside [0, GetNumberOfLookupTables()).
   */
  int GetNumberOfLookupTables();
  const char* GetLookupTableName(int index);
  ///@}

  ///@{
  /**
   * The vtkColorSeries::ColorSchemes scheme used to build new lookup tables.
   * The scheme's colors are spread evenly over the table and interpolated, so
   * a scheme of a dozen swatches still yields a smooth ramp.  Defaults to
   * vtkColorSeries::BREWER_DIVERGING_SPECTRAL_11.
   *
   * Changing this affects tables built afterwards; tables already handed out
   * keep the colors they were built with.
   */
  vtkSetMacro(DefaultColorScheme, int);
  vtkGetMacro(DefaultColorScheme, int);
  ///@}

  ///@{
  /**
   * How many entries new lookup tables are given.  Default is 256.  Like
   * DefaultColorScheme, this applies to tables built afterwards.
   */
  vtkSetMacro(NumberOfTableValues, int);
  vtkGetMacro(NumberOfTableValues, int);
  ///@}

protected:
  vtkLookupTableManager();
  ~vtkLookupTableManager() override;

  /**
   * Fill @a lut with NumberOfTableValues entries interpolated across the
   * colors of DefaultColorScheme.
   */
  void BuildDefaultLookupTable(vtkLookupTable* lut);

private:
  vtkLookupTableManager(const vtkLookupTableManager&) = delete;
  void operator=(const vtkLookupTableManager&) = delete;

  class vtkInternals;
  vtkInternals* Internals;

  int DefaultColorScheme;
  int NumberOfTableValues;
};

VTK_ABI_NAMESPACE_END
#endif

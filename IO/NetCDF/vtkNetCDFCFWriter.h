// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkNetCDFCFWriter_h
#define vtkNetCDFCFWriter_h

#include "vtkIONetCDFModule.h" // For export macro
#include "vtkWriter.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkIdList;
class vtkDataSet;
class vtkImageData;

/**
 * @class   vtkNetCDFCFWriter
 *
 * Writes netCDF files that follow the CF convention.  Details on this convention
 * can be found at <http://cfconventions.org/>
 */
class VTKIONETCDF_EXPORT vtkNetCDFCFWriter : public vtkWriter
{
public:
  static vtkNetCDFCFWriter* New();
  vtkTypeMacro(vtkNetCDFCFWriter, vtkWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the file name of the file.
   */
  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);
  ///@}

  ///@{
  /**
   * VTK allows point and cell arrays with the same name, but NetCDF does not.
   * This string is appended to a cell array name if it conflicts with a point
   * array name when it is saved in a NetCDF file. Default is _c.
   *
   */
  vtkSetStringMacro(CellArrayNamePostfix);
  vtkGetStringMacro(CellArrayNamePostfix);
  ///@}

  ///@{
  /**
   * Get/Set the FillValue for all array. Care must be taken to make sure
   * the value fits in the value type of each array.
   * Fill value has the same meaning as blanking in VTK but it is stored in
   * the data array. This is stored in the NetCDF file.
   */
  vtkSetMacro(FillValue, int);
  vtkGetMacro(FillValue, int);
  ///@}

  ///@{
  /**
   * Only arrays of this attribute type are saved in the file. (vtkDataObject::POINT or CELL).
   * Saving only one type of arrays avoids issues with conflicting array names
   * between points and cells. Default is vtkDataObject::POINT
   */
  vtkSetMacro(AttributeType, int);
  vtkGetMacro(AttributeType, int);
  ///@}

  ///@{
  /**
   * If true, before writing to the file it fills all blanked cells and points in
   * the attribute arrays with the fill value for the type. Default is false.
   */
  vtkSetMacro(FillBlankedAttributes, bool);
  vtkGetMacro(FillBlankedAttributes, bool);
  vtkBooleanMacro(FillBlankedAttributes, bool);
  ///@}

  ///@{
  /**
   * Add/clear attributes that define the grid mapping (or the coordinate
   * reference system (CRS))
   *
   * \verbatim
   * To obtain the correct CF conventions attribute names and values
   * when knowing the EPSG code use projinfo <epsg_code> This will
   * print the WKT string. From that you can get the attribute names
   * and values you need for CF convention.. The WKT attribute names
   * are fairly close to CF convention attribute names.  The following
   * link also helps with the conversion.
   * <a
   * href="https://github.com/cf-convention/cf-conventions/wiki/Mapping-from-CF-Grid-Mapping-Attributes-to-CRS-WKT-Elements">CF
   * Grid Mapping to WKT</a> See also <a
   * href="http://cfconventions.org/Data/cf-conventions/cf-conventions-1.9/cf-conventions.html#appendix-grid-mappings">CF
   * Grid Mapping</a> for the attributes needed for each projection.
   * \endverbatim
   */
  void AddGridMappingAttribute(const char* name, const char* value);
  void AddGridMappingAttribute(const char* name, double value);
  void ClearGridMappingAttributes();
  ///@}

protected:
  vtkNetCDFCFWriter();
  ~vtkNetCDFCFWriter() override;
  void WriteData() override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  char* FileName;
  char* CellArrayNamePostfix;
  bool FillBlankedAttributes;
  int FillValue;
  int AttributeType;
  class Implementation;
  Implementation* Impl;

private:
  vtkNetCDFCFWriter(const vtkNetCDFCFWriter&) = delete;
  void operator=(const vtkNetCDFCFWriter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif

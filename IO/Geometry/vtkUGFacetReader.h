/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUGFacetReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkUGFacetReader
 * @brief   read EDS Unigraphics facet files
 *
 * vtkUGFacetReader is a source object that reads Unigraphics facet files.
 * Unigraphics is a solid modeling system; facet files are the polygonal
 * plot files it uses to create 3D plots.
*/

#ifndef vtkUGFacetReader_h
#define vtkUGFacetReader_h

#include "vtkIOGeometryModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkIncrementalPointLocator;
class vtkShortArray;

class VTKIOGEOMETRY_EXPORT vtkUGFacetReader : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkUGFacetReader,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Construct object to extract all parts, and with point merging
   * turned on.
   */
  static vtkUGFacetReader *New();

  /**
   * Overload standard modified time function. If locator is modified,
   * then this object is modified as well.
   */
  vtkMTimeType GetMTime();

  //@{
  /**
   * Specify Unigraphics file name.
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

  /**
   * Special methods for interrogating the data file.
   */
  int GetNumberOfParts();

  /**
   * Retrieve color index for the parts in the file.
   */
  short GetPartColorIndex(int partId);

  //@{
  /**
   * Specify the desired part to extract. The part number must range between
   * [0,NumberOfParts-1]. If the value is =(-1), then all parts will be
   * extracted. If the value is <(-1), then no parts will be  extracted but
   * the part colors will be updated.
   */
  vtkSetMacro(PartNumber,int);
  vtkGetMacro(PartNumber,int);
  //@}

  //@{
  /**
   * Turn on/off merging of points/triangles.
   */
  vtkSetMacro(Merging,int);
  vtkGetMacro(Merging,int);
  vtkBooleanMacro(Merging,int);
  //@}

  //@{
  /**
   * Specify a spatial locator for merging points. By
   * default an instance of vtkMergePoints is used.
   */
  void SetLocator(vtkIncrementalPointLocator *locator);
  vtkGetObjectMacro(Locator,vtkIncrementalPointLocator);
  //@}

  /**
   * Create default locator. Used to create one when none is specified.
   */
  void CreateDefaultLocator();

protected:
  vtkUGFacetReader();
  ~vtkUGFacetReader();

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  char *FileName;
  vtkShortArray *PartColors;
  int PartNumber;
  int Merging;
  vtkIncrementalPointLocator *Locator;
private:
  vtkUGFacetReader(const vtkUGFacetReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkUGFacetReader&) VTK_DELETE_FUNCTION;
};

#endif

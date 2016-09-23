/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSTLReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSTLReader
 * @brief   read ASCII or binary stereo lithography files
 *
 * vtkSTLReader is a source object that reads ASCII or binary stereo
 * lithography files (.stl files). The FileName must be specified to
 * vtkSTLReader. The object automatically detects whether the file is
 * ASCII or binary.
 *
 * .stl files are quite inefficient since they duplicate vertex
 * definitions. By setting the Merging boolean you can control whether the
 * point data is merged after reading. Merging is performed by default,
 * however, merging requires a large amount of temporary storage since a
 * 3D hash table must be constructed.
 *
 * @warning
 * Binary files written on one system may not be readable on other systems.
 * vtkSTLWriter uses VAX or PC byte ordering and swaps bytes on other systems.
*/

#ifndef vtkSTLReader_h
#define vtkSTLReader_h

#include "vtkIOGeometryModule.h" // For export macro
#include "vtkAbstractPolyDataReader.h"

class vtkCellArray;
class vtkFloatArray;
class vtkIncrementalPointLocator;
class vtkPoints;

class VTKIOGEOMETRY_EXPORT vtkSTLReader : public vtkAbstractPolyDataReader
{
public:
  vtkTypeMacro(vtkSTLReader,vtkAbstractPolyDataReader);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Construct object with merging set to true.
   */
  static vtkSTLReader *New();

  /**
   * Overload standard modified time function. If locator is modified,
   * then this object is modified as well.
   */
  vtkMTimeType GetMTime();

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
   * Turn on/off tagging of solids with scalars.
   */
  vtkSetMacro(ScalarTags,int);
  vtkGetMacro(ScalarTags,int);
  vtkBooleanMacro(ScalarTags,int);
  //@}

  //@{
  /**
   * Specify a spatial locator for merging points. By
   * default an instance of vtkMergePoints is used.
   */
  void SetLocator(vtkIncrementalPointLocator *locator);
  vtkGetObjectMacro(Locator,vtkIncrementalPointLocator);
  //@}

protected:
  vtkSTLReader();
  ~vtkSTLReader();

  /**
   * Create default locator. Used to create one when none is specified.
   */
  vtkIncrementalPointLocator* NewDefaultLocator();

  int Merging;
  int ScalarTags;
  vtkIncrementalPointLocator *Locator;

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  bool ReadBinarySTL(FILE *fp, vtkPoints*, vtkCellArray*);
  bool ReadASCIISTL(FILE *fp, vtkPoints*, vtkCellArray*,
                    vtkFloatArray* scalars=0);
  int GetSTLFileType(const char *filename);
private:
  vtkSTLReader(const vtkSTLReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSTLReader&) VTK_DELETE_FUNCTION;
};

#endif

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMCubesReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkMCubesReader
 * @brief   read binary marching cubes file
 *
 * vtkMCubesReader is a source object that reads binary marching cubes
 * files. (Marching cubes is an isosurfacing technique that generates
 * many triangles.) The binary format is supported by W. Lorensen's
 * marching cubes program (and the vtkSliceCubes object). The format
 * repeats point coordinates, so this object will merge the points
 * with a vtkLocator object. You can choose to supply the vtkLocator
 * or use the default.
 *
 * @warning
 * Binary files assumed written in sun/hp/sgi (i.e., Big Endian) form.
 *
 * @warning
 * Because points are merged when read, degenerate triangles may be removed.
 * Thus the number of triangles read may be fewer than the number of triangles
 * actually created.
 *
 * @warning
 * The point merging does not take into account that the same point may have
 * different normals. For example, running vtkPolyDataNormals after
 * vtkContourFilter may split triangles because of the FeatureAngle
 * ivar. Subsequent reading with vtkMCubesReader will merge the points and
 * use the first point's normal. For the most part, this is undesirable.
 *
 * @warning
 * Normals are generated from the gradient of the data scalar values. Hence
 * the normals may on occasion point in a direction inconsistent with the
 * ordering of the triangle vertices. If this happens, the resulting surface
 * may be "black".  Reverse the sense of the FlipNormals boolean flag to
 * correct this.
 *
 * @sa
 * vtkContourFilter vtkMarchingCubes vtkSliceCubes vtkLocator
*/

#ifndef vtkMCubesReader_h
#define vtkMCubesReader_h

#include "vtkIOGeometryModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

#define VTK_FILE_BYTE_ORDER_BIG_ENDIAN 0
#define VTK_FILE_BYTE_ORDER_LITTLE_ENDIAN 1

class vtkIncrementalPointLocator;

class VTKIOGEOMETRY_EXPORT vtkMCubesReader : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkMCubesReader,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct object with FlipNormals turned off and Normals set to true.
   */
  static vtkMCubesReader *New();

  //@{
  /**
   * Specify file name of marching cubes file.
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Set / get the file name of the marching cubes limits file.
   */
  vtkSetStringMacro(LimitsFileName);
  vtkGetStringMacro(LimitsFileName);
  //@}

  //@{
  /**
   * Specify a header size if one exists. The header is skipped and not used at this time.
   */
  vtkSetClampMacro(HeaderSize,int,0,VTK_INT_MAX);
  vtkGetMacro(HeaderSize,int);
  //@}

  //@{
  /**
   * Specify whether to flip normals in opposite direction. Flipping ONLY
   * changes the direction of the normal vector. Contrast this with flipping
   * in vtkPolyDataNormals which flips both the normal and the cell point
   * order.
   */
  vtkSetMacro(FlipNormals,vtkTypeBool);
  vtkGetMacro(FlipNormals,vtkTypeBool);
  vtkBooleanMacro(FlipNormals,vtkTypeBool);
  //@}

  //@{
  /**
   * Specify whether to read normals.
   */
  vtkSetMacro(Normals,vtkTypeBool);
  vtkGetMacro(Normals,vtkTypeBool);
  vtkBooleanMacro(Normals,vtkTypeBool);
  //@}

  //@{
  /**
   * These methods should be used instead of the SwapBytes methods.
   * They indicate the byte ordering of the file you are trying
   * to read in. These methods will then either swap or not swap
   * the bytes depending on the byte ordering of the machine it is
   * being run on. For example, reading in a BigEndian file on a
   * BigEndian machine will result in no swapping. Trying to read
   * the same file on a LittleEndian machine will result in swapping.
   * As a quick note most UNIX machines are BigEndian while PC's
   * and VAX tend to be LittleEndian. So if the file you are reading
   * in was generated on a VAX or PC, SetDataByteOrderToLittleEndian otherwise
   * SetDataByteOrderToBigEndian.
   */
  void SetDataByteOrderToBigEndian();
  void SetDataByteOrderToLittleEndian();
  int GetDataByteOrder();
  void SetDataByteOrder(int);
  const char *GetDataByteOrderAsString();
  //@}

  //@{
  /**
   * Turn on/off byte swapping.
   */
  vtkSetMacro(SwapBytes,vtkTypeBool);
  vtkGetMacro(SwapBytes,vtkTypeBool);
  vtkBooleanMacro(SwapBytes,vtkTypeBool);
  //@}

  //@{
  /**
   * Set / get a spatial locator for merging points. By default,
   * an instance of vtkMergePoints is used.
   */
  void SetLocator(vtkIncrementalPointLocator *locator);
  vtkGetObjectMacro(Locator,vtkIncrementalPointLocator);
  //@}

  /**
   * Create default locator. Used to create one when none is specified.
   */
  void CreateDefaultLocator();

  /**
   * Return the mtime also considering the locator.
   */
  vtkMTimeType GetMTime() override;

protected:
  vtkMCubesReader();
  ~vtkMCubesReader() override;

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;

  char *FileName;
  char *LimitsFileName;
  vtkIncrementalPointLocator *Locator;
  vtkTypeBool SwapBytes;
  int HeaderSize;
  vtkTypeBool FlipNormals;
  vtkTypeBool Normals;

private:
  vtkMCubesReader(const vtkMCubesReader&) = delete;
  void operator=(const vtkMCubesReader&) = delete;
};

#endif

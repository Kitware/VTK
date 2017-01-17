/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSliceCubes.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSliceCubes
 * @brief   generate isosurface(s) from volume four slices at a time
 *
 * vtkSliceCubes is a special version of the marching cubes filter. Instead
 * of ingesting an entire volume at once it processes only four slices at
 * a time. This way, it can generate isosurfaces from huge volumes. Also, the
 * output of this object is written to a marching cubes triangle file. That
 * way, output triangles do not need to be held in memory.
 *
 * To use vtkSliceCubes you must specify an instance of vtkVolumeReader to
 * read the data. Set this object up with the proper file prefix, image range,
 * data origin, data dimensions, header size, data mask, and swap bytes flag.
 * The vtkSliceCubes object will then take over and read slices as necessary.
 * You also will need to specify the name of an output marching cubes triangle
 * file.
 *
 * @warning
 * This process object is both a source and mapper (i.e., it reads and writes
 * data to a file). This is different than the other marching cubes objects
 * (and most process objects in the system). It's specialized to handle very
 * large data.
 *
 * @warning
 * This object only extracts a single isosurface. This compares with the other
 * contouring objects in vtk that generate multiple surfaces.
 *
 * @warning
 * To read the output file use vtkMCubesReader.
 *
 * @sa
 * vtkMarchingCubes vtkContourFilter vtkMCubesReader vtkDividingCubes vtkVolumeReader
*/

#ifndef vtkSliceCubes_h
#define vtkSliceCubes_h

#include "vtkImagingHybridModule.h" // For export macro
#include "vtkObject.h"

class vtkVolumeReader;

class VTKIMAGINGHYBRID_EXPORT vtkSliceCubes : public vtkObject
{
public:
  static vtkSliceCubes *New();
  vtkTypeMacro(vtkSliceCubes,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  // methods to make it look like a filter
  void Write() {this->Update();};
  void Update();

  //@{
  /**
   * Set/get object to read slices.
   */
  virtual void SetReader(vtkVolumeReader*);
  vtkGetObjectMacro(Reader,vtkVolumeReader);
  //@}

  //@{
  /**
   * Specify file name of marching cubes output file.
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Set/get isosurface contour value.
   */
  vtkSetMacro(Value,double);
  vtkGetMacro(Value,double);
  //@}

  //@{
  /**
   * Specify file name of marching cubes limits file. The limits file
   * speeds up subsequent reading of output triangle file.
   */
  vtkSetStringMacro(LimitsFileName);
  vtkGetStringMacro(LimitsFileName);
  //@}

protected:
  vtkSliceCubes();
  ~vtkSliceCubes() VTK_OVERRIDE;

  void Execute();

  vtkVolumeReader *Reader;
  char *FileName;
  double Value;
  char *LimitsFileName;

private:
  vtkSliceCubes(const vtkSliceCubes&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSliceCubes&) VTK_DELETE_FUNCTION;
};

#endif

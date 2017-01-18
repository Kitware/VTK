/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPNMReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPNMReader
 * @brief   read pnm (i.e., portable anymap) files
 *
 *
 * vtkPNMReader is a source object that reads pnm (portable anymap) files.
 * This includes .pbm (bitmap), .pgm (grayscale), and .ppm (pixmap) files.
 * (Currently this object only reads binary versions of these files.)
 *
 * PNMReader creates structured point datasets. The dimension of the
 * dataset depends upon the number of files read. Reading a single file
 * results in a 2D image, while reading more than one file results in a
 * 3D volume.
 *
 * To read a volume, files must be of the form "FileName.<number>" (e.g.,
 * foo.ppm.0, foo.ppm.1, ...). You must also specify the DataExtent.  The
 * fifth and sixth values of the DataExtent specify the beginning and ending
 * files to read.
*/

#ifndef vtkPNMReader_h
#define vtkPNMReader_h

#include "vtkIOImageModule.h" // For export macro
#include "vtkImageReader.h"

class VTKIOIMAGE_EXPORT vtkPNMReader : public vtkImageReader
{
public:
  static vtkPNMReader *New();
  vtkTypeMacro(vtkPNMReader,vtkImageReader);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  int CanReadFile(const char* fname) VTK_OVERRIDE;
  /**
   * .pnm .pgm .ppm
   */
  const char* GetFileExtensions() VTK_OVERRIDE
  {
      return ".pnm .pgm .ppm";
  }

  /**
   * PNM
   */
  const char* GetDescriptiveName() VTK_OVERRIDE
  {
      return "PNM";
  }

protected:
  vtkPNMReader() {}
  ~vtkPNMReader() VTK_OVERRIDE {}
  void ExecuteInformation() VTK_OVERRIDE;
private:
  vtkPNMReader(const vtkPNMReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPNMReader&) VTK_DELETE_FUNCTION;
};

#endif



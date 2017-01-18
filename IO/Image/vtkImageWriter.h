/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageWriter
 * @brief   Writes images to files.
 *
 * vtkImageWriter writes images to files with any data type. The data type of
 * the file is the same scalar type as the input.  The dimensionality
 * determines whether the data will be written in one or multiple files.
 * This class is used as the superclass of most image writing classes
 * such as vtkBMPWriter etc. It supports streaming.
*/

#ifndef vtkImageWriter_h
#define vtkImageWriter_h

#include "vtkIOImageModule.h" // For export macro
#include "vtkImageAlgorithm.h"

class VTKIOIMAGE_EXPORT vtkImageWriter : public vtkImageAlgorithm
{
public:
  static vtkImageWriter *New();
  vtkTypeMacro(vtkImageWriter,vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Specify file name for the image file. You should specify either
   * a FileName or a FilePrefix. Use FilePrefix if the data is stored
   * in multiple files.
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Specify file prefix for the image file(s).You should specify either
   * a FileName or FilePrefix. Use FilePrefix if the data is stored
   * in multiple files.
   */
  vtkSetStringMacro(FilePrefix);
  vtkGetStringMacro(FilePrefix);
  //@}

  //@{
  /**
   * The sprintf format used to build filename from FilePrefix and number.
   */
  vtkSetStringMacro(FilePattern);
  vtkGetStringMacro(FilePattern);
  //@}

  //@{
  /**
   * What dimension are the files to be written. Usually this is 2, or 3.
   * If it is 2 and the input is a volume then the volume will be
   * written as a series of 2d slices.
   */
  vtkSetMacro(FileDimensionality, int);
  vtkGetMacro(FileDimensionality, int);
  //@}

  /**
   * Set/Get the input object from the image pipeline.
   */
  vtkImageData *GetInput();

  /**
   * The main interface which triggers the writer to start.
   */
  virtual void Write();

  void DeleteFiles();

protected:
  vtkImageWriter();
  ~vtkImageWriter() VTK_OVERRIDE;

  int FileDimensionality;
  char *FilePrefix;
  char *FilePattern;
  char *FileName;
  int FileNumber;
  int FileLowerLeft;
  char *InternalFileName;
  // Required for subclasses that need to prevent the writer
  // from touching the file system. The getter/setter are only
  // available in these subclasses.
  unsigned int WriteToMemory;

  virtual void RecursiveWrite(int dim,
                              vtkImageData *region,
                              vtkInformation*inInfo,
                              ofstream *file);
  virtual void RecursiveWrite(int dim,
                              vtkImageData *cache,
                              vtkImageData *data,
                              vtkInformation* inInfo,
                              ofstream *file);
  virtual void WriteFile(ofstream *file, vtkImageData *data,
                         int extent[6], int wExtent[6]);
  virtual void WriteFileHeader(ofstream *, vtkImageData *, int [6]) {}
  virtual void WriteFileTrailer(ofstream *, vtkImageData *) {}

  // This is called by the superclass.
  // This is the method you should override.
  int RequestData(vtkInformation *request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector) VTK_OVERRIDE;

  int MinimumFileNumber;
  int MaximumFileNumber;
  int FilesDeleted;

private:
  vtkImageWriter(const vtkImageWriter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkImageWriter&) VTK_DELETE_FUNCTION;
};

#endif

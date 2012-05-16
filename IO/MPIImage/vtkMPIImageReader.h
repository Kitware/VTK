// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMPIImageReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

// .NAME vtkMPIImageReader Superclass of parallel binary image file readers.
//
// .SECTION Description
//
// vtkMPIImageReader provides the mechanism to read a brick of bytes (or shorts,
// or ints, or floats, or doubles, ...) from a file or series of files.  You can
// use it to read raw image data from files.  You may also be able to subclass
// this to read simple file formats.
//
// What distinguishes this class from vtkImageReader and vtkImageReader2 is that
// it performs synchronized parallel I/O using the MPIIO layer.  This can make a
// huge difference in file read times, especially when reading in parallel from
// a parallel file system.
//
// Despite the name of this class, vtkMPIImageReader will work even if MPI is
// not available.  If MPI is not available or MPIIO is not available or the
// given Controller is not a vtkMPIController (or NULL), then this class will
// silently work exactly like its superclass.  The point is that you can safely
// use this class in applications that may or may not be compiled with MPI (or
// may or may not actually be run with MPI).
//
// .SECTION See Also
// vtkMultiProcessController, vtkImageReader, vtkImageReader2
//

#ifndef __vtkMPIImageReader_h
#define __vtkMPIImageReader_h

#include "vtkIOMPIImageModule.h" // For export macro
#include "vtkImageReader.h"

class vtkMPIOpaqueFileHandle;
class vtkMultiProcessController;

class VTKIOMPIIMAGE_EXPORT vtkMPIImageReader : public vtkImageReader
{
public:
  vtkTypeMacro(vtkMPIImageReader, vtkImageReader);
  static vtkMPIImageReader *New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Get/set the multi process controller to use for coordinated reads.  By
  // default, set to the global controller.
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  virtual void SetController(vtkMultiProcessController *);

protected:
  vtkMPIImageReader();
  ~vtkMPIImageReader();

  vtkMultiProcessController *Controller;

  // Description:
  // Returns the size, in bytes of the scalar data type (GetDataScalarType).
  int GetDataScalarTypeSize();

  // Description:
  // Break up the controller based on the files each process reads.  Each group
  // comprises the processes that read the same files in the same order.
  // this->GroupedController is set to the group for the current process.
  virtual void PartitionController(const int extent[6]);

  // Description:
  // Get the header size of the given open file.  This should be used in liu of
  // the GetHeaderSize methods of the superclass.
  virtual unsigned long GetHeaderSize(vtkMPIOpaqueFileHandle &file);

  // Description:
  // Set up a "view" on the open file that will allow you to read the 2D or 3D
  // subarray from the file in one read.  Once you call this method, the file
  // will look as if it contains only the data the local process needs to read
  // in.
  virtual void SetupFileView(vtkMPIOpaqueFileHandle &file, const int extent[6]);

  // Description:
  // Given a slice of the data, open the appropriate file, read the data into
  // given buffer, and close the file.  For three dimensional data, always
  // use "slice" 0.  Make sure the GroupedController is properly created before
  // calling this using the PartitionController method.
  virtual void ReadSlice(int slice, const int extent[6], void *buffer);

  // Description:
  // Transform the data from the order read from a file to the order to place
  // in the output data (as defined by the transform).
  virtual void TransformData(vtkImageData *data);

  // Description:
  // A group of processes that are reading the same file (as determined by
  // PartitionController.
  void SetGroupedController(vtkMultiProcessController *);
  vtkMultiProcessController *GroupedController;

  virtual void ExecuteDataWithInformation(vtkDataObject *data,
                                          vtkInformation *outInfo);

private:
  vtkMPIImageReader(const vtkMPIImageReader &); // Not implemented
  void operator=(const vtkMPIImageReader &);    // Not implemented
};

#endif //__vtkMPIImageReader_h

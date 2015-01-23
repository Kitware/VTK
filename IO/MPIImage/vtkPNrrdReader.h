// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPNrrdReader.h

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

// .NAME vtkPNrrdReader - Read nrrd files efficiently from parallel file systems (and reasonably well elsewhere).
//
// .SECTION Description
//
// vtkPNrrdReader is a subclass of vtkNrrdReader that will read Nrrd format
// header information of the image before reading the data.  This means that the
// reader will automatically set information like file dimensions.
//
// .SECTION Bugs
//
// There are several limitations on what type of nrrd files we can read.  This
// reader only supports nrrd files in raw format.  Other encodings like ascii
// and hex will result in errors.  When reading in detached headers, this only
// supports reading one file that is detached.
//

#ifndef vtkPNrrdReader_h
#define vtkPNrrdReader_h

#include "vtkIOMPIImageModule.h" // For export macro
#include "vtkNrrdReader.h"

class vtkCharArray;
class vtkMultiProcessController;
class vtkMPIOpaqueFileHandle;

class VTKIOMPIIMAGE_EXPORT vtkPNrrdReader : public vtkNrrdReader
{
public:
  vtkTypeMacro(vtkPNrrdReader, vtkNrrdReader);
  static vtkPNrrdReader *New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Get/set the multi process controller to use for coordinated reads.  By
  // default, set to the global controller.
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  virtual void SetController(vtkMultiProcessController *);

protected:
  vtkPNrrdReader();
  ~vtkPNrrdReader();

  virtual int ReadHeader();
  virtual int ReadHeader(vtkCharArray *headerBuffer);

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

  vtkMultiProcessController *Controller;

private:
  vtkPNrrdReader(const vtkPNrrdReader &);       // Not implemented.
  void operator=(const vtkPNrrdReader &);        // Not implemented.
};

#endif //vtkPNrrdReader_h

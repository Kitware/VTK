/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEnSightGoldBinaryReader.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkEnSightGoldBinaryReader - class to read binary EnSight Gold files
// .SECTION Description
// vtkEnSightGoldBinaryReader is a class to read EnSight Gold files into vtk.
// Because the different parts of the EnSight data can be of various data
// types, this reader produces multiple outputs, one per part in the input
// file.
// All variable information is being stored in field data.  The descriptions
// listed in the case file are used as the array names in the field data.
// For complex vector variables, the description is appended with _r (for the
// array of real values) and _i (for the array if imaginary values).  Complex
// scalar variables are stored as a single array with 2 components, real and
// imaginary, listed in that order.
// .SECTION Caveats
// You must manually call Update on this reader and then connect the rest
// of the pipeline because (due to the nature of the file format) it is
// not possible to know ahead of time how many outputs you will have or
// what types they will be.
// This reader can only handle static EnSight datasets (both static geometry
// and variables).

#ifndef __vtkEnSightGoldBinaryReader_h
#define __vtkEnSightGoldBinaryReader_h

#include "vtkEnSightReader.h"

class VTK_IO_EXPORT vtkEnSightGoldBinaryReader : public vtkEnSightReader
{
public:
  static vtkEnSightGoldBinaryReader *New();
  vtkTypeMacro(vtkEnSightGoldBinaryReader, vtkEnSightReader);
  
protected:
  vtkEnSightGoldBinaryReader();
  ~vtkEnSightGoldBinaryReader();
  vtkEnSightGoldBinaryReader(const vtkEnSightGoldBinaryReader&);
  void operator=(const vtkEnSightGoldBinaryReader&);
  
  // Description:
  // Read the geometry file.  If an error occurred, 0 is returned; otherwise 1.
  virtual int ReadGeometryFile(char* fileName, int timeStep);

  // Description:
  // Read the measured geometry file.  If an error occurred, 0 is returned;
  // otherwise 1.
  virtual int ReadMeasuredGeometryFile(char* fileName, int timeStep);

  // Description:
  // Read scalars per node for this dataset.  If an error occurred, 0 is
  // returned; otherwise 1.  If there will be more than one component in
  // the data array, it is assumed that 0 is the first component added.
  virtual int ReadScalarsPerNode(char* fileName, char* description,
				 int timeStep, int measured = 0,
				 int numberOfComponents = 1,
                                 int component = 0);
  
  // Description:
  // Read vectors per node for this dataset.  If an error occurred, 0 is
  // returned; otherwise 1.
  virtual int ReadVectorsPerNode(char* fileName, char* description,
				 int timeStep, int measured = 0);

  // Description:
  // Read tensors per node for this dataset.  If an error occurred, 0 is
  // returned; otherwise 1.
  virtual int ReadTensorsPerNode(char* fileName, char* description,
				 int timeStep);

  // Description:
  // Read scalars per element for this dataset.  If an error occurred, 0 is
  // returned; otherwise 1.  If there will be more than one componenet in the
  // data array, it is assumed that 0 is the first component added.
  virtual int ReadScalarsPerElement(char* fileName, char* description,
				    int timeStep, int numberOfComponents = 1,
                                    int component = 0);

  // Description:
  // Read vectors per element for this dataset.  If an error occurred, 0 is
  // returned; otherwise 1.
  virtual int ReadVectorsPerElement(char* fileName, char* description,
				    int timeStep);

  // Description:
  // Read tensors per element for this dataset.  If an error occurred, 0 is
  // returned; otherwise 1.
  virtual int ReadTensorsPerElement(char* fileName, char* description,
				    int timeStep);

  // Description:
  // Read an unstructured part (partId) from the geometry file and create a
  // vtkUnstructuredGrid output.  Return 0 if EOF reached. Return -1 if
  // an error occurred.
  virtual int CreateUnstructuredGridOutput(int partId, char line[80]);
  
  // Description:
  // Read a structured part from the geometry file and create a
  // vtkStructuredGrid output.  Return 0 if EOF reached.
  virtual int CreateStructuredGridOutput(int partId, char line[256]);
  
  // Description:
  // Read a structured part from the geometry file and create a
  // vtkRectilinearGrid output.  Return 0 if EOF reached.
  int CreateRectilinearGridOutput(int partId, char line[256]);
  
  // Description:
  // Read a structured part from the geometry file and create a
  // vtkImageData output.  Return 0 if EOF reached.
  int CreateImageDataOutput(int partId, char line[80]);
  
  // Description:
  // Internal function to read in a line up to 80 characters.
  // Returns zero if there was an error.
  int ReadLine(char result[80]);

  // Description:
  // Internal function to read in a single integer.
  // Returns zero if there was an error.
  int ReadInt(int *result);

  // Description:
  // Internal function to read in an integer array.
  // Returns zero if there was an error.
  int ReadIntArray(int *result, int numInts);

  // Description:
  // Internal function to read in a float array.
  // Returns zero if there was an error.
  int ReadFloatArray(float *result, int numFloats);

  // Description:
  // Read to the next time step in the geometry file.
  void SkipTimeStep();
  int SkipStructuredGrid(char line[256]);
  int SkipUnstructuredGrid(char line[256]);
  int SkipRectilinearGrid(char line[256]);
  int SkipImageData(char line[256]);
  
  int NodeIdsListed;
  int ElementIdsListed;
  
  FILE *IFile;
};

#endif

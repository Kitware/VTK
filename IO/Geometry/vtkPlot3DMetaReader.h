/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkPlot3DMetaReader.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPlot3DMetaReader - reads meta-files points to PLOT3D files
// .SECTION Description:
// The main goal of this reader is to make it easy to read PLOT3D files,
// specifically time series of PLOT3D files. PLOT3D files can take many
// different forms based on their content. Unfortunately, it is not a
// self-describing format therefore the user needs to pass information
// about the contents of the file to the reader. Normally, this is done
// by setting a number of member variables. The goal of this reader is
// to provide a simple format that enable the writer of the PLOT3D file
// to describe its settings as well as group a number of files as a time
// series. Note that for binary files, the auto-detect-format option,
// which is on by default negates the need to specify most other option.
// However, this reader is still very useful when trying to read file
// series even for binary files. The format for this meta-file is very simple
// and is based on JSON (there is no need to know anything about JSON to
// understand this format). Below is an example with comments (followed by //)
// that describe the format. Note that the PLOT3D file names are relative
// to the location of the meta-file unless they start with a leading /.
//
// \verbatim
// {
// "auto-detect-format" : true // Tells the reader to try to figure out the format automatically. Only works
//                             // with binary file. This is on by default, negating the need for most other
//                             // options for binary files (format, byte-order, precision, multi-grid,
//                             // blanking, 2D).
// "format" : "binary",  // Is this a binary or ascii file, values : binary, ascii
// "byte-order" : "big", // Byte order for binary files, values : little, big (denoting little or big endian)
// "precision" : 32,     // Precision of floating point values, can be 32 or 64 (bits)
// "multi-grid" : false, // Is this a multi-grid file, values: true, false
// "language" : "C",     // Which language was this file written in, values : C, fortran. This is
//                       // used to determine if an binary PLOT3D file contains byte counts, used by
//                       // Fortran IO routines.
// "blanking" : false,   // Does this file have blanking information (iblanks), values : true, false
// "2D" : false,         // Is this a 2D dataset, values : true, false
// "R" : 8.314,          // The value of the gas constant, default is 1.0. Set this according to the dimensions you use
// "gamma" : 1.4,        // Ratio of specific heats. Default is 1.4.
// "functions": [ 110, 200, 201 ],  // Additional derived values to calculate. This is an array of integers formatted
//                                  // as [ value, value, value, ...]
// "filenames" : [     // List of xyz (geometry) and q (value) file names along with the time values.
//                     // This is an array which contains items in the format:
//                     // {"time" : values, "xyz" : "xyz file name", "q" : "q file name", "function" : "function file name"}
//                     // Note that q and function are optional. Also, you can repeat the same file name for xyz or q
//                     // if they don't change over time. The reader will not read files unnecessarily.
//  { "time" : 3.5, "xyz" : "combxyz.bin", "q" : "combq.1.bin", "function" : "combf.1.bin" },
//  { "time" : 4.5, "xyz" : "combxyz.bin", "q" : "combq.2.bin", "function" : "combf.2.bin" }
// ]
// }
// \endverbatim
//
// This reader leverages vtkMultiBlockPLOT3DReader to do the actual
// reading so you may want to refer to the documenation of
// vtkMultiBlockPLOT3DReader about the details of some of these
// parameters including the function numbers for derived value
// calculation.
//
// .SECTION See Also
// vtkMultiBlockPLOT3DReader

#ifndef vtkPlot3DMetaReader_h
#define vtkPlot3DMetaReader_h

#include "vtkIOGeometryModule.h" // For export macro
#include "vtkMultiBlockDataSetAlgorithm.h"

//BTX
struct vtkPlot3DMetaReaderInternals;
//ETX

class vtkMultiBlockPLOT3DReader;

namespace Json
{
  class Value;
}

class VTKIOGEOMETRY_EXPORT vtkPlot3DMetaReader : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkPlot3DMetaReader* New();
  vtkTypeMacro(vtkPlot3DMetaReader, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the meta PLOT3D filename. See the class documentation for
  // format details.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

protected:
  vtkPlot3DMetaReader();
  ~vtkPlot3DMetaReader();

  virtual int RequestInformation(vtkInformation* request,
                                 vtkInformationVector** inputVector,
                                 vtkInformationVector* outputVector);
  virtual int RequestData(vtkInformation*,
                          vtkInformationVector**,
                          vtkInformationVector*);


  char* FileName;

  void SetAutoDetectFormat(Json::Value* value);
  void SetByteOrder(Json::Value* value);
  void SetPrecision(Json::Value* val);
  void SetMultiGrid(Json::Value* val);
  void SetFormat(Json::Value* val);
  void SetBlanking(Json::Value* val);
  void Set2D(Json::Value* val);
  void SetR(Json::Value* val);
  void SetGamma(Json::Value* val);
  void SetFileNames(Json::Value* val);
  void SetLanguage(Json::Value* val);
  void AddFunctions(Json::Value* val);

private:
  vtkPlot3DMetaReader(const vtkPlot3DMetaReader&); // Not implemented.
  void operator=(const vtkPlot3DMetaReader&); // Not implemented.

  vtkMultiBlockPLOT3DReader* Reader;
  vtkPlot3DMetaReaderInternals* Internal;
};

#endif

// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOCCTPDCReader
 * @brief   VTK Reader for STEP and IGES files using OpenCASCADE that produces
 * vtkPartionedDataSetCollections
 *
 * This reader is based on OpenCASCADE and use XCAF toolkits (TKXDESTEP and TKXDEIGES)
 * if available to read the names and the colors. If not available, TKSTEP and TKIGES are
 * used but no names or colors are read.
 *
 * The quality of the generated mesh is configured using RelativeDeflection, LinearDeflection,
 * and AngularDeflection.
 *
 * Reading 1D cells (wires) is optional.
 *
 * The reader will only create nodes in the resulting vtkDataAssembly iff the corresponding
 * OpenCascade assembly label has a name.  If it does not then all of the label's descendants will
 * be added to its corresponding parent in the assembly.
 */

#ifndef vtkOCCTPDCReader_h
#define vtkOCCTPDCReader_h

#include <vtkPartitionedDataSetCollectionAlgorithm.h>
#include <vtkResourceStream.h> // For providing stream support
#include <vtkSmartPointer.h>   // For holding onto a provided stream

#include "vtkIOOCCTModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKIOOCCT_EXPORT vtkOCCTPDCReader : public vtkPartitionedDataSetCollectionAlgorithm
{
public:
  static vtkOCCTPDCReader* New();
  vtkTypeMacro(vtkOCCTPDCReader, vtkPartitionedDataSetCollectionAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum Format : unsigned int
  {
    UNSUPPORTED, // This can result when trying to process an unsupported file or stream
    AUTO,
    STEP,
    IGES,
    BREP,
    XBF
  };

  ///@{
  /**
   * Set the file format to read.
   * Allowed values are AUTO, STEP, IGES, BREP, and XBF
   *
   * AUTO - have the reader automatically determine the format of the file or stream.
   * In the case of files, it will look at the file extension.  In the case of a stream,
   * it will look at its contents.
   *
   * Default is AUTO
   */
  vtkSetClampMacro(FileFormat, unsigned int, Format::AUTO, Format::XBF);
  ///@}
  ///@{
  /**
   * Get/Set the file name.
   */
  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);
  ///@}

  ///@{
  /**
   * Get/Set a resource stream to be used.
   * NOTE: Stream support for IGES is not currently supported
   */
  vtkSetSmartPointerMacro(Stream, vtkResourceStream);
  vtkGetSmartPointerMacro(Stream, vtkResourceStream);
  ///@}

  ///@{
  /**
   * Set/Get the linear deflection.
   * This value limits the distance between a curve and the resulting tessellation.
   * Default is 0.1
   */
  vtkSetMacro(LinearDeflection, double);
  vtkGetMacro(LinearDeflection, double);

  ///@{
  /**
   * Set/Get relative deflection.
   * Determine if the deflection values are relative to object size.
   * Default is false
   */
  vtkGetMacro(RelativeDeflection, bool);
  vtkSetMacro(RelativeDeflection, bool);
  vtkBooleanMacro(RelativeDeflection, bool);
  ///@}

  ///@{
  /**
   * Set/Get redundant node mapping control.
   * Indicates if  a 2tuple int array called **NodeMapping** should be
   * included as field data on the partitionedDataSetCollection being
   * returned.
   *
   * If created, each tuple represents a node id whose name needed to be changed to be unique
   * and the id of the sibling node that has the original name.
   *
   * *Note:* This array will not be created if all nodes have been uniquely named.
   * Default is false
   */
  vtkGetMacro(CreateRedundantMap, bool);
  vtkSetMacro(CreateRedundantMap, bool);
  vtkBooleanMacro(CreateRedundantMap, bool);
  ///@}

  ///@{
  /**
   * Set/Get the angular deflection.
   * This value limits the angle between two subsequent segments.
   * Default is 0.5
   */
  vtkGetMacro(AngularDeflection, double);
  vtkSetMacro(AngularDeflection, double);
  ///@}

  ///@{
  /**
   * Enable/Disable 1D cells read. If enabled, surface boundaries are read.
   * Default is false
   */
  vtkGetMacro(ReadWire, bool);
  vtkSetMacro(ReadWire, bool);
  vtkBooleanMacro(ReadWire, bool);
  ///@}
  ///@{
  /**
   * Get/Set the name of the assembly's root node.  Note that the
   * name can not be set to an empty string.
   * Default is Root.
   */
  void SetRootNodeName(const char* name);
  vtkGetCharFromStdStringMacro(RootNodeName);
  ///@}

  ///@{
  /**
   * Test to see if reader can the stream.  If the format is provided
   * then it will be set to the determined format.
   *
   * If the format is UNSUPPORTED then the method will return false,
   * else it will return true.
   */
  static bool CanReadFile(vtkResourceStream* stream);
  static bool CanReadFile(vtkResourceStream* stream, Format& format);
  ///@}

  /**
   * Convert a format enum into a string.
   */
  static std::string FormatToString(unsigned int format);

protected:
  vtkOCCTPDCReader();
  ~vtkOCCTPDCReader() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkOCCTPDCReader(const vtkOCCTPDCReader&) = delete;
  void operator=(const vtkOCCTPDCReader&) = delete;

  /**
   *  Determine the format of the input file/stream.  This is used when format is
   * set to AUTO, else the specified format is assumed.
   */
  Format DetermineDataFormat();
  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;

  char* FileName = nullptr;
  vtkSmartPointer<vtkResourceStream> Stream;
  std::string RootNodeName = "Root";
  double LinearDeflection = 0.1;
  double AngularDeflection = 0.5;
  bool RelativeDeflection = false;
  bool ReadWire = false;
  unsigned int FileFormat = Format::AUTO;
  bool CreateRedundantMap = false;
};

VTK_ABI_NAMESPACE_END
#endif

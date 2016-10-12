/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkXMLReader
 * @brief   Superclass for VTK's XML format readers.
 *
 * vtkXMLReader uses vtkXMLDataParser to parse a
 * <a href="http://www.vtk.org/Wiki/VTK_XML_Formats">VTK XML</a> input file.
 * Concrete subclasses then traverse the parsed file structure and extract data.
*/

#ifndef vtkXMLReader_h
#define vtkXMLReader_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkAlgorithm.h"

#include <string> // for std::string

class vtkAbstractArray;
class vtkCallbackCommand;
class vtkDataArraySelection;
class vtkDataSet;
class vtkDataSetAttributes;
class vtkXMLDataElement;
class vtkXMLDataParser;
class vtkInformationVector;
class vtkInformation;
class vtkCommand;

class VTKIOXML_EXPORT vtkXMLReader : public vtkAlgorithm
{
public:
  vtkTypeMacro(vtkXMLReader, vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * Get/Set the name of the input file.
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Enable reading from an InputString instead of the default, a file.
   */
  vtkSetMacro(ReadFromInputString, int);
  vtkGetMacro(ReadFromInputString, int);
  vtkBooleanMacro(ReadFromInputString, int);
  void SetInputString(std::string s) { this->InputString = s; }
  //@}

  /**
   * Test whether the file (type) with the given name can be read by this
   * reader. If the file has a newer version than the reader, we still say
   * we can read the file type and we fail later, when we try to read the file.
   * This enables clients (ParaView) to distinguish between failures when we
   * need to look for another reader and failures when we don't.
   */
  virtual int CanReadFile(const char* name);

  //@{
  /**
   * Get the output as a vtkDataSet pointer.
   */
  vtkDataSet* GetOutputAsDataSet();
  vtkDataSet* GetOutputAsDataSet(int index);
  //@}

  //@{
  /**
   * Get the data array selection tables used to configure which data
   * arrays are loaded by the reader.
   */
  vtkGetObjectMacro(PointDataArraySelection, vtkDataArraySelection);
  vtkGetObjectMacro(CellDataArraySelection, vtkDataArraySelection);
  //@}

  //@{
  /**
   * Get the number of point or cell arrays available in the input.
   */
  int GetNumberOfPointArrays();
  int GetNumberOfCellArrays();
  //@}

  //@{
  /**
   * Get the name of the point or cell array with the given index in
   * the input.
   */
  const char* GetPointArrayName(int index);
  const char* GetCellArrayName(int index);
  //@}

  //@{
  /**
   * Get/Set whether the point or cell array with the given name is to
   * be read.
   */
  int GetPointArrayStatus(const char* name);
  int GetCellArrayStatus(const char* name);
  void SetPointArrayStatus(const char* name, int status);
  void SetCellArrayStatus(const char* name, int status);
  //@}

  // For the specified port, copy the information this reader sets up in
  // SetupOutputInformation to outInfo
  virtual void CopyOutputInformation(vtkInformation *vtkNotUsed(outInfo),
                                   int vtkNotUsed(port)) {}

  //@{
  /**
   * Which TimeStep to read.
   */
  vtkSetMacro(TimeStep, int);
  vtkGetMacro(TimeStep, int);
  //@}

  vtkGetMacro(NumberOfTimeSteps, int);
  //@{
  /**
   * Which TimeStepRange to read
   */
  vtkGetVector2Macro(TimeStepRange, int);
  vtkSetVector2Macro(TimeStepRange, int);
  //@}

  /**
   * Returns the internal XML parser. This can be used to access
   * the XML DOM after RequestInformation() was called.
   */
  vtkXMLDataParser* GetXMLParser()
  {
    return this->XMLParser;
  }

  virtual int ProcessRequest(vtkInformation *request,
                             vtkInformationVector **inputVector,
                             vtkInformationVector *outputVector);

  //@{
  /**
   * Set/get the ErrorObserver for the internal reader
   * This is useful for applications that want to catch error messages.
   */
  void SetReaderErrorObserver(vtkCommand *);
  vtkGetObjectMacro(ReaderErrorObserver, vtkCommand);
  //@}

  //@{
  /**
   * Set/get the ErrorObserver for the internal xml parser
   * This is useful for applications that want to catch error messages.
   */
  void SetParserErrorObserver(vtkCommand *);
  vtkGetObjectMacro(ParserErrorObserver, vtkCommand);
  //@}

protected:
  vtkXMLReader();
  ~vtkXMLReader();

  // Pipeline execution methods to be defined by subclass.  Called by
  // corresponding RequestData methods after appropriate setup has been
  // done.
  virtual int ReadXMLInformation();
  virtual void ReadXMLData();

  // Get the name of the data set being read.
  virtual const char* GetDataSetName()=0;

  // Test if the reader can read a file with the given version number.
  virtual int CanReadFileVersion(int major, int minor);

  // Setup the output with no data available.  Used in error cases.
  virtual void SetupEmptyOutput()=0;

  // Setup the output's information.
  virtual void SetupOutputInformation(vtkInformation *vtkNotUsed(outInfo)) {}

  // Setup the output's data with allocation.
  virtual void SetupOutputData();

  // Read the primary element from the file.  This is the element
  // whose name is the value returned by GetDataSetName().
  virtual int ReadPrimaryElement(vtkXMLDataElement* ePrimary);

  // Read the top-level element from the file.  This is always the
  // VTKFile element.
  virtual int ReadVTKFile(vtkXMLDataElement* eVTKFile);

  // Create a vtkAbstractArray from its cooresponding XML representation.
  // Does not allocate.
  vtkAbstractArray* CreateArray(vtkXMLDataElement* da);

  // Create a vtkInformationKey from its coresponding XML representation.
  // Stores it in the instance of vtkInformationProvided. Does not allocate.
  int CreateInformationKey(vtkXMLDataElement *eInfoKey, vtkInformation *info);

  // Populates the info object with the InformationKey children in infoRoot.
  // Returns false if errors occur.
  bool ReadInformation(vtkXMLDataElement *infoRoot, vtkInformation *info);

  // Internal utility methods.
  virtual int OpenStream();
  virtual void CloseStream();
  virtual int OpenVTKFile();
  virtual void CloseVTKFile();
  virtual int OpenVTKString();
  virtual void CloseVTKString();
  virtual void CreateXMLParser();
  virtual void DestroyXMLParser();
  void SetupCompressor(const char* type);
  int CanReadFileVersionString(const char* version);

  /**
   * This method is used by CanReadFile() to check if the reader can read an XML
   * with the primary element with the given name. Default implementation
   * compares the name with the text returned by this->GetDataSetName().
   */
  virtual int CanReadFileWithDataType(const char* dsname);

  // Returns the major version for the file being read. -1 when invalid.
  vtkGetMacro(FileMajorVersion, int);

  // Returns the minor version for the file being read. -1 when invalid.
  vtkGetMacro(FileMinorVersion, int);

  // Utility methods for subclasses.
  int IntersectExtents(int* extent1, int* extent2, int* result);
  int Min(int a, int b);
  int Max(int a, int b);
  void ComputePointDimensions(int* extent, int* dimensions);
  void ComputePointIncrements(int* extent, vtkIdType* increments);
  void ComputeCellDimensions(int* extent, int* dimensions);
  void ComputeCellIncrements(int* extent, vtkIdType* increments);
  vtkIdType GetStartTuple(int* extent, vtkIdType* increments,
                          int i, int j, int k);
  void ReadAttributeIndices(vtkXMLDataElement* eDSA,
                            vtkDataSetAttributes* dsa);
  char** CreateStringArray(int numStrings);
  void DestroyStringArray(int numStrings, char** strings);

  // Setup the data array selections for the input's set of arrays.
  void SetDataArraySelections(vtkXMLDataElement* eDSA,
                              vtkDataArraySelection* sel);

  int SetFieldDataInfo(vtkXMLDataElement *eDSA, int association,
  int numTuples, vtkInformationVector *(&infoVector));

  // Check whether the given array element is an enabled array.
  int PointDataArrayIsEnabled(vtkXMLDataElement* ePDA);
  int CellDataArrayIsEnabled(vtkXMLDataElement* eCDA);

  // Callback registered with the SelectionObserver.
  static void SelectionModifiedCallback(vtkObject* caller, unsigned long eid,
                                        void* clientdata, void* calldata);

  // Give concrete classes an option to squeeze any output arrays
  // at the end of RequestData.
  virtual void SqueezeOutputArrays(vtkDataObject*) {}

  // The vtkXMLDataParser instance used to hide XML reading details.
  vtkXMLDataParser* XMLParser;

  // The FieldData element representation.
  vtkXMLDataElement* FieldDataElement;

  // The input file's name.
  char* FileName;

  // The stream used to read the input.
  istream* Stream;

  // Whether this object is reading from a string or a file.
  // Default is 0: read from file.
  int ReadFromInputString;

  // The input string.
  std::string InputString;

  // The array selections.
  vtkDataArraySelection* PointDataArraySelection;
  vtkDataArraySelection* CellDataArraySelection;

  // The observer to modify this object when the array selections are
  // modified.
  vtkCallbackCommand* SelectionObserver;

  // Whether there was an error reading the file in RequestInformation.
  int InformationError;

  // Whether there was an error reading the file in RequestData.
  int DataError;

  // incrementally fine-tuned progress updates.
  virtual void GetProgressRange(float* range);
  virtual void SetProgressRange(const float range[2], int curStep, int numSteps);
  virtual void SetProgressRange(const float range[2], int curStep, const float* fractions);
  virtual void UpdateProgressDiscrete(float progress);
  float ProgressRange[2];

  virtual int RequestData(vtkInformation *request,
                          vtkInformationVector **inputVector,
                          vtkInformationVector *outputVector);
  virtual int RequestDataObject(vtkInformation *vtkNotUsed(request),
                                vtkInformationVector **vtkNotUsed(inputVector),
                                vtkInformationVector *vtkNotUsed(outputVector))
    { return 1; }
  virtual int RequestInformation(vtkInformation *request,
                                 vtkInformationVector **inputVector,
                                 vtkInformationVector *outputVector);
  vtkTimeStamp ReadMTime;

  // Whether there was an error reading the XML.
  int ReadError;

  // For structured data keep track of dimensions empty of cells.  For
  // unstructured data these are always zero.  This is used to support
  // 1-D and 2-D cell data.
  int AxesEmpty[3];

  // The timestep currently being read.
  int TimeStep;
  int CurrentTimeStep;
  int NumberOfTimeSteps;
  void SetNumberOfTimeSteps(int num);
  // buffer for reading timestep from the XML file the length is of
  // NumberOfTimeSteps and therefore is always long enough
  int *TimeSteps;
  // Store the range of time steps
  int TimeStepRange[2];

  // Now we need to save what was the last time read for each kind of
  // data to avoid rereading it that is to say we need a var for
  // e.g. PointData/CellData/Points/Cells...
  // See SubClass for details with member vars like PointsTimeStep/PointsOffset

  // Helper function useful to know if a timestep is found in an array of timestep
  static int IsTimeStepInArray(int timestep, int* timesteps, int length);

  vtkDataObject* GetCurrentOutput();
  vtkInformation* GetCurrentOutputInformation();

private:
  // The stream used to read the input if it is in a file.
  ifstream* FileStream;
  // The stream used to read the input if it is in a string.
  std::istringstream* StringStream;
  int TimeStepWasReadOnce;

  int FileMajorVersion;
  int FileMinorVersion;

  vtkDataObject* CurrentOutput;
  vtkInformation* CurrentOutputInformation;

private:
  vtkXMLReader(const vtkXMLReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkXMLReader&) VTK_DELETE_FUNCTION;

  vtkCommand *ReaderErrorObserver;
  vtkCommand *ParserErrorObserver;
};

#endif

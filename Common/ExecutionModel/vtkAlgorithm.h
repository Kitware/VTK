/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAlgorithm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAlgorithm - Superclass for all sources, filters, and sinks in VTK.
// .SECTION Description
// vtkAlgorithm is the superclass for all sources, filters, and sinks
// in VTK.  It defines a generalized interface for executing data
// processing algorithms.  Pipeline connections are associated with
// input and output ports that are independent of the type of data
// passing through the connections.
//
// Instances may be used independently or within pipelines with a
// variety of architectures and update mechanisms.  Pipelines are
// controlled by instances of vtkExecutive.  Every vtkAlgorithm
// instance has an associated vtkExecutive when it is used in a
// pipeline.  The executive is responsible for data flow.

#ifndef __vtkAlgorithm_h
#define __vtkAlgorithm_h

#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkObject.h"

class vtkAbstractArray;
class vtkAlgorithmInternals;
class vtkAlgorithmOutput;
class vtkCollection;
class vtkDataArray;
class vtkDataObject;
class vtkExecutive;
class vtkInformation;
class vtkInformationInformationVectorKey;
class vtkInformationIntegerKey;
class vtkInformationStringKey;
class vtkInformationStringVectorKey;
class vtkInformationVector;

class VTKCOMMONEXECUTIONMODEL_EXPORT vtkAlgorithm : public vtkObject
{
public:
  static vtkAlgorithm *New();
  vtkTypeMacro(vtkAlgorithm,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Values used for setting the desired output precision for various
  // algorithms. Currently, only a few algorithms (vtkContourFilter,
  // vtkThreshold) support changing their output precision.
  //
  // SINGLE_PRECISION - Output single-precision floating-point (i.e. float)
  // DOUBLE_PRECISION - Output double-precision floating-point (i.e. double)
  // DEFAULT_PRECISION - Output precision should match the input precision.
  enum DesiredOutputPrecision
    {
    SINGLE_PRECISION,
    DOUBLE_PRECISION,
    DEFAULT_PRECISION
    };

  // Description:
  // Check whether this algorithm has an assigned executive.  This
  // will NOT create a default executive.
  int HasExecutive();

  // Description:
  // Get this algorithm's executive.  If it has none, a default
  // executive will be created.
  vtkExecutive* GetExecutive();

  // Description:
  // Set this algorithm's executive.  This algorithm is removed from
  // any executive to which it has previously been assigned and then
  // assigned to the given executive.
  virtual void SetExecutive(vtkExecutive* executive);

  // Description:
  // Upstream/Downstream requests form the generalized interface
  // through which executives invoke a algorithm's functionality.
  // Upstream requests correspond to information flow from the
  // algorithm's outputs to its inputs.  Downstream requests
  // correspond to information flow from the algorithm's inputs to its
  // outputs.
  //
  // A downstream request is defined by the contents of the request
  // information object.  The input to the request is stored in the
  // input information vector passed to ProcessRequest.  The results
  // of an downstream request are stored in the output information
  // vector passed to ProcessRequest.
  //
  // An upstream request is defined by the contents of the request
  // information object.  The input to the request is stored in the
  // output information vector passed to ProcessRequest.  The results
  // of an upstream request are stored in the input information vector
  // passed to ProcessRequest.
  //
  // It returns the boolean status of the pipeline (false
  // means failure).
  virtual int ProcessRequest(vtkInformation* request,
                             vtkInformationVector** inInfo,
                             vtkInformationVector* outInfo);

  // Description:
  // Version of ProcessRequest() that is wrapped. This converts the
  // collection to an array and calls the other version.
  int ProcessRequest(vtkInformation* request,
                     vtkCollection* inInfo,
                     vtkInformationVector* outInfo);

  // Description:
  // A special version of ProcessRequest meant specifically for the
  // pipeline modified time request.  See
  // vtkExecutive::ComputePipelineMTime() for details.
  virtual int
  ComputePipelineMTime(vtkInformation* request,
                       vtkInformationVector** inInfoVec,
                       vtkInformationVector* outInfoVec,
                       int requestFromOutputPort,
                       unsigned long* mtime);

  // Description:
  // This method gives the algorithm a chance to modify the contents of a
  // request before or after (specified in the when argument) it is
  // forwarded. The default implementation is empty. Returns 1 on success,
  // 0 on failure. When can be either vtkExecutive::BeforeForward or
  // vtkExecutive::AfterForward.
  virtual int ModifyRequest(vtkInformation* request, int when);

  // Description:
  // Get the information object associated with an input port.  There
  // is one input port per kind of input to the algorithm.  Each input
  // port tells executives what kind of data and downstream requests
  // this algorithm can handle for that input.
  vtkInformation* GetInputPortInformation(int port);

  // Description:
  // Get the information object associated with an output port.  There
  // is one output port per output from the algorithm.  Each output
  // port tells executives what kind of upstream requests this
  // algorithm can handle for that output.
  vtkInformation* GetOutputPortInformation(int port);

  // Description:
  // Set/Get the information object associated with this algorithm.
  vtkGetObjectMacro(Information, vtkInformation);
  virtual void SetInformation(vtkInformation*);

  // Description:
  // Get the number of input ports used by the algorithm.
  int GetNumberOfInputPorts();

  // Description:
  // Get the number of output ports provided by the algorithm.
  int GetNumberOfOutputPorts();

  // Description:
  // Participate in garbage collection.
  virtual void Register(vtkObjectBase* o);
  virtual void UnRegister(vtkObjectBase* o);

  // Description:
  // Set/Get the AbortExecute flag for the process object. Process objects
  // may handle premature termination of execution in different ways.
  vtkSetMacro(AbortExecute,int);
  vtkGetMacro(AbortExecute,int);
  vtkBooleanMacro(AbortExecute,int);

  // Description:
  // Set/Get the execution progress of a process object.
  vtkSetClampMacro(Progress,double,0.0,1.0);
  vtkGetMacro(Progress,double);

  // Description:
  // Update the progress of the process object. If a ProgressMethod exists,
  // executes it.  Then set the Progress ivar to amount. The parameter amount
  // should range between (0,1).
  void UpdateProgress(double amount);

  // Description:
  // Set the current text message associated with the progress state.
  // This may be used by a calling process/GUI.
  // Note: Because SetProgressText() is called from inside RequestData()
  // it does not modify the algorithm object. Algorithms are not
  // allowed to modify themselves from inside RequestData().
  void SetProgressText(const char* ptext);
  vtkGetStringMacro(ProgressText);

  // Description:
  // The error code contains a possible error that occurred while
  // reading or writing the file.
  vtkGetMacro( ErrorCode, unsigned long );

  // left public for performance since it is used in inner loops
  int AbortExecute;

  // Description:
  // Keys used to specify input port requirements.
  static vtkInformationIntegerKey* INPUT_IS_OPTIONAL();
  static vtkInformationIntegerKey* INPUT_IS_REPEATABLE();
  static vtkInformationInformationVectorKey* INPUT_REQUIRED_FIELDS();
  static vtkInformationStringVectorKey* INPUT_REQUIRED_DATA_TYPE();
  static vtkInformationInformationVectorKey* INPUT_ARRAYS_TO_PROCESS();
  static vtkInformationIntegerKey* INPUT_PORT();
  static vtkInformationIntegerKey* INPUT_CONNECTION();


  // Description:
  // Set the input data arrays that this algorithm will
  // process. Specifically the idx array that this algorithm will process
  // (starting from 0) is the array on port, connection with the specified
  // association and name or attribute type (such as SCALARS). The
  // fieldAssociation refers to which field in the data object the array is
  // stored. See vtkDataObject::FieldAssociations for detail.
  virtual void SetInputArrayToProcess(int idx, int port, int connection,
                              int fieldAssociation,
                              const char *name);
  virtual void SetInputArrayToProcess(int idx, int port, int connection,
                              int fieldAssociation,
                              int fieldAttributeType);
  virtual void SetInputArrayToProcess(int idx, vtkInformation *info);

  // Description:
  // String based versions of SetInputArrayToProcess(). Because
  // fieldAssociation and fieldAttributeType are enums, they cannot be
  // easily accessed from scripting language. These methods provides an
  // easy and safe way of passing association and attribute type
  // information. Field association is one of the following:
  // @verbatim
  // vtkDataObject::FIELD_ASSOCIATION_POINTS
  // vtkDataObject::FIELD_ASSOCIATION_CELLS
  // vtkDataObject::FIELD_ASSOCIATION_NONE
  // vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS
  // @endverbatim
  // Attribute type is one of the following:
  // @verbatim
  // vtkDataSetAttributes::SCALARS
  // vtkDataSetAttributes::VECTORS
  // vtkDataSetAttributes::NORMALS
  // vtkDataSetAttributes::TCOORDS
  // vtkDataSetAttributes::TENSORS
  // @endverbatim
  // If the last argument is not an attribute type, it is assumed to
  // be an array name.
  virtual void SetInputArrayToProcess(int idx, int port, int connection,
                              const char* fieldAssociation,
                              const char* attributeTypeorName);

  // Description:
  // Get the info object for the specified input array to this algorithm
  vtkInformation *GetInputArrayInformation(int idx);

  // from here down are convenience methods that really are executive methods



  // Description:
  // Remove all the input data.
  void RemoveAllInputs();

  // Description:
  // Get the data object that will contain the algorithm output for
  // the given port.
  vtkDataObject* GetOutputDataObject(int port);

  // Description:
  // Get the data object that will contain the algorithm input for the given
  // port and given connection.
  vtkDataObject *GetInputDataObject(int port,
                                    int connection);

  // Description:
  // Set the connection for the given input port index.  Each input
  // port of a filter has a specific purpose.  A port may have zero or
  // more connections and the required number is specified by each
  // filter.  Setting the connection with this method removes all
  // other connections from the port.  To add more than one connection
  // use AddInputConnection().
  //
  // The input for the connection is the output port of another
  // filter, which is obtained with GetOutputPort().  Typical usage is
  //
  //   filter2->SetInputConnection(0, filter1->GetOutputPort(0)).
  virtual void SetInputConnection(int port, vtkAlgorithmOutput* input);
  virtual void SetInputConnection(vtkAlgorithmOutput* input);

  // Description:
  // Add a connection to the given input port index.  See
  // SetInputConnection() for details on input connections.  This
  // method is the complement to RemoveInputConnection() in that it
  // adds only the connection specified without affecting other
  // connections.  Typical usage is
  //
  //   filter2->AddInputConnection(0, filter1->GetOutputPort(0)).
  virtual void AddInputConnection(int port, vtkAlgorithmOutput* input);
  virtual void AddInputConnection(vtkAlgorithmOutput* input);

  // Description:
  // Remove a connection from the given input port index.  See
  // SetInputConnection() for details on input connection.  This
  // method is the complement to AddInputConnection() in that it
  // removes only the connection specified without affecting other
  // connections.  Typical usage is
  //
  //   filter2->RemoveInputConnection(0, filter1->GetOutputPort(0)).
  virtual void RemoveInputConnection(int port, vtkAlgorithmOutput* input);

  // Description:
  // Remove a connection given by index idx.
  virtual void RemoveInputConnection(int port, int idx);

  // Description:
  // Removes all input connections.
  virtual void RemoveAllInputConnections(int port);

  // Description:
  // Sets the data-object as an input on the given port index. Setting the input with
  // this method removes all other connections from the port. Internally, this
  // method creates a vtkTrivialProducer instance and sets that as the
  // input-connection for the given port. It is safe to call this method repeatedly
  // with the same input data object. The MTime of the vtkAlgorithm will not
  // change unless the data object changed.
  virtual void SetInputDataObject(int port, vtkDataObject* data);
  virtual void SetInputDataObject(vtkDataObject* data)
    { this->SetInputDataObject(0, data); }

  // Description:
  // Add the data-object as an input to this given port. This will add a new
  // input connection on the specified port without affecting any existing
  // connections on the same input port.
  virtual void AddInputDataObject(int port, vtkDataObject* data);
  virtual void AddInputDataObject(vtkDataObject* data)
    { this->AddInputDataObject(0, data); }

  // Description:
  // Get a proxy object corresponding to the given output port of this
  // algorithm.  The proxy object can be passed to another algorithm's
  // SetInputConnection(), AddInputConnection(), and
  // RemoveInputConnection() methods to modify pipeline connectivity.
  vtkAlgorithmOutput* GetOutputPort(int index);
  vtkAlgorithmOutput* GetOutputPort() {
    return this->GetOutputPort(0); }

  // Description:
  // Get the number of inputs currently connected to a port.
  int GetNumberOfInputConnections(int port);

  // Description:
  // Get the total number of inputs for this algorithm
  int GetTotalNumberOfInputConnections();

  // Description:
  // Get the algorithm output port connected to an input port.
  vtkAlgorithmOutput* GetInputConnection(int port, int index);

  // Description:
  // Returns the algorithm and the output port index of
  // that algorithm connected to a port-index pair.
  vtkAlgorithm* GetInputAlgorithm(int port, int index, int& algPort);

  // Description:
  // Returns the algorithm connected to a port-index pair.
  vtkAlgorithm* GetInputAlgorithm(int port, int index);

  // Description:
  // Equivalent to GetInputAlgorithm(0, 0).
  vtkAlgorithm* GetInputAlgorithm()
  {
    return this->GetInputAlgorithm(0, 0);
  }

  // Description:
  // Returns the executive associated with a particular input
  // connection.
  vtkExecutive* GetInputExecutive(int port, int index);

  // Description:
  // Equivalent to GetInputExecutive(0, 0)
  vtkExecutive* GetInputExecutive()
  {
    return this->GetInputExecutive(0, 0);
  }

  // Description:
  // Return the information object that is associated with
  // a particular input connection. This can be used to get
  // meta-data coming from the REQUEST_INFORMATION pass and set
  // requests for the REQUEST_UPDATE_EXTENT pass. NOTE:
  // Do not use this in any of the pipeline passes. Use
  // the information objects passed as arguments instead.
  vtkInformation* GetInputInformation(int port, int index);

  // Description:
  // Equivalent to GetInputInformation(0, 0)
  vtkInformation* GetInputInformation()
  {
    return this->GetInputInformation(0, 0);
  }

  // Description:
  // Return the information object that is associated with
  // a particular output port. This can be used to set
  // meta-data coming during the REQUEST_INFORMATION. NOTE:
  // Do not use this in any of the pipeline passes. Use
  // the information objects passed as arguments instead.
  vtkInformation* GetOutputInformation(int port);

  // Description:
  // Bring this algorithm's outputs up-to-date.
  virtual void Update(int port);
  virtual void Update();


  // Description:
  // Bring the algorithm's information up-to-date.
  virtual void UpdateInformation();

  // Description::
  // Propagate meta-data upstream.
  virtual void PropagateUpdateExtent();

  // Description:
  // Bring this algorithm's outputs up-to-date.
  virtual void UpdateWholeExtent();

  // Description:
  // Convenience routine to convert from a linear ordering of input
  // connections to a port/connection pair.
  void ConvertTotalInputToPortConnection(int ind, int& port, int& conn);

  //======================================================================
  //The following block of code is to support old style VTK applications. If
  //you are using these calls there are better ways to do it in the new
  //pipeline
  //======================================================================

  // Description:
  // Turn release data flag on or off for all output ports.
  virtual void SetReleaseDataFlag(int);
  virtual int GetReleaseDataFlag();
  void ReleaseDataFlagOn();
  void ReleaseDataFlagOff();

  //========================================================================

  // Description:
  // This detects when the UpdateExtent will generate no data
  // This condition is satisfied when the UpdateExtent has
  // zero volume (0,-1,...) or the UpdateNumberOfPieces is 0.
  // The source uses this call to determine whether to call Execute.
  int UpdateExtentIsEmpty(vtkInformation *pinfo, vtkDataObject *output);
  int UpdateExtentIsEmpty(vtkInformation *pinfo, int extentType);

  // Description:
  // If the DefaultExecutivePrototype is set, a copy of it is created
  // in CreateDefaultExecutive() using NewInstance().
  static void SetDefaultExecutivePrototype(vtkExecutive* proto);

  // Description:
  // Returns the priority of the piece described by the current update
  // extent. The priority is a number between 0.0 and 1.0 with 0 meaning
  // skippable (REQUEST_DATA not needed) and 1.0 meaning important.
  virtual double ComputePriority();

  // Description:
  // These are flags that can be set that let the pipeline keep accurate
  // meta-information for ComputePriority.
  static vtkInformationIntegerKey* PRESERVES_DATASET();
  static vtkInformationIntegerKey* PRESERVES_GEOMETRY();
  static vtkInformationIntegerKey* PRESERVES_BOUNDS();
  static vtkInformationIntegerKey* PRESERVES_TOPOLOGY();
  static vtkInformationIntegerKey* PRESERVES_ATTRIBUTES();
  static vtkInformationIntegerKey* PRESERVES_RANGES();
  static vtkInformationIntegerKey* MANAGES_METAINFORMATION();

  // Description:
  // If the whole output extent is required, this method can be called to set
  // the output update extent to the whole extent. This method assumes that
  // the whole extent is known (that UpdateInformation has been called).
  int SetUpdateExtentToWholeExtent(int port);

  // Description:
  // Convenience function equivalent to SetUpdateExtentToWholeExtent(0)
  // This method assumes that the whole extent is known (that UpdateInformation
  // has been called).
  int SetUpdateExtentToWholeExtent();

  // Description:
  // Set the output update extent in terms of piece and ghost levels.
  void SetUpdateExtent(int port,
                       int piece,int numPieces, int ghostLevel);

  // Description:
  // Convenience function equivalent to SetUpdateExtent(0, piece,
  // numPieces, ghostLevel)
  void SetUpdateExtent(int piece,int numPieces, int ghostLevel)
  {
    this->SetUpdateExtent(0, piece, numPieces, ghostLevel);
  }

  // Description:
  // Set the output update extent for data objects that use 3D extents
  void SetUpdateExtent(int port, int extent[6]);

  // Description:
  // Convenience function equivalent to SetUpdateExtent(0, extent)
  void SetUpdateExtent(int extent[6])
  {
    this->SetUpdateExtent(0, extent);
  }

  // Description:
  // These functions return the update extent for output ports that
  // use 3D extents. Where port is not specified, it is assumed to
  // be 0.
  int* GetUpdateExtent()
  {
    return this->GetUpdateExtent(0);
  }
  int* GetUpdateExtent(int port);
  void GetUpdateExtent(int& x0, int& x1, int& y0, int& y1,
                       int& z0, int& z1)
  {
    this->GetUpdateExtent(0, x0, x1, y0, y1, z0, z1);
  }
  void GetUpdateExtent(int port,
                       int& x0, int& x1, int& y0, int& y1,
                       int& z0, int& z1);
  void GetUpdateExtent(int extent[6])
  {
    this->GetUpdateExtent(0, extent);
  }
  void GetUpdateExtent(int port, int extent[6]);

  // Description:
  // These functions return the update extent for output ports that
  // use piece extents. Where port is not specified, it is assumed to
  // be 0.
  int GetUpdatePiece()
  {
    return this->GetUpdatePiece(0);
  }
  int GetUpdatePiece(int port);
  int GetUpdateNumberOfPieces()
  {
    return this->GetUpdateNumberOfPieces(0);
  }
  int GetUpdateNumberOfPieces(int port);
  int GetUpdateGhostLevel()
  {
    return this->GetUpdateGhostLevel(0);
  }
  int GetUpdateGhostLevel(int port);

protected:
  vtkAlgorithm();
  ~vtkAlgorithm();

  // Keys used to indicate that input/output port information has been
  // filled.
  static vtkInformationIntegerKey* PORT_REQUIREMENTS_FILLED();

  // Arbitrary extra information associated with this algorithm
  vtkInformation* Information;

  // Description:
  // Fill the input port information objects for this algorithm.  This
  // is invoked by the first call to GetInputPortInformation for each
  // port so subclasses can specify what they can handle.
  virtual int FillInputPortInformation(int port, vtkInformation* info);

  // Description:
  // Fill the output port information objects for this algorithm.
  // This is invoked by the first call to GetOutputPortInformation for
  // each port so subclasses can specify what they can handle.
  virtual int FillOutputPortInformation(int port, vtkInformation* info);

  // Description:
  // Set the number of input ports used by the algorithm.
  virtual void SetNumberOfInputPorts(int n);

  // Description:
  // Set the number of output ports provided by the algorithm.
  virtual void SetNumberOfOutputPorts(int n);

  // Helper methods to check input/output port index ranges.
  int InputPortIndexInRange(int index, const char* action);
  int OutputPortIndexInRange(int index, const char* action);

  // Description:
  // Get the assocition of the actual data array for the input array specified
  // by idx, this is only reasonable during the REQUEST_DATA pass.
  int GetInputArrayAssociation(int idx, vtkInformationVector **inputVector);

  // Description:
  // Filters that have multiple connections on one port can use
  // this signature. This will override the connection id that the
  // user set in SetInputArrayToProcess() with the connection id
  // passed. This way, the user specifies one array to process and
  // that information is used to obtain arrays for all the connection
  // on the port with the appropriate connection id substituted.
  int GetInputArrayAssociation(int idx, int connection,
                               vtkInformationVector **inputVector);
  int GetInputArrayAssociation(int idx, vtkDataObject* input);


  // Description:
  // Get the actual data array for the input array specified by idx, this is
  // only reasonable during the REQUEST_DATA pass
  vtkDataArray *GetInputArrayToProcess(int idx,vtkInformationVector **inputVector);
  vtkDataArray *GetInputArrayToProcess(int idx,
                                       vtkInformationVector **inputVector,
                                       int& association);

  // Description:
  // Filters that have multiple connections on one port can use
  // this signature. This will override the connection id that the
  // user set in SetInputArrayToProcess() with the connection id
  // passed. This way, the user specifies one array to process and
  // that information is  used to obtain arrays for all the connection
  // on the port with the appropriate connection id substituted.
  vtkDataArray *GetInputArrayToProcess(int idx,
                                       int connection,
                                       vtkInformationVector **inputVector);
  vtkDataArray *GetInputArrayToProcess(int idx,
                                       int connection,
                                       vtkInformationVector **inputVector,
                                       int& association);
  vtkDataArray *GetInputArrayToProcess(int idx,
                                       vtkDataObject* input);
  vtkDataArray *GetInputArrayToProcess(int idx,
                                       vtkDataObject* input,
                                       int& association);


  // Description:
  // Get the actual data array for the input array specified by idx, this is
  // only reasonable during the REQUEST_DATA pass
  vtkAbstractArray *GetInputAbstractArrayToProcess(int idx,vtkInformationVector **inputVector);
  vtkAbstractArray *GetInputAbstractArrayToProcess
    (int idx, vtkInformationVector **inputVector, int& association);

  // Description:
  // Filters that have multiple connections on one port can use
  // this signature. This will override the connection id that the
  // user set in SetInputArrayToProcess() with the connection id
  // passed. This way, the user specifies one array to process and
  // that information is  used to obtain arrays for all the connection
  // on the port with the appropriate connection id substituted.
  vtkAbstractArray *GetInputAbstractArrayToProcess(int idx,
                                       int connection,
                                       vtkInformationVector **inputVector);
  vtkAbstractArray *GetInputAbstractArrayToProcess(int idx,
                                       int connection,
                                       vtkInformationVector **inputVector,
                                       int& association);
  vtkAbstractArray *GetInputAbstractArrayToProcess(int idx,
                                       vtkDataObject* input);
  vtkAbstractArray *GetInputAbstractArrayToProcess(int idx,
                                       vtkDataObject* input,
                                       int& association);



  // Description:
  // This method takes in an index (as specified in SetInputArrayToProcess)
  // and a pipeline information vector. It then finds the information about
  // input array idx and then uses that information to find the field
  // information from the relevant field in the pifo vector (as done by
  // vtkDataObject::GetActiveFieldInformation)
  vtkInformation *GetInputArrayFieldInformation(int idx,
                                                vtkInformationVector **inputVector);


  // Description:
  // Create a default executive.
  // If the DefaultExecutivePrototype is set, a copy of it is created
  // in CreateDefaultExecutive() using NewInstance().
  // Otherwise, vtkStreamingDemandDrivenPipeline is created.
  virtual vtkExecutive* CreateDefaultExecutive();

  // Description:
  // The error code contains a possible error that occurred while
  // reading or writing the file.
  vtkSetMacro( ErrorCode, unsigned long );
  unsigned long ErrorCode;

  // Progress/Update handling
  double Progress;
  char  *ProgressText;

  // Garbage collection support.
  virtual void ReportReferences(vtkGarbageCollector*);

  // executive methods below

  // Description:
  // Replace the Nth connection on the given input port.  For use only
  // by this class and subclasses.  If this is used to store a NULL
  // input then the subclass must be able to handle NULL inputs in its
  // ProcessRequest method.
  virtual void SetNthInputConnection(int port, int index,
                                     vtkAlgorithmOutput* input);

  // Description:
  // Set the number of input connections on the given input port.  For
  // use only by this class and subclasses.  If this is used to store
  // a NULL input then the subclass must be able to handle NULL inputs
  // in its ProcessRequest method.
  virtual void SetNumberOfInputConnections(int port, int n);

  static vtkExecutive* DefaultExecutivePrototype;

  // Description:
  // These methods are used by subclasses to implement methods to
  // set data objects directly as input. Internally, they create
  // a vtkTrivialProducer that has the data object as output and
  // connect it to the algorithm.
  void SetInputDataInternal(int port, vtkDataObject *input)
    { this->SetInputDataObject(port, input); }
  void AddInputDataInternal(int port, vtkDataObject *input)
    { this->AddInputDataObject(port, input); }

private:
  vtkExecutive* Executive;
  vtkInformationVector* InputPortInformation;
  vtkInformationVector* OutputPortInformation;
  vtkAlgorithmInternals* AlgorithmInternal;
  static void ConnectionAdd(vtkAlgorithm* producer, int producerPort,
                            vtkAlgorithm* consumer, int consumerPort);
  static void ConnectionRemove(vtkAlgorithm* producer, int producerPort,
                               vtkAlgorithm* consumer, int consumerPort);
  static void ConnectionRemoveAllInput(vtkAlgorithm* consumer, int port);
  static void ConnectionRemoveAllOutput(vtkAlgorithm* producer, int port);

private:
  vtkAlgorithm(const vtkAlgorithm&);  // Not implemented.
  void operator=(const vtkAlgorithm&);  // Not implemented.
};

#endif

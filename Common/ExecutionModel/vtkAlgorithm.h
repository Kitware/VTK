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
/**
 * @class   vtkAlgorithm
 * @brief   Superclass for all sources, filters, and sinks in VTK.
 *
 * vtkAlgorithm is the superclass for all sources, filters, and sinks
 * in VTK.  It defines a generalized interface for executing data
 * processing algorithms.  Pipeline connections are associated with
 * input and output ports that are independent of the type of data
 * passing through the connections.
 *
 * Instances may be used independently or within pipelines with a
 * variety of architectures and update mechanisms.  Pipelines are
 * controlled by instances of vtkExecutive.  Every vtkAlgorithm
 * instance has an associated vtkExecutive when it is used in a
 * pipeline.  The executive is responsible for data flow.
*/

#ifndef vtkAlgorithm_h
#define vtkAlgorithm_h

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
class vtkProgressObserver;

class VTKCOMMONEXECUTIONMODEL_EXPORT vtkAlgorithm : public vtkObject
{
public:
  static vtkAlgorithm *New();
  vtkTypeMacro(vtkAlgorithm,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Values used for setting the desired output precision for various
   * algorithms. Currently, the following algorithms support changing their
   * output precision: vtkAppendFilter, vtkAppendPoints, vtkContourFilter,
   * vtkContourGrid, vtkCutter, vtkGridSynchronizedTemplates3D,
   * vtkPolyDataNormals, vtkSynchronizedTemplatesCutter3D,
   * vtkTableBasedClipDataSet, vtkThreshold, vtkTransformFilter, and
   * vtkTransformPolyData.

   * SINGLE_PRECISION - Output single-precision floating-point (i.e. float)
   * DOUBLE_PRECISION - Output double-precision floating-point (i.e. double)
   * DEFAULT_PRECISION - Output precision should match the input precision.
   */
  enum DesiredOutputPrecision
  {
    SINGLE_PRECISION,
    DOUBLE_PRECISION,
    DEFAULT_PRECISION
  };

  /**
   * Check whether this algorithm has an assigned executive.  This
   * will NOT create a default executive.
   */
  int HasExecutive();

  /**
   * Get this algorithm's executive.  If it has none, a default
   * executive will be created.
   */
  vtkExecutive* GetExecutive();

  /**
   * Set this algorithm's executive.  This algorithm is removed from
   * any executive to which it has previously been assigned and then
   * assigned to the given executive.
   */
  virtual void SetExecutive(vtkExecutive* executive);

  /**
   * Upstream/Downstream requests form the generalized interface
   * through which executives invoke a algorithm's functionality.
   * Upstream requests correspond to information flow from the
   * algorithm's outputs to its inputs.  Downstream requests
   * correspond to information flow from the algorithm's inputs to its
   * outputs.

   * A downstream request is defined by the contents of the request
   * information object.  The input to the request is stored in the
   * input information vector passed to ProcessRequest.  The results
   * of an downstream request are stored in the output information
   * vector passed to ProcessRequest.

   * An upstream request is defined by the contents of the request
   * information object.  The input to the request is stored in the
   * output information vector passed to ProcessRequest.  The results
   * of an upstream request are stored in the input information vector
   * passed to ProcessRequest.

   * It returns the boolean status of the pipeline (false
   * means failure).
   */
  virtual int ProcessRequest(vtkInformation* request,
                             vtkInformationVector** inInfo,
                             vtkInformationVector* outInfo);

  /**
   * Version of ProcessRequest() that is wrapped. This converts the
   * collection to an array and calls the other version.
   */
  int ProcessRequest(vtkInformation* request,
                     vtkCollection* inInfo,
                     vtkInformationVector* outInfo);

  /**
   * A special version of ProcessRequest meant specifically for the
   * pipeline modified time request.  See
   * vtkExecutive::ComputePipelineMTime() for details.
   */
  virtual int
  ComputePipelineMTime(vtkInformation* request,
                       vtkInformationVector** inInfoVec,
                       vtkInformationVector* outInfoVec,
                       int requestFromOutputPort,
                       vtkMTimeType* mtime);

  /**
   * This method gives the algorithm a chance to modify the contents of a
   * request before or after (specified in the when argument) it is
   * forwarded. The default implementation is empty. Returns 1 on success,
   * 0 on failure. When can be either vtkExecutive::BeforeForward or
   * vtkExecutive::AfterForward.
   */
  virtual int ModifyRequest(vtkInformation* request, int when);

  /**
   * Get the information object associated with an input port.  There
   * is one input port per kind of input to the algorithm.  Each input
   * port tells executives what kind of data and downstream requests
   * this algorithm can handle for that input.
   */
  vtkInformation* GetInputPortInformation(int port);

  /**
   * Get the information object associated with an output port.  There
   * is one output port per output from the algorithm.  Each output
   * port tells executives what kind of upstream requests this
   * algorithm can handle for that output.
   */
  vtkInformation* GetOutputPortInformation(int port);

  //@{
  /**
   * Set/Get the information object associated with this algorithm.
   */
  vtkGetObjectMacro(Information, vtkInformation);
  virtual void SetInformation(vtkInformation*);
  //@}

  /**
   * Get the number of input ports used by the algorithm.
   */
  int GetNumberOfInputPorts();

  /**
   * Get the number of output ports provided by the algorithm.
   */
  int GetNumberOfOutputPorts();

  //@{
  /**
   * Participate in garbage collection.
   */
  void Register(vtkObjectBase* o) VTK_OVERRIDE;
  void UnRegister(vtkObjectBase* o) VTK_OVERRIDE;
  //@}

  //@{
  /**
   * Set/Get the AbortExecute flag for the process object. Process objects
   * may handle premature termination of execution in different ways.
   */
  vtkSetMacro(AbortExecute,int);
  vtkGetMacro(AbortExecute,int);
  vtkBooleanMacro(AbortExecute,int);
  //@}

  //@{
  /**
   * Set/Get the execution progress of a process object.
   */
  vtkSetClampMacro(Progress,double,0.0,1.0);
  vtkGetMacro(Progress,double);
  //@}

  /**
   * Update the progress of the process object. If a ProgressMethod exists,
   * executes it.  Then set the Progress ivar to amount. The parameter amount
   * should range between (0,1).
   */
  void UpdateProgress(double amount);

  //@{
  /**
   * Set the current text message associated with the progress state.
   * This may be used by a calling process/GUI.
   * Note: Because SetProgressText() is called from inside RequestData()
   * it does not modify the algorithm object. Algorithms are not
   * allowed to modify themselves from inside RequestData().
   */
  void SetProgressText(const char* ptext);
  vtkGetStringMacro(ProgressText);
  //@}

  //@{
  /**
   * The error code contains a possible error that occurred while
   * reading or writing the file.
   */
  vtkGetMacro( ErrorCode, unsigned long );
  //@}

  // left public for performance since it is used in inner loops
  int AbortExecute;

  /**
   * Keys used to specify input port requirements.
   * \ingroup InformationKeys
   */
  static vtkInformationIntegerKey* INPUT_IS_OPTIONAL();
  /**
   * \ingroup InformationKeys
   */
  static vtkInformationIntegerKey* INPUT_IS_REPEATABLE();
  /**
   * \ingroup InformationKeys
   */
  static vtkInformationInformationVectorKey* INPUT_REQUIRED_FIELDS();
  /**
   * \ingroup InformationKeys
   */
  static vtkInformationStringVectorKey* INPUT_REQUIRED_DATA_TYPE();
  /**
   * \ingroup InformationKeys
   */
  static vtkInformationInformationVectorKey* INPUT_ARRAYS_TO_PROCESS();
  /**
   * \ingroup InformationKeys
   */
  static vtkInformationIntegerKey* INPUT_PORT();
  /**
   * \ingroup InformationKeys
   */
  static vtkInformationIntegerKey* INPUT_CONNECTION();

  /**
   * This key tells the executive that a particular output port
   * is capable of producing an arbitrary subextent of the whole
   * extent. Many image sources and readers fall into this category
   * but some such as the legacy structured data readers cannot
   * support this feature.
   * \ingroup InformationKeys
   */
  static vtkInformationIntegerKey* CAN_PRODUCE_SUB_EXTENT();

  /**
   * Key that tells the pipeline that a particular algorithm
   * can or cannot handle piece request. If a filter cannot handle
   * piece requests and is asked for a piece, the executive will
   * flag an error. If a structured data source cannot handle piece
   * requests but can produce sub-extents (CAN_PRODUCE_SUB_EXTENT),
   * the executive will use an extent translator to split the extent
   * into pieces. Otherwise, if a source cannot handle piece requests,
   * the executive will ask for the whole data for piece 0 and not
   * execute the source for other pieces.
   * \ingroup InformationKeys
   */
  static vtkInformationIntegerKey* CAN_HANDLE_PIECE_REQUEST();


  //@{
  /**
   * Set the input data arrays that this algorithm will
   * process. Specifically the idx array that this algorithm will process
   * (starting from 0) is the array on port, connection with the specified
   * association and name or attribute type (such as SCALARS). The
   * fieldAssociation refers to which field in the data object the array is
   * stored. See vtkDataObject::FieldAssociations for detail.
   */
  virtual void SetInputArrayToProcess(int idx, int port, int connection,
                              int fieldAssociation,
                              const char *name);
  virtual void SetInputArrayToProcess(int idx, int port, int connection,
                              int fieldAssociation,
                              int fieldAttributeType);
  virtual void SetInputArrayToProcess(int idx, vtkInformation *info);
  //@}

  /**
   * String based versions of SetInputArrayToProcess(). Because
   * fieldAssociation and fieldAttributeType are enums, they cannot be
   * easily accessed from scripting language. These methods provides an
   * easy and safe way of passing association and attribute type
   * information. Field association is one of the following:
   * @verbatim
   * vtkDataObject::FIELD_ASSOCIATION_POINTS
   * vtkDataObject::FIELD_ASSOCIATION_CELLS
   * vtkDataObject::FIELD_ASSOCIATION_NONE
   * vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS
   * @endverbatim
   * Attribute type is one of the following:
   * @verbatim
   * vtkDataSetAttributes::SCALARS
   * vtkDataSetAttributes::VECTORS
   * vtkDataSetAttributes::NORMALS
   * vtkDataSetAttributes::TCOORDS
   * vtkDataSetAttributes::TENSORS
   * @endverbatim
   * If the last argument is not an attribute type, it is assumed to
   * be an array name.
   */
  virtual void SetInputArrayToProcess(int idx, int port, int connection,
                              const char* fieldAssociation,
                              const char* attributeTypeorName);

  /**
   * Get the info object for the specified input array to this algorithm
   */
  vtkInformation *GetInputArrayInformation(int idx);

  // from here down are convenience methods that really are executive methods



  /**
   * Remove all the input data.
   */
  void RemoveAllInputs();

  /**
   * Get the data object that will contain the algorithm output for
   * the given port.
   */
  vtkDataObject* GetOutputDataObject(int port);

  /**
   * Get the data object that will contain the algorithm input for the given
   * port and given connection.
   */
  vtkDataObject *GetInputDataObject(int port,
                                    int connection);

  //@{
  /**
   * Set the connection for the given input port index.  Each input
   * port of a filter has a specific purpose.  A port may have zero or
   * more connections and the required number is specified by each
   * filter.  Setting the connection with this method removes all
   * other connections from the port.  To add more than one connection
   * use AddInputConnection().

   * The input for the connection is the output port of another
   * filter, which is obtained with GetOutputPort().  Typical usage is

   * filter2->SetInputConnection(0, filter1->GetOutputPort(0)).
   */
  virtual void SetInputConnection(int port, vtkAlgorithmOutput* input);
  virtual void SetInputConnection(vtkAlgorithmOutput* input);
  //@}

  //@{
  /**
   * Add a connection to the given input port index.  See
   * SetInputConnection() for details on input connections.  This
   * method is the complement to RemoveInputConnection() in that it
   * adds only the connection specified without affecting other
   * connections.  Typical usage is

   * filter2->AddInputConnection(0, filter1->GetOutputPort(0)).
   */
  virtual void AddInputConnection(int port, vtkAlgorithmOutput* input);
  virtual void AddInputConnection(vtkAlgorithmOutput* input);
  //@}

  /**
   * Remove a connection from the given input port index.  See
   * SetInputConnection() for details on input connection.  This
   * method is the complement to AddInputConnection() in that it
   * removes only the connection specified without affecting other
   * connections.  Typical usage is

   * filter2->RemoveInputConnection(0, filter1->GetOutputPort(0)).
   */
  virtual void RemoveInputConnection(int port, vtkAlgorithmOutput* input);

  /**
   * Remove a connection given by index idx.
   */
  virtual void RemoveInputConnection(int port, int idx);

  /**
   * Removes all input connections.
   */
  virtual void RemoveAllInputConnections(int port);

  /**
   * Sets the data-object as an input on the given port index. Setting the input with
   * this method removes all other connections from the port. Internally, this
   * method creates a vtkTrivialProducer instance and sets that as the
   * input-connection for the given port. It is safe to call this method repeatedly
   * with the same input data object. The MTime of the vtkAlgorithm will not
   * change unless the data object changed.
   */
  virtual void SetInputDataObject(int port, vtkDataObject* data);
  virtual void SetInputDataObject(vtkDataObject* data)
    { this->SetInputDataObject(0, data); }

  /**
   * Add the data-object as an input to this given port. This will add a new
   * input connection on the specified port without affecting any existing
   * connections on the same input port.
   */
  virtual void AddInputDataObject(int port, vtkDataObject* data);
  virtual void AddInputDataObject(vtkDataObject* data)
    { this->AddInputDataObject(0, data); }

  /**
   * Get a proxy object corresponding to the given output port of this
   * algorithm.  The proxy object can be passed to another algorithm's
   * SetInputConnection(), AddInputConnection(), and
   * RemoveInputConnection() methods to modify pipeline connectivity.
   */
  vtkAlgorithmOutput* GetOutputPort(int index);
  vtkAlgorithmOutput* GetOutputPort() {
    return this->GetOutputPort(0); }

  /**
   * Get the number of inputs currently connected to a port.
   */
  int GetNumberOfInputConnections(int port);

  /**
   * Get the total number of inputs for this algorithm
   */
  int GetTotalNumberOfInputConnections();

  /**
   * Get the algorithm output port connected to an input port.
   */
  vtkAlgorithmOutput* GetInputConnection(int port, int index);

  /**
   * Returns the algorithm and the output port index of
   * that algorithm connected to a port-index pair.
   */
  vtkAlgorithm* GetInputAlgorithm(int port, int index, int& algPort);

  /**
   * Returns the algorithm connected to a port-index pair.
   */
  vtkAlgorithm* GetInputAlgorithm(int port, int index);

  /**
   * Equivalent to GetInputAlgorithm(0, 0).
   */
  vtkAlgorithm* GetInputAlgorithm()
  {
    return this->GetInputAlgorithm(0, 0);
  }

  /**
   * Returns the executive associated with a particular input
   * connection.
   */
  vtkExecutive* GetInputExecutive(int port, int index);

  /**
   * Equivalent to GetInputExecutive(0, 0)
   */
  vtkExecutive* GetInputExecutive()
  {
    return this->GetInputExecutive(0, 0);
  }

  /**
   * Return the information object that is associated with
   * a particular input connection. This can be used to get
   * meta-data coming from the REQUEST_INFORMATION pass and set
   * requests for the REQUEST_UPDATE_EXTENT pass. NOTE:
   * Do not use this in any of the pipeline passes. Use
   * the information objects passed as arguments instead.
   */
  vtkInformation* GetInputInformation(int port, int index);

  /**
   * Equivalent to GetInputInformation(0, 0)
   */
  vtkInformation* GetInputInformation()
  {
    return this->GetInputInformation(0, 0);
  }

  /**
   * Return the information object that is associated with
   * a particular output port. This can be used to set
   * meta-data coming during the REQUEST_INFORMATION. NOTE:
   * Do not use this in any of the pipeline passes. Use
   * the information objects passed as arguments instead.
   */
  vtkInformation* GetOutputInformation(int port);

  //@{
  /**
   * Bring this algorithm's outputs up-to-date.
   */
  virtual void Update(int port);
  virtual void Update();
  //@}

  /**
   * This method enables the passing of data requests to the algorithm
   * to be used during execution (in addition to bringing a particular
   * port up-to-date). The requests argument should contain an information
   * object for each port that requests need to be passed. For each
   * of those, the pipeline will copy all keys to the output information
   * before execution. This is equivalent to:
   * \verbatim
   * algorithm->UpdateInformation();
   * for (int i=0; i<algorithm->GetNumberOfOutputPorts(); i++)
   * {
   * vtkInformation* portRequests = requests->GetInformationObject(i);
   * if (portRequests)
   * {
   * algorithm->GetOutputInformation(i)->Append(portRequests);
   * }
   * }
   * algorithm->Update();
   * \endverbatim
   * Available requests include UPDATE_PIECE_NUMBER(), UPDATE_NUMBER_OF_PIECES()
   * UPDATE_EXTENT() etc etc.
   */
  virtual int Update(int port, vtkInformationVector* requests);

  /**
   * Convenience method to update an algorithm after passing requests
   * to its first output port. See documentation for
   * Update(int port, vtkInformationVector* requests) for details.
   */
  virtual int Update(vtkInformation* requests);

  /**
   * Convenience method to update an algorithm after passing requests
   * to its first output port. See documentation for
   * Update(int port, vtkInformationVector* requests) for details.
   * Supports piece and extent (optional) requests.
   */
  virtual int UpdatePiece(
    int piece, int numPieces, int ghostLevels, const int extents[6]=0);

  /**
   * Convenience method to update an algorithm after passing requests
   * to its first output port.
   * Supports extent request.
   */
  virtual int UpdateExtent(const int extents[6]);

  /**
   * Convenience method to update an algorithm after passing requests
   * to its first output port. See documentation for
   * Update(int port, vtkInformationVector* requests) for details.
   * Supports time, piece (optional) and extent (optional) requests.
   */
  virtual int UpdateTimeStep(double time,
    int piece=-1, int numPieces=1, int ghostLevels=0, const int extents[6]=0);

  /**
   * Bring the algorithm's information up-to-date.
   */
  virtual void UpdateInformation();

  /**
   * Create output object(s).
   */
  virtual void UpdateDataObject();

  /**
   * Propagate meta-data upstream.
   */
  virtual void PropagateUpdateExtent();

  /**
   * Bring this algorithm's outputs up-to-date.
   */
  virtual void UpdateWholeExtent();

  /**
   * Convenience routine to convert from a linear ordering of input
   * connections to a port/connection pair.
   */
  void ConvertTotalInputToPortConnection(int ind, int& port, int& conn);

  //======================================================================
  //The following block of code is to support old style VTK applications. If
  //you are using these calls there are better ways to do it in the new
  //pipeline
  //======================================================================

  //@{
  /**
   * Turn release data flag on or off for all output ports.
   */
  virtual void SetReleaseDataFlag(int);
  virtual int GetReleaseDataFlag();
  void ReleaseDataFlagOn();
  void ReleaseDataFlagOff();
  //@}

  //========================================================================

  //@{
  /**
   * This detects when the UpdateExtent will generate no data
   * This condition is satisfied when the UpdateExtent has
   * zero volume (0,-1,...) or the UpdateNumberOfPieces is 0.
   * The source uses this call to determine whether to call Execute.
   */
  int UpdateExtentIsEmpty(vtkInformation *pinfo, vtkDataObject *output);
  int UpdateExtentIsEmpty(vtkInformation *pinfo, int extentType);
  //@}

  /**
   * If the DefaultExecutivePrototype is set, a copy of it is created
   * in CreateDefaultExecutive() using NewInstance().
   */
  static void SetDefaultExecutivePrototype(vtkExecutive* proto);

  /**
   * If the whole output extent is required, this method can be called to set
   * the output update extent to the whole extent. This method assumes that
   * the whole extent is known (that UpdateInformation has been called).
   */
  VTK_LEGACY(int SetUpdateExtentToWholeExtent(int port));

  /**
   * Convenience function equivalent to SetUpdateExtentToWholeExtent(0)
   * This method assumes that the whole extent is known (that UpdateInformation
   * has been called).
   */
  VTK_LEGACY(int SetUpdateExtentToWholeExtent());

  /**
   * Set the output update extent in terms of piece and ghost levels.
   */
  VTK_LEGACY(void SetUpdateExtent(int port,
                       int piece,int numPieces, int ghostLevel));

  /**
   * Convenience function equivalent to SetUpdateExtent(0, piece,
   * numPieces, ghostLevel)
   */
  VTK_LEGACY(void SetUpdateExtent(
    int piece,int numPieces, int ghostLevel));

  /**
   * Set the output update extent for data objects that use 3D extents
   */
  VTK_LEGACY(void SetUpdateExtent(int port, int extent[6]));

  /**
   * Convenience function equivalent to SetUpdateExtent(0, extent)
   */
  VTK_LEGACY(void SetUpdateExtent(int extent[6]));

  //@{
  /**
   * These functions return the update extent for output ports that
   * use 3D extents. Where port is not specified, it is assumed to
   * be 0.
   */
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
  //@}

  //@{
  /**
   * These functions return the update extent for output ports that
   * use piece extents. Where port is not specified, it is assumed to
   * be 0.
   */
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
  //@}

  //@{
  /**
   * If an ProgressObserver is set, the algorithm will report
   * progress through it rather than directly. This means that
   * it will call UpdateProgress() on the ProgressObserver rather
   * than itself report it and set progress.
   * This is most useful in situations where multiple threads
   * are executing an algorithm at the same time and want to
   * handle progress locally.
   */
  void SetProgressObserver(vtkProgressObserver*);
  vtkGetObjectMacro(ProgressObserver, vtkProgressObserver);
  //@}

protected:
  vtkAlgorithm();
  ~vtkAlgorithm() VTK_OVERRIDE;

  // Keys used to indicate that input/output port information has been
  // filled.
  static vtkInformationIntegerKey* PORT_REQUIREMENTS_FILLED();

  // Arbitrary extra information associated with this algorithm
  vtkInformation* Information;

  /**
   * Fill the input port information objects for this algorithm.  This
   * is invoked by the first call to GetInputPortInformation for each
   * port so subclasses can specify what they can handle.
   */
  virtual int FillInputPortInformation(int port, vtkInformation* info);

  /**
   * Fill the output port information objects for this algorithm.
   * This is invoked by the first call to GetOutputPortInformation for
   * each port so subclasses can specify what they can handle.
   */
  virtual int FillOutputPortInformation(int port, vtkInformation* info);

  /**
   * Set the number of input ports used by the algorithm.
   */
  virtual void SetNumberOfInputPorts(int n);

  /**
   * Set the number of output ports provided by the algorithm.
   */
  virtual void SetNumberOfOutputPorts(int n);

  // Helper methods to check input/output port index ranges.
  int InputPortIndexInRange(int index, const char* action);
  int OutputPortIndexInRange(int index, const char* action);

  /**
   * Get the assocition of the actual data array for the input array specified
   * by idx, this is only reasonable during the REQUEST_DATA pass.
   */
  int GetInputArrayAssociation(int idx, vtkInformationVector **inputVector);

  //@{
  /**
   * Filters that have multiple connections on one port can use
   * this signature. This will override the connection id that the
   * user set in SetInputArrayToProcess() with the connection id
   * passed. This way, the user specifies one array to process and
   * that information is used to obtain arrays for all the connection
   * on the port with the appropriate connection id substituted.
   */
  int GetInputArrayAssociation(int idx, int connection,
                               vtkInformationVector **inputVector);
  int GetInputArrayAssociation(int idx, vtkDataObject* input);
  //@}


  //@{
  /**
   * Get the actual data array for the input array specified by idx, this is
   * only reasonable during the REQUEST_DATA pass
   */
  vtkDataArray *GetInputArrayToProcess(int idx,vtkInformationVector **inputVector);
  vtkDataArray *GetInputArrayToProcess(int idx,
                                       vtkInformationVector **inputVector,
                                       int& association);
  //@}

  //@{
  /**
   * Filters that have multiple connections on one port can use
   * this signature. This will override the connection id that the
   * user set in SetInputArrayToProcess() with the connection id
   * passed. This way, the user specifies one array to process and
   * that information is  used to obtain arrays for all the connection
   * on the port with the appropriate connection id substituted.
   */
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
  //@}


  //@{
  /**
   * Get the actual data array for the input array specified by idx, this is
   * only reasonable during the REQUEST_DATA pass
   */
  vtkAbstractArray *GetInputAbstractArrayToProcess(int idx,vtkInformationVector **inputVector);
  vtkAbstractArray *GetInputAbstractArrayToProcess
    (int idx, vtkInformationVector **inputVector, int& association);
  //@}

  //@{
  /**
   * Filters that have multiple connections on one port can use
   * this signature. This will override the connection id that the
   * user set in SetInputArrayToProcess() with the connection id
   * passed. This way, the user specifies one array to process and
   * that information is  used to obtain arrays for all the connection
   * on the port with the appropriate connection id substituted.
   */
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
  //@}



  /**
   * This method takes in an index (as specified in SetInputArrayToProcess)
   * and a pipeline information vector. It then finds the information about
   * input array idx and then uses that information to find the field
   * information from the relevant field in the pifo vector (as done by
   * vtkDataObject::GetActiveFieldInformation)
   */
  vtkInformation *GetInputArrayFieldInformation(int idx,
                                                vtkInformationVector **inputVector);


  /**
   * Create a default executive.
   * If the DefaultExecutivePrototype is set, a copy of it is created
   * in CreateDefaultExecutive() using NewInstance().
   * Otherwise, vtkStreamingDemandDrivenPipeline is created.
   */
  virtual vtkExecutive* CreateDefaultExecutive();

  //@{
  /**
   * The error code contains a possible error that occurred while
   * reading or writing the file.
   */
  vtkSetMacro( ErrorCode, unsigned long );
  unsigned long ErrorCode;
  //@}

  // Progress/Update handling
  double Progress;
  char  *ProgressText;

  // Garbage collection support.
  void ReportReferences(vtkGarbageCollector*) VTK_OVERRIDE;

  // executive methods below

  /**
   * Replace the Nth connection on the given input port.  For use only
   * by this class and subclasses.  If this is used to store a NULL
   * input then the subclass must be able to handle NULL inputs in its
   * ProcessRequest method.
   */
  virtual void SetNthInputConnection(int port, int index,
                                     vtkAlgorithmOutput* input);

  /**
   * Set the number of input connections on the given input port.  For
   * use only by this class and subclasses.  If this is used to store
   * a NULL input then the subclass must be able to handle NULL inputs
   * in its ProcessRequest method.
   */
  virtual void SetNumberOfInputConnections(int port, int n);

  static vtkExecutive* DefaultExecutivePrototype;

  /**
   * These methods are used by subclasses to implement methods to
   * set data objects directly as input. Internally, they create
   * a vtkTrivialProducer that has the data object as output and
   * connect it to the algorithm.
   */
  void SetInputDataInternal(int port, vtkDataObject *input)
    { this->SetInputDataObject(port, input); }
  void AddInputDataInternal(int port, vtkDataObject *input)
    { this->AddInputDataObject(port, input); }

  vtkProgressObserver* ProgressObserver;

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
  vtkAlgorithm(const vtkAlgorithm&) VTK_DELETE_FUNCTION;
  void operator=(const vtkAlgorithm&) VTK_DELETE_FUNCTION;
};

#endif

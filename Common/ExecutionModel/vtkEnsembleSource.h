/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEnsembleSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkEnsembleSource
 * @brief   source that manages dataset ensembles
 *
 * vtkEnsembleSource manages a collection of data sources in order to
 * represent a dataset ensemble. It has the ability to provide meta-data
 * about the ensemble in the form of a table, using the META_DATA key
 * as well as accept a pipeline request using the UPDATE_MEMBER key.
 * Note that it is expected that all ensemble members produce data of the
 * same type.
*/

#ifndef vtkEnsembleSource_h
#define vtkEnsembleSource_h

#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkAlgorithm.h"

struct vtkEnsembleSourceInternal;
class vtkTable;
class vtkInformationDataObjectMetaDataKey;
class vtkInformationIntegerRequestKey;
class vtkInformationIntegerKey;

class VTKCOMMONEXECUTIONMODEL_EXPORT vtkEnsembleSource : public vtkAlgorithm
{
public:
  static vtkEnsembleSource *New();
  vtkTypeMacro(vtkEnsembleSource,vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Add an algorithm (source) that will produce the next ensemble member.
   * This algorithm will be passed the REQUEST_INFORMATION, REQUEST_UPDATE_EXTENT
   * and REQUEST_DATA pipeline passes for execution.
   */
  void AddMember(vtkAlgorithm*);

  /**
   * Removes all ensemble members.
   */
  void RemoveAllMembers();

  /**
   * Returns the number of ensemble members.
   */
  unsigned int GetNumberOfMembers();

  //@{
  /**
   * Set/Get the current ensemble member to process. Note that this data member
   * will not be used if the UPDATE_MEMBER key is present in the pipeline. Also,
   * this data member may be removed in the future. Unless it is absolutely necessary
   * to use this data member, use the UPDATE_MEMBER key instead.
   */
  vtkSetMacro(CurrentMember, unsigned int);
  vtkGetMacro(CurrentMember, unsigned int);
  //@}

  /**
   * Set the meta-data that will be propagated downstream. Make sure that this table
   * has as many rows as the ensemble members and the meta-data for each row matches
   * the corresponding ensemble source.
   */
  void SetMetaData(vtkTable*);

  /**
   * Meta-data for the ensemble. This is set with SetMetaData.
   */
  static vtkInformationDataObjectMetaDataKey* META_DATA();

  /**
   * Key used to request a particular ensemble member.
   */
  static vtkInformationIntegerRequestKey* UPDATE_MEMBER();

protected:
  vtkEnsembleSource();
  ~vtkEnsembleSource() override;

  static vtkInformationIntegerKey* DATA_MEMBER();

  friend class vtkInformationEnsembleMemberRequestKey;

  int ProcessRequest(vtkInformation *request,
                             vtkInformationVector **inputVector,
                             vtkInformationVector *outputVector) override;
  int FillOutputPortInformation(int, vtkInformation*) override;

  vtkAlgorithm* GetCurrentReader(vtkInformation*);

  vtkEnsembleSourceInternal* Internal;
  unsigned int CurrentMember;

  vtkTable* MetaData;

private:
  vtkEnsembleSource(const vtkEnsembleSource&) = delete;
  void operator=(const vtkEnsembleSource&) = delete;
};

#endif

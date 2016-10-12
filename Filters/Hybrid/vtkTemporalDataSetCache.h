/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTemporalDataSetCache.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkTemporalDataSetCache
 * @brief   cache time steps
 *
 * vtkTemporalDataSetCache cache time step requests of a temporal dataset,
 * when cached data is requested it is returned using a shallow copy.
 * @par Thanks:
 * Ken Martin (Kitware) and John Bidiscombe of
 * CSCS - Swiss National Supercomputing Centre
 * for creating and contributing this class.
 * For related material, please refer to :
 * John Biddiscombe, Berk Geveci, Ken Martin, Kenneth Moreland, David Thompson,
 * "Time Dependent Processing in a Parallel Pipeline Architecture",
 * IEEE Visualization 2007.
*/

#ifndef vtkTemporalDataSetCache_h
#define vtkTemporalDataSetCache_h

#include "vtkFiltersHybridModule.h" // For export macro

#include "vtkAlgorithm.h"
#include <map> // used for the cache

class VTKFILTERSHYBRID_EXPORT vtkTemporalDataSetCache : public vtkAlgorithm
{
public:
  static vtkTemporalDataSetCache *New();
  vtkTypeMacro(vtkTemporalDataSetCache, vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * This is the maximum number of time steps that can be retained in memory.
   * it defaults to 10.
   */
  void SetCacheSize(int size);
  vtkGetMacro(CacheSize,int);
  //@}

protected:
  vtkTemporalDataSetCache();
  ~vtkTemporalDataSetCache();

  int CacheSize;

  typedef std::map<double,std::pair<unsigned long,vtkDataObject *> >
  CacheType;
  CacheType Cache;

  /**
   * see vtkAlgorithm for details
   */
  virtual int ProcessRequest(vtkInformation* request,
                             vtkInformationVector** inputVector,
                             vtkInformationVector* outputVector);

  virtual int FillInputPortInformation(int port, vtkInformation* info);
  virtual int FillOutputPortInformation(int vtkNotUsed(port), vtkInformation* info);
  virtual int RequestDataObject( vtkInformation*,
                                 vtkInformationVector** inputVector ,
                                 vtkInformationVector* outputVector);

  virtual int RequestUpdateExtent (vtkInformation *,
                                   vtkInformationVector **,
                                   vtkInformationVector *);

  virtual int RequestData(vtkInformation *,
                          vtkInformationVector **,
                          vtkInformationVector *);

private:
  vtkTemporalDataSetCache(const vtkTemporalDataSetCache&) VTK_DELETE_FUNCTION;
  void operator=(const vtkTemporalDataSetCache&) VTK_DELETE_FUNCTION;
};



#endif




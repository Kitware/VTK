/*******************************************************************/
/*                               XDMF                              */
/*                   eXtensible Data Model and Format              */
/*                                                                 */
/*  Id : Id  */
/*  Date : $Date$ */
/*  Version : $Revision$ */
/*                                                                 */
/*  Author:                                                        */
/*     Kenneth Leiter                                              */
/*     kenneth.leiter@arl.army.mil                                 */
/*     US Army Research Laboratory                                 */
/*     Aberdeen Proving Ground, MD                                 */
/*                                                                 */
/*     Copyright @ 2010 US Army Research Laboratory                */
/*     All Rights Reserved                                         */
/*     See Copyright.txt or http://www.arl.hpc.mil/ice for details */
/*                                                                 */
/*     This software is distributed WITHOUT ANY WARRANTY; without  */
/*     even the implied warranty of MERCHANTABILITY or FITNESS     */
/*     FOR A PARTICULAR PURPOSE.  See the above copyright notice   */
/*     for more information.                                       */
/*                                                                 */
/*******************************************************************/

#include "Xdmf.h"

/*!
 * @brief XdmfPartitioner partitions an XdmfGrid into a number of XdmfGrids using the metis library.  A pointer to an 
 * XdmfGrid spatial collection is returned containing the partitioned grids.
 */

class XdmfPartitioner
{
  public:
    /*!
     * Constructor.
     */
    XdmfPartitioner();

    /*!
     * Destructor.
     */
    ~XdmfPartitioner();

    /*!
     * 
     * Partitions an XdmfGrid into a number of partitions using the metis library.  Currently supported topology types are:
     *
     * XDMF_TRI, XDMF_TRI_6, XDMF_QUAD, XDMF_QUAD_8, XDMF_TET, XDMF_TET_10, XDMF_HEX, XDMF_HEX_20, XDMF_HEX_24, XDMF_HEX_27
     *
     * The routine splits the XdmfGrid and also splits all attributes and sets into their proper partitions.  An attribute named
     * "GlobalNodeId" is added that serves to map child node ids to their global id for the entire spatial collection.
     *
     * @param grid a XdmfGrid* to partition
     * @param numPartitions the number of partitions to split the grid into
     * @param parentElement the parent XdmfElement* to insert the created spatial collection of partitioned grids.
     *
     * @return XdmfGrid* a spatial collection containing partitioned grids.
     */
    XdmfGrid * Partition(XdmfGrid * grid, int numPartitions, XdmfElement * parentElement);
};

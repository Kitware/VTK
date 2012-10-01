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
/*     Copyright @ 2009 US Army Research Laboratory                */
/*     All Rights Reserved                                         */
/*     See Copyright.txt or http://www.arl.hpc.mil/ice for details */
/*                                                                 */
/*     This software is distributed WITHOUT ANY WARRANTY; without  */
/*     even the implied warranty of MERCHANTABILITY or FITNESS     */
/*     FOR A PARTICULAR PURPOSE.  See the above copyright notice   */
/*     for more information.                                       */
/*                                                                 */
/*******************************************************************/

#include <Xdmf.h>

/*!
 * @brief XdmfExodusReader class encapsulates the operation of reading from a
 *        ExodusII file containing finite element mesh and boundary sets.
 *        Data is read and stored directly into Xdmf format.
 */

class XdmfExodusReader
{
  public:
    /*!
     * Constructor.
     */
    XdmfExodusReader();

    /*!
     * Destructor.
     */
    ~XdmfExodusReader();

    /*!
     * Read the contents of the file and store them internally.
     *
     * @param fileName a const char * containing the path to the exodus ii file to read
     * @param parentElement Pointer to XdmfElement the parent xdmf element to hold the grid - this
     *        should be an XdmfDomain for insertion into the top of the xdmf file, but could be an XdmfGrid
     *        if collections are desired.
     *
     * @return Pointer to XdmfGrid containing the mesh information read in from the exodus ii file
     */
    XdmfGrid * read(const char * fileName, XdmfElement * parentElement);

  private:
    /*!
     * Convert from exodus ii to xdmf cell types
     *
     * @param exoElemType a char * containing the elem_type of the current topology being read
     * @param numPointsPerCell an int the number of points per cell for the current topology being read
     *
     * @return a XdmfInt32 of the xdmf topology type equivalent to the exodus topology being read
     */
    XdmfInt32 DetermineXdmfCellType(char * exoElemType, int numPointsPerCell);
};

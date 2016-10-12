/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPLSDynaReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

/**
 * @class   vtkPLSDynaReader
 * @brief   Read LS-Dyna databases (d3plot) in parallel
 *
 * This filter reads LS-Dyna databases in parallel.
 *
 * The Set/GetFileName() routines are actually wrappers around the
 * Set/GetDatabaseDirectory() members; the actual filename you choose is
 * irrelevant -- only the directory name is used.  This is done in order to
 * accommodate ParaView.
 *
 * @attention
 * LSDyna files contain 3 different types of sections: control, data, and
 * state.  Control sections contain constants that describe the type of
 * simulation data in a file or group of files.  Data sections contain
 * simulation information that is invariant across individual time steps
 * (but can vary when a mesh adaptation occurs).  This information includes
 * material, connectivity, and undeformed geometry.  Finally, state data is
 * information that varies with each time step.  Unless a mesh adaptation
 * occurs, there will be a single control and data section, and they will
 * be located at the start of the database (the first file).
 *
 * @attention
 * In their infinite wisdom, LSDyna developers decided to split simulation
 * data into multiple files, each no larger than some predetermined limit.
 * Each file can contain one section, a partial section (if it would not
 * fit into a single file), or multiple sections. Files are padded with
 * zeros so that their lengths will be multiples of 512*512.  The size of
 * each section is determined by constants in the control and data
 * sections, which means that these must be parsed carefully in order to
 * correctly locate desired information.  Unfortunately, the constants are
 * not terribly well-documented and in some cases the documentation is in
 * error.
 *
 * @par "Open Issues":
 * The LS-Dyna file format document leaves a good bit open to
 * interpretation.  In addition to the "documentation vs. files in the
 * wild" issues there are also implementation problems.
 *
 * @par "Open Issues":
 * - Where exactly may breaks to a new file occur in the pre-state
 * information? At each section?
 * - Will state data sections (node/cell data, element deletion, sph data,
 * rigid body motion) be moved to  the beginning of a new file if their data
 * will be too large for a given file, or are all the sections
 * counted together as a single state (makes more sense for keeping time
 * word at start of every file).
 *  The questions above arise because the docs (p. 3) state "There are 3
 * sections in this database." but then call many smaller pieces of data
 * "sections". Should they be subsections? The docs are quiet about whether
 * the second section (of 3) is ever split across multiple files and, if
 * so, whether it is done at (sub)section boundaries when possible or just
 * wherever it needs to occur.
 * - How many components does Eddy Viscosity have? It's shown as 7 bits in
 * NCFDV1 which makes no sense at all.
 * - Why is NARBS larger than 10+NUMNP+NEL8+NEL2+NEL4+NELT (which is the
 * value specified by the documentation)?
 * Obviously, NARBS is definitive, but what are the extra numbers at the end?
 * - Is there a difference between rigid body elements NUMRBE and rigid road
 * surfaces? It appears that the nodes and connectivity of the road surface
 * are given separately (p.13) while on p.7 the Material
 *   Type Data subsection says that shells in a rigid body will just have a
 * certain material ID but be  interspersed among deformable shell elements.
 * - Word 37 of the control section serves two possible purposes... it can
 * mean NMSPH or EDLOPT.
 *   I assume that different versions of the code use that word differently.
 * How do we know the difference?
 * - It's unclear how much state isn't stored when a shell element is marked
 * as rigid. Specifically, is element deletion data stored for rigid shells?
 * Page 21 of the spec is mute on this.
 * - The loop to read cell User IDs won't work if Rigid Body and Shell
 * elements are interleaved (which I now believe they are).
 *
 * @par "Open Issues":
 * On the VTK side of things:
 * - The reader doesn't handle crack files (d3crck)
 * - The reader doesn't handle interface force files (no default name)
 * - The reader doesn't handle time history (abbreviated output) files (d3thdt)
 * - The reader doesn't handle dynamic relaxation files (d3drfl)
 * - The reader doesn't handle reduced parts (state for a subset of parts) files (d3part)
 * - The reader doesn't handle mode shape files (d3eigv)
 * - The reader doesn't handle equilibrium iteration files (d3iter)
 * - The reader doesn't handle extra time data files (d3xtf)
 * - The reader doesn't handle printer files (d3hsp)
 * - The reader doesn't handle modal neutral files (d3mnf)
 * - The reader doesn't handle packed connectivity.
 * - The reader doesn't handle adapted element parent lists (but the 2002 specification says LSDyna doesn't implement it).
 * - All the sample datasets have MATTYP = 0. Need something to test MATTYP = 1.
 * - I have no test datasets with rigid body and/or road surfaces, so the
 * implementation is half-baked.
 * - It's unclear how some of the data should be presented. Although blindly
 * tacking the numbers into a large chuck of cell data is better than nothing,
 * some attributes (e.g., forces & moments) lend themselves to more elaborate
 * presentation. Also, shell and thick shell elements have stresses that
 * belong to a particular side of an element or have a finite thickness that
 * could be rendered.
 *   Finally, beam elements have cross sections that could be rendered.
 * Some of these operations require numerical processing of the results and
 * so we shouldn't eliminate the ability to get at the raw simulation data.
 * Perhaps a filter could be applied to "fancify" the geometry.
 *
*/

#ifndef vtkPLSDynaReader_h
#define vtkPLSDynaReader_h

#include "vtkIOParallelLSDynaModule.h" // For export macro
#include "vtkLSDynaReader.h"

class vtkMultiProcessController;
class VTKIOPARALLELLSDYNA_EXPORT vtkPLSDynaReader : public vtkLSDynaReader
{
public:
  vtkTypeMacro(vtkPLSDynaReader,vtkLSDynaReader);
  virtual void PrintSelf(ostream &os, vtkIndent indent);
  static vtkPLSDynaReader *New();

  /**
   * Determine if the file can be readed with this reader.
   */
  virtual int CanReadFile( const char* fname );

  //@{
  /**
   * Set/Get the communicator object. By default we use the world controller
   */
  void SetController(vtkMultiProcessController *c);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  //@}

protected:
  vtkPLSDynaReader();
  virtual ~vtkPLSDynaReader();

  virtual int RequestInformation( vtkInformation*, vtkInformationVector**, vtkInformationVector* );
  virtual int RequestData( vtkInformation*, vtkInformationVector**, vtkInformationVector* );

  virtual int ReadTopology();

private:

  vtkPLSDynaReader( const vtkPLSDynaReader& ) VTK_DELETE_FUNCTION;
  void operator = ( const vtkPLSDynaReader& ) VTK_DELETE_FUNCTION;

  void GetPartRanges(vtkIdType* mins,vtkIdType* maxs);

  vtkMultiProcessController *Controller;

  struct vtkPLSDynaReaderInternal;
  vtkPLSDynaReaderInternal *Internal;
};

#endif // vtkPLSDynaReader_h

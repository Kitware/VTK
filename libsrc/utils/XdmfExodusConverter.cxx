/*******************************************************************/
/*                               XDMF                              */
/*                   eXtensible Data Model and Format              */
/*                                                                 */
/*  Id : Id  */
/*  Date : $Date$ */
/*  Version : $ $ */
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

#include "XdmfExodusReader.h"
#include "XdmfExodusWriter.h"
#include <exodusII.h>

#include <sstream>

/**
 * XdmfExodusConverter is a command line utility for converting between Xdmf and ExodusII files.
 * If given a path to an Xdmf file, the tool converts the file to Exodus and if given a path
 * to an Exodus file, the tool converts the file to Xdmf.
 *
 * Usage:
 *     XdmfExodusConverter <path-of-file-to-convert> (Optional: <path-to-output-file>)
 *
 */
int
main(int argc, char* argv[])
{
  std::string usage = "Converts an Exodus II file to XDMF or converts an XDMF file to Exodus II: \n \n Usage: \n \n   XdmfExodusConverter <path-of-file-to-convert> (Optional: <path-to-output-file>)";
  std::string meshName = "";

  if (argc < 2)
  {
    cout << usage << endl;
    return 1;
  }

  if (argc >= 2)
  {
    FILE * refFile = fopen(argv[1], "r");
    if (refFile)
    {
      // Success
      meshName = argv[1];
      fclose(refFile);
    }
    else
    {
      cout << "Cannot open file: " << argv[1] << endl;
      return 1;
    }
    
    if (argc >= 3)
    {
      meshName = argv[2];
    }
  }

  if(meshName.find_last_of("/\\") != std::string::npos)
  {
    meshName = meshName.substr(meshName.find_last_of("/\\")+1, meshName.length());
  }

  if (meshName.rfind(".") != std::string::npos)
  {
    meshName = meshName.substr(0, meshName.rfind("."));
  }

  int CPU_word_size = sizeof(double);
  int IO_word_size = 0; // Get from file
  float version;
  int exodusHandle = ex_open(argv[1], EX_READ, &CPU_word_size, &IO_word_size, &version);
  if(exodusHandle < 0)
  {
    // Xdmf to Exodus
    XdmfDOM dom;
    XdmfInt32 error = dom.Parse(argv[1]);
    if(error == XDMF_FAIL)
    {
      std::cout << "File does not appear to be either an ExodusII or Xdmf file" << std::endl;
      return 1;
    }
    XdmfXmlNode gridElement = dom.FindElementByPath("/Xdmf/Domain/Grid");
    if(gridElement == NULL)
    {
      std::cout << "Cannot parse Xdmf file!" << std::endl;
      return 1;
    }
    XdmfGrid * grid = new XdmfGrid();
    grid->SetDOM(&dom);
    grid->SetElement(gridElement);
    grid->Update();

    std::stringstream outputFileStream;
    outputFileStream << meshName << ".exo";

    XdmfExodusWriter writer;
    writer.write(outputFileStream.str().c_str(), grid);

    cout << "Wrote: " << outputFileStream.str() << endl;

    delete grid;
  }
  else
  {
    // Exodus To Xdmf

    // Initialize xdmf file
    XdmfDOM dom;
    XdmfRoot root;
    XdmfDomain domain;

    root.SetDOM(&dom);
    root.Build();
    root.Insert(&domain);

    XdmfExodusReader reader;
    XdmfGrid * mesh = reader.read(argv[1], &domain);

    std::stringstream outputFileStream;
    outputFileStream << meshName << ".xmf";
    std::string outputFile = outputFileStream.str();

    // Set heavy data set names for geometry and topology
    mesh->SetName(meshName.c_str());

    std::stringstream heavyPointName;
    heavyPointName << meshName << ".h5:/XYZ";
    mesh->GetGeometry()->GetPoints()->SetHeavyDataSetName(heavyPointName.str().c_str());

    std::stringstream heavyConnName;
    heavyConnName << meshName << ".h5:/Connections";
    mesh->GetTopology()->GetConnectivity()->SetHeavyDataSetName(heavyConnName.str().c_str());

    // Set heavy data set names for mesh attributes and sets
    for(int i=0; i<mesh->GetNumberOfAttributes(); i++)
    {
      std::stringstream heavyAttrName;
      heavyAttrName << meshName << ".h5:/Attribute/" << mesh->GetAttribute(i)->GetAttributeCenterAsString() << "/" << mesh->GetAttribute(i)->GetName();
      mesh->GetAttribute(i)->GetValues()->SetHeavyDataSetName(heavyAttrName.str().c_str());
    }

    for(int i=0; i<mesh->GetNumberOfSets(); i++)
    {
      std::stringstream heavySetName;
      heavySetName << meshName << ".h5:/Set/" << mesh->GetSets(i)->GetSetTypeAsString() << "/" << mesh->GetSets(i)->GetName();
      mesh->GetSets(i)->GetIds()->SetHeavyDataSetName(heavySetName.str().c_str());
    }

    mesh->Build();
    dom.Write(outputFile.c_str());

    cout << "Wrote: " << outputFile << endl;

    // cleanup
    delete mesh;
  }

  return 0;
}

/*
 *
 * pdb.cxx - Display PDB file 
 *
 * Author: Randy Heiland, NCSA,UIUC
 * 
 * Usage:
 *  pdb <pdbfile> <atomRadius> <bondRadius> <sphereRes> <cylRes> <bondsFlag>
 *
 * Example:
 *    pdb caffeine.pdb .5 .1 10 6 1
 *    pdb nanotube.pdb .5 .1 10 6 1
 *    pdb wheel.pdb .3 .05 10 8 1
 *    pdb diffGear.pdb 1 .05 6 6 0
 *    pdb fineMotion.pdb 2.5 .05 6 6 0
 *
 *

01234567890123456789012345678901234567890123456789012345678901234567890123456789
Sample PDB file:

ATOM   1849  C1  A77   800       3.484   3.332  10.744 12.0 1.91 .11  0.0 NP -- 
ATOM   1850  O2  A77   800 HD    3.336   4.024  11.730 16.0 1.60 .20  0.0 PA HD 
ATOM   1851  N3  A77   800 HA    3.986   3.834   9.598 14.0 1.72 .17 -0.1 PA HA 
ATOM   1852  C4  A77   800       4.186   3.015   8.397 12.0 2.00 .15  0.0 NP -- 

CONECT 1962 1895                                                        1HVI2148
CONECT 1963 1895   

*/

#undef DEBUG

#include "SaveImage.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vtkRenderWindow.h"
#include "vtkPolyData.h"
#include "vtkRenderer.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSphereSource.h"
#include "vtkLineSource.h"
#include "vtkCylinderSource.h"
#include "vtkGlyph3D.h"
#include "vtkTubeFilter.h"
#include "vtkPolyDataMapper.h"
#include "vtkLookupTable.h"
#include "vtkActor.h"
#include "vtkScalars.h"

// "Known" atoms...
char atomType[] = {'H','C','N','O','S'};
static unsigned char colorMap[5][3] = 
{{255,255,255}, {125,125,125}, {58,144,255}, {255,0,0}, {255,246,0}};
#define MINSCALAR 0.0
#define MAXSCALAR 5.0
#define MAXTYPE 5
static unsigned char unknownAtomColor[3] = {0,255,0};


#define MAXATOM 8300
static float atomPos[MAXATOM][3];
static float atomVal[MAXATOM];

#define MAXBOND 10000
static int bondAtomIdx[MAXBOND][2];
vtkLineSource *bond[MAXBOND];
vtkTubeFilter *bondTube[MAXBOND];

// Global variables to allow access by UserMethod (if necessary)
vtkRenderWindow *renWin;
vtkPolyData *atomsPolyData, *bondsPolyData;

int getLine(FILE *fp, char *s, int lim)
{
  int c, i;

  for (i=0; i<=lim-1 && (c=fgetc(fp))!=EOF && c!='\n'; ++i)
    s[i]=c;
  s[i]='\0';
  return(i);
}

int main(int argc, char **argv)
{
  FILE *fp;
  char lbuf[128],keywd[16],atomId[16],atomSym[8],tok[16],*charPtr;
  int idx, atomIdNum[MAXATOM], atomIdx1,atomIdx2,
    srcAtomIdx = 0,srcAtom,dstAtom, bondsFlag;
  float atomRadius, sphereResolution, bondRadius, cylResolution;
  double xval,yval,zval;

  // This if is to set defaults if we're regression testing
  if( (argc >= 2) && (strcmp("-S", argv[argc-1]) == 0) )
    {
    fp = fopen("../../../vtkdata/finemotion.pdb", "r");
    atomRadius = 2.5;
    bondRadius = .05;
    sphereResolution = 6;
    cylResolution = 6;
    bondsFlag = 0;
    }
  else if (argc < 7)
    {
    printf("Usage: %s <pdbfile> <atomRadius> <bondRadius> <sphereRes> <cylRes> <bondsFlag>\n", argv[0]);
    exit(-1);
    }
  else
    { 
    atomRadius = atof(argv[2]);
    bondRadius = atof(argv[3]);
    sphereResolution = atoi(argv[4]);
    cylResolution = atoi(argv[5]);
    bondsFlag = atoi(argv[6]);

    fp = fopen(argv[1],"r");
    }

  int nchars;
  int nAtoms = 0;
  int nBonds = 0;
  while (1)
    {
    nchars = getLine(fp,lbuf,128);
    if (nchars == 0) break;

#ifdef DEBUG
    printf("from getLine, lbuf=%s\n",lbuf);
    printf("keywd=%s\n",strtok(lbuf, " "));
#endif

    strcpy(keywd,strtok(lbuf, " "));
    if (!strcmp(keywd,"ATOM") || !strcmp(keywd,"HETATM")) 
      {
      strcpy(atomId,strtok(&lbuf[5], " "));
      atomIdNum[nAtoms] = atoi(atomId);
      strcpy(atomSym,strtok(&lbuf[12], " "));

#ifdef DEBUG
      printf("atomId=%s\n",atomId);
      printf("atomIdNum=%d\n",atomIdNum[nAtoms]);
      printf("atomSym=%s\n",atomSym);
      printf("atomSym[0]=%c\n",atomSym[0]);
#endif
      // Default: any unknown atom will take on the MAXSCALAR value
      atomVal[nAtoms] = MAXSCALAR;
      for (int itype=0; itype<MAXTYPE; itype++)
	{
        if (atomSym[0] == atomType[itype])
	  {
          atomVal[nAtoms] = (float)itype;
          break;
	  }
	}
      xval = atof(strtok(&lbuf[29], " "));
      yval = atof(strtok(&lbuf[39], " "));
      zval = atof(strtok(&lbuf[47], " "));
#ifdef DEBUG
      printf("atomVal[nAtoms]= %f\n",atomVal[nAtoms]);
      printf("x,y,z=%f %f %f\n",xval,yval,zval);
#endif
      atomPos[nAtoms][0] = xval;
      atomPos[nAtoms][1] = yval;
      atomPos[nAtoms][2] = zval;
      nAtoms++;

      if (nAtoms > MAXATOM)
	{
        printf("\n\n   Oops - you need to increase MAXATOM and recompile...\n\n");
        exit(-1);
	}
      }
    else if (!strcmp(keywd,"CONECT"))
      {
      if (bondsFlag)
	{

#ifdef DEBUG
	printf("--- Got CONECT:\n");
	printf("lbuf=%s\n",lbuf);
#endif
	// The 1st atom id following the CONECT is the "source" atom for a bond
	srcAtom = atoi(strtok(&lbuf[7], " "));
#ifdef DEBUG
	printf("srcAtom=%d\n",srcAtom);
#endif
	for (idx=0; idx<nAtoms; idx++)
	  {
	  if (srcAtom == atomIdNum[idx])
	    {
	    srcAtomIdx = idx;
	    break;
	    }
	  }
	// The remaining atom ids are the "destination" atoms for bonds
	while (1)
	  {
	  if (!(charPtr=strtok(NULL," "))) break;
	  strcpy(tok, charPtr);
	  dstAtom = atoi(tok);
#ifdef DEBUG
	  printf("=%s, %d\n", tok,dstAtom);
#endif
	  bondAtomIdx[nBonds][0] = srcAtomIdx;
	  for (idx=0; idx<nAtoms; idx++)
	    {
	    if (dstAtom == atomIdNum[idx])
	      {
	      bondAtomIdx[nBonds][1] = idx;
	      break;
	      }
	    }
	  nBonds++;
	  if (nBonds > MAXBOND)
	    {
	    printf("\n\n   Oops - you need to increase MAXBOND and recompile...\n\n");
	    exit(-1);
	    }
	  }
	}
      }
    else if (!strcmp(keywd,"END"))
      {
      }
    else
      {
      printf("unknown keyword = %s\n", keywd);
      printf("lbuf=%s\n",lbuf);
      }
    }

#ifdef DEBUG
  printf("# of atoms= %d\n",nAtoms);
  for (idx=0; idx<nAtoms; idx++)
    printf("%d) %f %f %f\n",idx,atomPos[idx][0],atomPos[idx][1],atomPos[idx][2]);
  printf("# of bonds= %d\n",nBonds);
  for (idx=0; idx<nBonds; idx++)
    printf("%d) %d %d\n",idx,bondAtomIdx[idx][0],bondAtomIdx[idx][1]);
#endif


  //===  VTK-specific  =======================================================
  // Create the vtk renderer stuff
  vtkRenderer *ren = vtkRenderer::New();
  renWin = vtkRenderWindow::New();
  renWin->AddRenderer(ren);
  renWin->SetSize (300,300);
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  // ---- Atoms (as spheres) -----

  // Create points for the starting positions of the particles
  vtkPoints *atomPoints = vtkPoints::New();
  vtkScalars *atomScalars = vtkScalars::New();

  atomPoints->SetDataTypeToFloat();
  
  for (int i=0; i<nAtoms; i++)
    {
    atomPoints->InsertNextPoint(atomPos[i]);
    // Weird... the first works (range [0,1]), the latter doesn't
    // Color sequentially by atom
    atomScalars->InsertScalar(i, (float)i/nAtoms);

    // Color by atomVal (symbol)
    atomScalars->InsertScalar(i, atomVal[i]);
#ifdef DEBUG
    printf("  setting atomScalar = %f\n",atomVal[i]);
#endif
    }

  // Create a data set for the atoms.
  atomsPolyData = vtkPolyData::New();
  atomsPolyData->SetPoints(atomPoints);
  atomPoints->Delete();

  float range[2];
  atomsPolyData->GetPointData()->SetScalars(atomScalars);
  atomsPolyData->GetPointData()->GetScalars()->GetRange(range);

  // Create a vtkSphereSource object to represent an atom
  vtkSphereSource *sphereSource = vtkSphereSource::New();
  // Increase the values here for a smoother sphere
  sphereSource->SetThetaResolution(sphereResolution);
  sphereSource->SetPhiResolution(sphereResolution);
  sphereSource->SetRadius(atomRadius);

  vtkLookupTable *lut = vtkLookupTable::New();
  lut->SetNumberOfColors(MAXTYPE+1);
  for (idx=0; idx<MAXTYPE; idx++)
    {
    lut->SetTableValue(idx,colorMap[idx][0]/255.0,colorMap[idx][1]/255.0,
		       colorMap[idx][2]/255.0, 1.0);
      
    }
  // Make the "unknown" atom color
  lut->SetTableValue(MAXTYPE, unknownAtomColor[0]/255.0,unknownAtomColor[1]/255.0,unknownAtomColor[2]/255.0,1.0);

  // Create atoms, using a vtkGlyph3D object
  vtkGlyph3D *atoms = vtkGlyph3D::New();
  atoms->SetInput(atomsPolyData);
  atoms->SetSource(sphereSource->GetOutput());
  atoms->SetScaleFactor(0.7);
  //    atoms->SetScaleModeToScaleByScalar();
  // To make all atoms the same size
  atoms->SetScaleModeToDataScalingOff();
  atoms->SetColorModeToColorByScalar();
  atoms->SetRange(MINSCALAR,MAXSCALAR);
  atoms->GetRange(range);

  // Create the mapper and actor and finish up the visualization pipeline
  vtkPolyDataMapper *atomsMapper = vtkPolyDataMapper::New();
  atomsMapper->SetInput(atoms->GetOutput());
  atomsMapper->SetScalarRange(MINSCALAR,MAXSCALAR);
  atomsMapper->SetLookupTable(lut);
  vtkActor *atomsActor = vtkActor::New();
  atomsActor->SetMapper(atomsMapper);

  // Make them all one color
  //    atomsActor->GetProperty()->SetColor(0, 1, 1);
  ren->AddActor(atomsActor);


  // ---- Bonds (as lines or tubes) -----
  //#ifdef notdef
  if (bondsFlag)
    {
    for (idx=0; idx<nBonds; idx++)
      {
      //  vtkLineSource *bond[idx] = vtkLineSource::New();
      bond[idx] = vtkLineSource::New();
      bond[idx]->SetResolution(4);
      atomIdx1 = bondAtomIdx[idx][0];
      atomIdx2 = bondAtomIdx[idx][1];
      bond[idx]->SetPoint1(atomPos[atomIdx1][0],atomPos[atomIdx1][1],atomPos[atomIdx1][2]);
      bond[idx]->SetPoint2(atomPos[atomIdx2][0],atomPos[atomIdx2][1],atomPos[atomIdx2][2]);

      if (bondRadius > 0.0)
	{
	bondTube[idx] = vtkTubeFilter::New();
	bondTube[idx]->SetInput(bond[idx]->GetOutput());
	bondTube[idx]->SetRadius(bondRadius);
	bondTube[idx]->SetNumberOfSides(cylResolution);
	}

      // Create the mapper and actor and finish up the visualization pipeline
      vtkPolyDataMapper *bondsMapper = vtkPolyDataMapper::New();
      if (bondRadius > 0.0)
	bondsMapper->SetInput(bondTube[idx]->GetOutput());
      else
	bondsMapper->SetInput(bond[idx]->GetOutput());

      vtkActor *bondsActor = vtkActor::New();
      bondsActor->SetMapper(bondsMapper);

      // Make them all one color
      //    atomsActor->GetProperty()->SetColor(0, 1, 1);
      ren->AddActor(bondsActor);
      }
    }
  //#endif


  // ----------------------------------
  //   ren->SetBackground(1,1,1);
  ren->SetBackground(0,0,0);

  // interact with data
  renWin->Render();

  SAVEIMAGE( renWin );

  iren->Start();

  // Clean up
  ren->Delete();
  renWin->Delete();
  atomsPolyData->Delete();
  sphereSource->Delete();
  atoms->Delete();
  atomsMapper->Delete();
  atomsActor->Delete();
}

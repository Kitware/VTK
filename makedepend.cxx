#ifdef WIN32
#include "pcmaker/stdafx.h"
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fstream.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef WIN32
#include "pcmaker/pcmaker.h"
#include "pcmaker/pcmakerDlg.h"
#endif

#define MAX_DEPENDS 2000

struct DEPENDS_STRUCT
{
	int indices[100];  // aloows up to 100 includes in a single files
	int numIndices;
	char name[256];
};

static DEPENDS_STRUCT *DependsStructArray[MAX_DEPENDS];
static int NumInDepends = 0;

static int  num;
static int dependIndices[MAX_DEPENDS];

void AddToDepends(const char *file, const char *vtkHome, char *extraPtr[], int extra_num);
void GetIncludes(DEPENDS_STRUCT *dependsEntry, const char *vtkHome, char *extraPtr[],
		 int extra_num);

int GetFullPath(char *name, const char *vtkHome, char *fullPath, char *extraPtr[],
		int extra_num)
{
  struct stat statBuff;
  int i;
  
  // does the file exist ?
  // search for it in the vtk src code
  sprintf(fullPath,"%s/common/%s",vtkHome,name);
  if (!stat(fullPath,&statBuff))
    return 1;

  // if control reaches here then it hasn't been found yet
  sprintf(fullPath,"%s/graphics/%s",vtkHome,name);
  if (!stat(fullPath,&statBuff))
    return 1;

  // if control reaches here then it hasn't been found yet
  sprintf(fullPath,"%s/imaging/%s",vtkHome,name);
  if (!stat(fullPath,&statBuff))
    return 1;

  // if control reaches here then it hasn't been found yet
  sprintf(fullPath,"%s/contrib/%s",vtkHome,name);
  if (!stat(fullPath,&statBuff))
    return 1;

  // if control reaches here then it hasn't been found yet
  sprintf(fullPath,"%s/patented/%s",vtkHome,name);
  if (!stat(fullPath,&statBuff))
    return 1;

  // if control reaches here then it hasn't been found yet
  sprintf(fullPath,"%s/local/%s",vtkHome,name);
  if (!stat(fullPath,&statBuff))
    return 1;

  // if control reaches here then it hasn't been found yet
  sprintf(fullPath,"%s/parallel/%s",vtkHome,name);
  if (!stat(fullPath,&statBuff))
    return 1;

  // now do extra directories
  for (i = 0; i < extra_num; i++)
    {
    sprintf(fullPath,"%s/%s",extraPtr[i],name);
    if (!stat(fullPath,&statBuff))
      return 1;
    }

  return 0;
}


void GetDepends(int index)
{
  int i, j;

  for (i = 0; i < DependsStructArray[index]->numIndices; i++)
    {
    // see if entry is alreay in the list
    for (j = 0; j < num; j++)
      {
      if ( DependsStructArray[index]->indices[i] == dependIndices[j] )
        break;
      }
    if (j != num ) // already been added
      continue;

    dependIndices[ num++ ] = DependsStructArray[index]->indices[i];
    GetDepends(DependsStructArray[index]->indices[i]);
    }
}


extern void OutputPCDepends(char *file, FILE *fp, const char *vtkHome, char *extraPtr[],
			    int extra_num)
{
  int i;

  fprintf(fp,"DEPENDS=\\\n");
  num = 0;

  // find this entry in DependsStructArray
  for (i = 0; i < NumInDepends; i++)
    {
    if ( !strcmp(file,DependsStructArray[i]->name) )
      break;
    }

  if ( i == NumInDepends ) // need to add the file (and any new files it needs)
    AddToDepends(file, vtkHome, extraPtr, extra_num);

  GetDepends(i);

  // now output the results
  for (i = 0; i < num; i++)
    {
    fprintf(fp,"  \"%s\"\\\n",DependsStructArray[ dependIndices[i] ]->name);
    }
  fprintf(fp,"\n");
}

extern char **ReturnDepends(int &rnum)
{
  int i, j;
  num = 0;
  char *tname;
  
  for (i = 0; i < NumInDepends; i++)
    {
    GetDepends(i);
    }
  
  rnum = num;
  
  char **list = new char *[num];
  for (i = 0; i < num; i++)
    {
    // we only want the filename, not the full path
    tname = DependsStructArray[ dependIndices[i] ]->name;
    for (j = strlen(tname)-1; j; j--)
      {
      if (tname[j] == '/' || tname[j] == '\\')
        {
        break;
        }
      }
    tname = tname+j+1;
    for (j = 0; j < strlen(tname); j++)
      {
      if (tname[j] == '.')
        {
        break;
        }
      }
    tname[j] = '\0';
    list[i] = strdup(tname);
    }
  return list;
}

extern void CheckAndAddToDepends(char *file, const char *vtkHome, 
                                 char *extraPtr[], int extra_num)
{
  int i;
  num = 0;
  char fullPath[300];
  
  // get the full path
  if (GetFullPath(file, vtkHome, fullPath, extraPtr, extra_num))
    {
    // find this entry in DependsStructArray
    for (i = 0; i < NumInDepends; i++)
      {
      if ( !strcmp(fullPath,DependsStructArray[i]->name) )
        break;
      }
    if ( i == NumInDepends ) 
      {
      // need to add the file (and any new files it needs)
      AddToDepends(fullPath, vtkHome, extraPtr, extra_num);
      }
    }
}

extern void OutputUNIXDepends(char *file, FILE *fp, const char *vtkHome, char *extraPtr[],
			      int extra_num)
{
  int i;

  num = 0;

  // find this entry in DependsStructArray
  for (i = 0; i < NumInDepends; i++)
    {
    if ( !strcmp(file,DependsStructArray[i]->name) )
      break;
    }

  if ( i == NumInDepends ) // need to add the file (and any new files it needs)
    AddToDepends(file, vtkHome, extraPtr, extra_num);

  GetDepends(i);

  // now output the results
  for (i = 0; i < num; i++)
  {
    fprintf(fp," \\\n  %s",DependsStructArray[ dependIndices[i] ]->name);
  }
  fprintf(fp,"\n");
}




void AddToDepends(const char *filename, const char *vtkHome, 
		  char *extraPtr[], int extra_num)
{
  DEPENDS_STRUCT *dependsEntry;

// allocate new entry
  dependsEntry = new DEPENDS_STRUCT;

  dependsEntry->numIndices = 0;
  strcpy( dependsEntry->name, filename );

  if ( NumInDepends >= MAX_DEPENDS )
    {
#ifdef WIN32
    AfxMessageBox("ERROR:  To many depends files... recompile with larger MAX_DEPENDS!!!");
#else
    fprintf(stderr,
       "ERROR:  To many depends files... recompile with larger MAX_DEPENDS!!!");
#endif
    exit(-1);
    }

  DependsStructArray[ NumInDepends++ ] = dependsEntry;

  GetIncludes(DependsStructArray[NumInDepends-1],vtkHome, extraPtr, extra_num);
}



void GetIncludes(DEPENDS_STRUCT *dependsEntry, const char *vtkHome,
		 char *extraPtr[], int extra_num)
{
  ifstream *IS;
  char line[256];
  int j, k;
  char name[256], fullPath[512];
  struct stat statBuff;

  // does the file exist ?
  // search for it in the vtk src code
  if (stat(dependsEntry->name,&statBuff))
    {
#ifdef WIN32
    sprintf(line,"ERROR:  file %s not found... Continuing anyway!", dependsEntry->name);
    AfxMessageBox(line);
#else
    fprintf(stderr,"ERROR:  file %s not found... Continuing anyway!", dependsEntry->name);
#endif
    return;
    }

  IS = new ifstream(dependsEntry->name);

  // search for includes
  while (!IS->eof() && !IS->fail())
    {
    IS->getline(line,255);
    // do we have an include
    if (!strncmp(line,"#include",8))
      {
      // is it a quoted include
      for (j = 8; j < (int)strlen(line); j++)
        {
        if (line[j] == '<') j = 1000;
        else
          {
          if (line[j] == '"')
            {
            // we found a quoted include, process it
            // make sure it is a vtk include file
            if (!strncmp(line +j +1,"vtk",3))
              {
              // extract the class name
              // there should always be an end quote or this will die
              for (k = 0; line[j+k+1] != '"'; k++) 
                {
                name[k] = line[j+k+1];
                }
              name[k] = '\0';

              // Get the full name
              if (!GetFullPath(name, vtkHome, fullPath, extraPtr, extra_num))
                {
#ifdef WIN32
                sprintf(line,"ERROR:  Dependency %s not found!!!", name);
                AfxMessageBox(line);
#else
                fprintf(stderr,"ERROR:  Dependency %s not found!!!", name);
#endif
                exit(-1);
                }

              // get the index in depends
              for (k = 0; k < NumInDepends; k++)
                {
                if ( !strcmp(fullPath,DependsStructArray[k]->name) )
                  {
                  dependsEntry->indices[ dependsEntry->numIndices++ ] = k;
                  break;
                  }
                }

              // if not found, add it to the end
              if ( k == NumInDepends )
                {
                AddToDepends(fullPath, vtkHome, extraPtr, extra_num);
                dependsEntry->indices[ dependsEntry->numIndices++ ] = k;
                }

              break; // break for (j = 8... loop
              }
            else j = 1000;
            }
          } // end if line[j] == '<' and else
        } // end for j = 8 ... strlen(line)
      } // end if (!strncmp(line,"#include"))
    } // end while

  IS->close();
  delete IS;  

}



/*****************************************************************
Similar Code to above but for plitting up the Graphics library.
Messy but gets the job done!!!!
*****************************************************************/

#ifdef WIN32

int GLibFlag[MAX_DEPENDS];

DEPENDS_STRUCT *GLibDependsArray[MAX_DEPENDS];
static int NumInGLibDepends = 0, NumInGLibDependsOriginal;



void AddToGLibDepends(char *file)
{
  DEPENDS_STRUCT *dependsEntry;

// allocate new entry
  dependsEntry = new DEPENDS_STRUCT;

  dependsEntry->numIndices = 0;
  strcpy( dependsEntry->name, file );

  if ( NumInGLibDepends >= MAX_DEPENDS )
    {
    AfxMessageBox("ERROR:  To many depends files... recompile with larger MAX_DEPENDS!!!");
    exit(1);
    }

  GLibDependsArray[ NumInGLibDepends++ ] = dependsEntry;


}



int GetGLibFullPath(char *name, const char *vtkHome, char *fullPath)
{
  struct stat statBuff;

  // if control reaches here then it hasn't been found yet
  sprintf(fullPath,"%s\\graphics\\%s",vtkHome,name);
  if (!stat(fullPath,&statBuff))
    return 1;

  // if control reaches here then it hasn't been found yet
  sprintf(fullPath,"%s\\patented\\%s",vtkHome,name);
  if (!stat(fullPath,&statBuff))
    return 1;


  return 0;
}



void GetGLibIncludes(DEPENDS_STRUCT *dependsEntry, const char *vtkHome )
{
  ifstream *IS;
  char line[256];
  int j, k;
  char name[256], fullPath[512];

  IS = new ifstream(dependsEntry->name);

  // search for includes
  while (!IS->eof())
    {
    IS->getline(line,255);
    // do we have an include
    if (!strncmp(line,"#include",8))
      {
      // is it a quoted include
      for (j = 8; j < (int)strlen(line); j++)
        {
        if (line[j] == '<') j = 1000;
        else
          {
          if (line[j] == '"')
            {
            // we found a quoted include, process it
            // make sure it is a vtk include file
            if (!strncmp(line +j +1,"vtk",3))
              {
              // extract the class name
              // there should always be an end quote or this will die
              for (k = 0; line[j+k+1] != '"'; k++) 
                {
                name[k] = line[j+k+1];
                }
              name[k] = '\0';

              // Get the full name
              if (!GetGLibFullPath(name, vtkHome, fullPath))
                break;

              // get the index in depends
              for (k = 0; k < NumInGLibDepends; k++)
                {
                if ( !strcmp(fullPath,GLibDependsArray[k]->name) )
                  {
                  dependsEntry->indices[ dependsEntry->numIndices++ ] = k;
                  break;
                  }
                }

              // if not found, add it to the end
              if ( k == NumInGLibDepends )
                {
                AddToGLibDepends(fullPath);
                dependsEntry->indices[ dependsEntry->numIndices++ ] = k;
                }

              break; // break for (j = 8... loop
              }
            else j = 1000;
            }
          } // end if line[j] == '<' and else
        } // end for j = 8 ... strlen(line)
      } // end if (!strncmp(line,"#include"))
    } // end while

  IS->close();
  delete IS;  

}



void BuildGLibDepends(CPcmakerDlg *vals)
{
  int i;

  NumInGLibDependsOriginal = NumInGLibDepends;
  for (i = 0; i < NumInGLibDepends; i++)
    {
    GetGLibIncludes(GLibDependsArray[i],vals->m_WhereVTK);
    if (i < NumInGLibDependsOriginal)
      vals->m_Progress.OffsetPos(5);
    }

  for (i=0;i<NumInGLibDepends;i++)
    GLibFlag[i] = 0;
}




void GetGLibDependency(int index)
  {
  int i;

  for (i = 0; i < GLibDependsArray[index]->numIndices; i++)
    {
    int thisIndex = GLibDependsArray[index]->indices[i];

    // for each in list, check to see if flag has been set
    if ( !GLibFlag[ thisIndex ] )
      {
      GLibFlag[ thisIndex ] = 1;
      GetGLibDependency( thisIndex );
      }

    // if cxx file to go with h file, check it
    if ( thisIndex < NumInGLibDependsOriginal && !GLibFlag[ thisIndex + 1 ] )
      {
      GLibFlag[ thisIndex + 1 ] = 1;
      GetGLibDependency( thisIndex + 1 );
      }
    }

  }



int GetGraphicsSplit(int maxSet[])
{
  int i, theIndex;
  int SetOfClasses[MAX_DEPENDS], numInSet;

//  FILE *fp = fopen("GraphicsDependenciesMore.txt","w");
  int maxNumInSet=0, maxNumIndex;

  for (theIndex = 0; theIndex < NumInGLibDependsOriginal; theIndex+=2)
    {
    if ( GLibFlag[theIndex] == -1 )
      continue;
  
    for (i=0;i<NumInGLibDepends;i++)
      {
      if ( GLibFlag[i] != - 1)
        GLibFlag[i] = 0;
      }

    GLibFlag[theIndex] = 1;

    // now get lib dependency for this class
    GetGLibDependency(theIndex);

    // count how many use
    numInSet = 0;
    for (i=0;i<NumInGLibDependsOriginal;i+=2)
      {
      if (GLibFlag[i] > 0)
        SetOfClasses[numInSet++] = i;
      }

    if ( numInSet > maxNumInSet )
      {
      maxNumInSet = numInSet;
      maxNumIndex = theIndex;
      for (i = 0; i < numInSet; i++)
        maxSet[i] = SetOfClasses[i];
      }

    if (theIndex == 0) // "force" PCForce into first library
      break;
    }
  
  // keep track of max.. set all to -1 for the max
  for (i=0;i<maxNumInSet;i++)
    {
    GLibFlag[ maxSet[i] ] = -1;
    if ( maxSet[i] > 0 )
      GLibFlag[ maxSet[i]-1 ] = -1;
    }
  return maxNumInSet;
  }


#endif




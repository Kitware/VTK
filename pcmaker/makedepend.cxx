#include <sys\types.h>
#include <sys\stat.h>
#include <string.h>
#include <fstream.h>
#include <stdio.h>

static char depends[400][256];
static char names[400][256];
static int  num;

void GetDepends(char *file, const char *vtkHome);

void CheckAndAdd(char *name, const char *vtkHome)
{
  char fname[512];
  struct stat statBuff;

  // does the file exist ?
  // search for it in the vtk src code
  sprintf(fname,"%s\\common\\%s",vtkHome,name);
  if (!stat(fname,&statBuff))
    {
    // add this to the depend list
    sprintf(depends[num],"common\\%s",name);
    strcpy(names[num],name);
    num++;
    // now recurse
    GetDepends(fname,vtkHome);
    return;
    }

  // if control reaches here then it hasn't been found yet
  sprintf(fname,"%s\\graphics\\%s",vtkHome,name);
  if (!stat(fname,&statBuff))
    {
    // add this to the depend list
    sprintf(depends[num],"graphics\\%s",name);
    strcpy(names[num],name);
    num++;
    // now recurse
    GetDepends(fname,vtkHome);
    return;
    }

  // if control reaches here then it hasn't been found yet
  sprintf(fname,"%s\\imaging\\%s",vtkHome,name);
  if (!stat(fname,&statBuff))
    {
    // add this to the depend list
    sprintf(depends[num],"imaging\\%s",name);
    strcpy(names[num],name);
    num++;
    // now recurse
    GetDepends(fname,vtkHome);
    return;
    }

  // if control reaches here then it hasn't been found yet
  sprintf(fname,"%s\\contrib\\%s",vtkHome,name);
  if (!stat(fname,&statBuff))
    {
    // add this to the depend list
    sprintf(depends[num],"contrib\\%s",name);
    strcpy(names[num],name);
    num++;
    // now recurse
    GetDepends(fname,vtkHome);
    return;
    }

  // if control reaches here then it hasn't been found yet
  sprintf(fname,"%s\\patented\\%s",vtkHome,name);
  if (!stat(fname,&statBuff))
    {
    // add this to the depend list
    sprintf(depends[num],"patented\\%s",name);
    strcpy(names[num],name);
    num++;
    // now recurse
    GetDepends(fname,vtkHome);
    return;
    }

  // if control reaches here then it hasn't been found yet
  sprintf(fname,"%s\\volume\\%s",vtkHome,name);
  if (!stat(fname,&statBuff))
    {
    // add this to the depend list
    sprintf(depends[num],"volume\\%s",name);
    strcpy(names[num],name);
    num++;
    // now recurse
    GetDepends(fname,vtkHome);
    return;
    }

  // if control reaches here then it hasn't been found yet
  sprintf(fname,"%s\\gemsvolume\\%s",vtkHome,name);
  if (!stat(fname,&statBuff))
    {
    // add this to the depend list
    sprintf(depends[num],"gemsvolume\\%s",name);
    strcpy(names[num],name);
    num++;
    // now recurse
    GetDepends(fname,vtkHome);
    return;
    }
}


void GetDepends(char *file, const char *vtkHome)
{
  ifstream *IS;
  char line[256];
  int present, j, k;
  char name[256];

  IS = new ifstream(file);

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

              // make this isn't already in the list
              present = 0;
              for (k = 0; k < num; k++)
                {
                if (!strcmp(name,names[k])) present = 1;
                }
              // check and see if the file exists and add it
              // this will also recurse if the file exists
              if (!present) CheckAndAdd(name,vtkHome);
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

extern void OutputDepends(char *file, FILE *fp, const char *vtkHome)
{
  int i;

  fprintf(fp,"DEPENDS=\\\n");
  num = 0;

  GetDepends(file,vtkHome);

  // now output the results
  for (i = 0; i < num; i++)
  {
    fprintf(fp,"  \"%s\\%s\"\\\n",vtkHome,depends[i]);
  }
  fprintf(fp,"\n");
}


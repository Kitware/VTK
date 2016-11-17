/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlane.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef SEGYVISUALIZER2D_IOUTIL_H
#define SEGYVISUALIZER2D_IOUTIL_H

#include <fstream>
using namespace std;

class IOUtil {
public:
    int readShortInteger(int pos, ifstream &in);
    int readLongInteger(int pos, ifstream &in);
    float readFloat(ifstream &in);
    char readChar(ifstream &in);
    void swap(char* a, char* b);
    bool isBigEndian;
    static IOUtil* Instance()
    {
        if(instance == NULL)
            instance = new IOUtil();
        return instance;
    }
    int getFileSize(ifstream& in);

private:
    IOUtil();
    static IOUtil* instance;
    bool checkIfBigEndian() {
        ushort a = 0x1234;
        if (*((unsigned char *) &a) == 0x12)
            return true;
        return false;
    }

};


#endif //SEGYVISUALIZER2D_IOUTIL_H

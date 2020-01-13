#!/usr/bin/env python
# -*- coding: utf-8 -*-

'''
=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPartitionedRenderWindowExporter.py

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================
'''

import ctypes
import io
import json
import shutil
import vtk
import vtk.test.Testing
from vtk.util.misc import vtkGetTempDir
import zipfile

VTK_TEMP_DIR = vtkGetTempDir()

# Construct a render window and write it to disk and to buffer. Decompress the
# buffer and compare its contents to the files on disk.
class TestPartitionedRenderWindowExporter(vtk.test.Testing.vtkTest):

    def testPartitionedRenderWindowExporter(self):

        cone = vtk.vtkConeSource()

        coneMapper = vtk.vtkPolyDataMapper()
        coneMapper.SetInputConnection(cone.GetOutputPort())

        coneActor = vtk.vtkActor()
        coneActor.SetMapper(coneMapper)

        ren = vtk.vtkRenderer()
        ren.AddActor(coneActor)
        renWin = vtk.vtkRenderWindow()
        renWin.AddRenderer(ren)

        ren.ResetCamera()
        renWin.Render()

        archiveName = VTK_TEMP_DIR + '/scene'

        exporter = vtk.vtkJSONRenderWindowExporter()
        exporter.GetArchiver().SetArchiveName(archiveName)
        exporter.SetRenderWindow(renWin)
        exporter.Write()

        partitionedExporter = vtk.vtkJSONRenderWindowExporter()
        partitionedArchiver = vtk.vtkPartitionedArchiver()
        partitionedExporter.SetArchiver(partitionedArchiver)
        partitionedExporter.SetRenderWindow(renWin)
        partitionedExporter.Write()

        for i in range(partitionedArchiver.GetNumberOfBuffers()):
            bufferName = partitionedArchiver.GetBufferName(i)
            ptr = partitionedArchiver.GetBufferAddress(bufferName)
            address = int(ptr[1:-7], 16)
            ArrayType = ctypes.c_byte*partitionedArchiver.GetBufferSize(bufferName)
            b = ArrayType.from_address(address)
            stream = io.BytesIO(b)

            if not zipfile.is_zipfile(stream):
                print('Could not access zipped data')
                exit(1)

            with zipfile.ZipFile(stream) as zf:
                for info in zf.infolist():
                    data = zf.read(info.filename)
                    fdata = open(archiveName + '/' + info.filename, 'rb').read()
                    if data != fdata:
                        print('Partitioned file %s does not match file on disk' % info.filename)
                        exit(1)

        shutil.rmtree(archiveName)


if __name__ == "__main__":
     vtk.test.Testing.main([(TestPartitionedRenderWindowExporter, 'test')])

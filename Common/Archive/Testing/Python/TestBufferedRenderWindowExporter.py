#!/usr/bin/env python
# -*- coding: utf-8 -*-



import ctypes
import io
import json
from random import random, seed
import shutil
from vtkmodules.vtkCommonArchive import vtkBufferedArchiver
from vtkmodules.vtkFiltersSources import vtkConeSource
from vtkmodules.vtkIOExport import vtkJSONRenderWindowExporter
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderer,
)
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
import vtkmodules.test.Testing
from vtkmodules.util.misc import vtkGetTempDir
import zipfile

VTK_TEMP_DIR = vtkGetTempDir()

# Construct a render window and write it to disk and to buffer. Decompress the
# buffer and compare its contents to the files on disk.
class TestBufferedRenderWindowExporter(vtkmodules.test.Testing.vtkTest):

    def testBufferedRenderWindowExporter(self):

        cone = vtkConeSource()

        coneMapper = vtkPolyDataMapper()
        coneMapper.SetInputConnection(cone.GetOutputPort())

        coneActor = vtkActor()
        coneActor.SetMapper(coneMapper)

        ren = vtkRenderer()
        ren.AddActor(coneActor)
        renWin = vtkRenderWindow()
        renWin.AddRenderer(ren)

        ren.ResetCamera()
        renWin.Render()

        seed(0)
        archiveName = VTK_TEMP_DIR + '/scene_' + str(random())

        exporter = vtkJSONRenderWindowExporter()
        exporter.GetArchiver().SetArchiveName(archiveName)
        exporter.SetRenderWindow(renWin)
        exporter.Write()

        bufferedExporter = vtkJSONRenderWindowExporter()
        bufferedArchiver = vtkBufferedArchiver()
        bufferedExporter.SetArchiver(bufferedArchiver)
        bufferedExporter.SetRenderWindow(renWin)
        bufferedExporter.Write()

        ptr = bufferedArchiver.GetBufferAddress()
        address = int(ptr[1:-7], 16)
        ArrayType = ctypes.c_byte*bufferedArchiver.GetBufferSize()
        b = ArrayType.from_address(address)

        stream = io.BytesIO(b)

        with zipfile.ZipFile(stream) as zf:
            for info in zf.infolist():
                data = zf.read(info.filename)
                fdata = open(archiveName + '/' + info.filename, 'rb').read()
                if data != fdata:
                    print('Buffered file %s does not match file on disk' % info.filename)
                    exit(1)

        shutil.rmtree(archiveName)


if __name__ == "__main__":
     vtkmodules.test.Testing.main([(TestBufferedRenderWindowExporter, 'test')])

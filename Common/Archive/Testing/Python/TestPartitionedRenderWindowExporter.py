#!/usr/bin/env python
# -*- coding: utf-8 -*-



import ctypes
import io
import json
import shutil
from vtkmodules.vtkCommonArchive import vtkPartitionedArchiver
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
class TestPartitionedRenderWindowExporter(vtkmodules.test.Testing.vtkTest):

    def testPartitionedRenderWindowExporter(self):

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

        archiveName = VTK_TEMP_DIR + '/scene'

        exporter = vtkJSONRenderWindowExporter()
        exporter.GetArchiver().SetArchiveName(archiveName)
        exporter.SetRenderWindow(renWin)
        exporter.Write()

        partitionedExporter = vtkJSONRenderWindowExporter()
        partitionedArchiver = vtkPartitionedArchiver()
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
     vtkmodules.test.Testing.main([(TestPartitionedRenderWindowExporter, 'test')])

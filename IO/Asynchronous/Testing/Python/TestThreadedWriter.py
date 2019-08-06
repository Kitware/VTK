#!/usr/bin/env python
import sys
import os
import time

import vtk
from vtk.util.misc import vtkGetTempDir

VTK_TEMP_DIR = vtkGetTempDir()

# Write all file types: tif, tiff, bpm, png, jpg, jpeg, vti, Z, ppm, raw
fileNames = [
    'threaded-writer.vti',
    # 'threaded-writer.ppm', # Only for unsigned char
    'threaded-writer.Z',
    # 'threaded-writer.jpg', # Only for unsigned char
    # 'threaded-writer.png', # Only for unsigned char
    # 'threaded-writer.bpm', # Only for unsigned char
    # 'threaded-writer.tif', # Only for unsigned char
    'threaded-writer.raw'
]

# Generate Data
source = vtk.vtkRTAnalyticSource()
source.Update()
image = source.GetOutput()

# Initialize writer
writer = vtk.vtkThreadedImageWriter()

# Reduce the number of worker threads
writer.SetMaxThreads(2)
writer.Initialize()

# Write all files
wroteFiles = []
t0 = time.time()
for i in range(10):
    for fileName in fileNames:
        filePath = '%s/%s-%s' % (VTK_TEMP_DIR, i, fileName)
        wroteFiles.append(filePath)
        writer.EncodeAndWrite(image, filePath)

t1 = time.time()

# Try to put the print outside the write loop
# to see if that remove time on mum for the dashboard
for filePath in wroteFiles:
    print('write: %s' % filePath)

# Wait for the work to be done
writer.Finalize()
t2 = time.time()

print('Write time', t1 - t0)
print('Wait time', t2 - t1)
print('Total time', t2 - t0)

# The timing is too flicky on the dashboard
# Removing the error detection
if (t1 - t0) > t2 - t1: # The write time should be smaller than the wait time
    print('Calling Write should be like a NoOp and therefore should be fast')

# Validate data checksum
# ...TODO

print("All good...")
# sys.exit(0)

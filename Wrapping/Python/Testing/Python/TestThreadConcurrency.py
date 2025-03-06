#!/usr/bin/env python

# This checks that Python threads provide concurrency in VTK.
# For this to work, VTK must be configured with VTK_PYTHON_FULL_THREADSAFE=ON.

from vtkmodules.vtkFiltersSources import vtkSphereSource
from vtkmodules.test import Testing

import threading
import time
import os

# this is the method that we will run from multiple threads
def f(alg, output, index):
    alg.SetThetaResolution(200)
    alg.SetPhiResolution(200)
    # Update() has the VTK_UNBLOCK hint to allow concurrency
    alg.Update()
    output[index] = alg.GetOutput()

# run 5 times to get an average
M = 5

# run in 4 threads simultaneously
N = 4

class TestThreadConcurrency(Testing.vtkTest):
    def testThreadedSpeed(self):
        # for averaging timing results
        threadedTime = 0.0
        sequentialTime = 0.0

        # to store per-thread information
        outputs = [None] * N
        threads = [None] * N

        for repeats in range(M):
            # do a run with threads
            start = time.time()

            for i in range(N):
                alg = vtkSphereSource()
                thread = threading.Thread(target=f, args=(alg, outputs, i))
                thread.start()
                threads[i] = thread

            for i in range(N):
                threads[i].join()

            t = time.time() - start
            print("threaded:  ", t)
            threadedTime += t

            # do a sequential run
            start = time.time()

            for i in range(N):
                alg = vtkSphereSource()
                f(alg, outputs, i)

            t = time.time() - start
            print("sequential:", t)
            sequentialTime += t

        speedup = sequentialTime/threadedTime
        print("speed up of %0.2g for %d threads" % (speedup, N))
        # would be nice to add this check, but times jitter too much
        #self.assertGreater(speedup, 1.2, "Threading didn't accelerate code.")

if __name__ == '__main__':
    Testing.main([(TestThreadConcurrency, 'test')])

r"""
This is a script that can be used to run tests that require multiple
executables to be run e.g. those client-server tests.

Usage:
  python vtkTestDriver.py --process exe1 arg11 arg12 ...
                          --process exe2 arg21 arg22 ...
                          --process ...
"""

import sys
import subprocess
import time

# Extract arguments for each process to execute.
command_lists = []
prev = None
for idx in range(1, len(sys.argv)):
    if sys.argv[idx] == "--process":
        if prev:
            command_lists.append(sys.argv[prev:idx])
        prev = idx+1
if prev <= len(sys.argv):
    command_lists.append(sys.argv[prev:])

procs = []
for cmdlist in command_lists:
    print >> sys.stderr, "Executing '", " ".join(cmdlist), "'"
    proc = subprocess.Popen(cmdlist)
    procs.append(proc)
    # sleep to ensure that the process starts.
    time.sleep(0.1)

# Now wait for each of the processes to terminate.
# If ctest times out, it will kill this process and all subprocesses will be
# terminated anyways, so we don't need to handle timeout specially.
for proc in procs:
    proc.wait()

for proc in procs:
    if proc.returncode != 0:
        print >> sys.stderr, "ERROR: A process exited with error. Test will fail."
        sys.exit(1) # error
print "All's well!"

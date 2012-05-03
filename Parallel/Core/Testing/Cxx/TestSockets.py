""" Driver script for testing sockets
    Unix only
"""
import os, sys, time

# Fork, run server in child, client in parent
pid = os.fork()
if pid == 0:
    # exec the parent
    os.execv(sys.argv[1], ('-D', sys.argv[3]))
else:
    # wait a little to make sure that the server is ready
    time.sleep(10)
    # run the client
    retVal = os.system('"%s" -D "%s" -V "%s" -T "%s"' %
                       ( sys.argv[2], sys.argv[3], sys.argv[4], sys.argv[5] ))
    # in case the client fails, we need to kill the server
    # or it will stay around
    time.sleep(20)
    try:
      os.kill(pid, 15)
    except:
      pass
    try:
      status = os.WEXITSTATUS(retVal)
    except:
      status = 0
      print "Cannot get exit status"
    sys.exit(status)

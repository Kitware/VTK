import sys, os

print os.path.join(sys.prefix,'lib','python'+sys.version[0:3],
                   'site-packages','')

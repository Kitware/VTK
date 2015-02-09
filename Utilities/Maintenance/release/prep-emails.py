#!/usr/bin/env python

import os
import re
import sys
import time

version = sys.argv[1]

template = \
"""Hi,

Thank you for contributing to VTK!

If you would like your recent work (listed below) to be mentioned in the vtk
{version} release notes, please summarize your changes and we will try to work the
description in.

{changes}
Thanks,

VTK Maintenance Team
"""

with open('changes.txt', 'r') as changes:
    committer = ''
    contribs = []

    for change in changes:
        email = re.search('.*<(.*@.*)>.*', change)
        if email:
            if committer:
                print committer
                with open('%s.txt' % committer, 'w+') as output:
                    output.write(template.format(
                        version=version,
                        changes=''.join(contribs)))
            committer = email.group(1)
            contribs = []
        if change != '\n':
            contribs.append(change)

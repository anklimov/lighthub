# -*- coding: utf-8 -*-
from __future__ import print_function
import os
import sys
print(sys.argv[1:], file=sys.stderr)
#print("build_flags_"+sys.argv[1:][0])
try:
    file = open("build-flags/build_flags_"+sys.argv[1:][0])
except IOError as e:
    print(u'No build flags file ', file=sys.stderr)
else:
    with file:
           for line in file.readlines():
               if not line.startswith('#'):
                    print (line)
           file.close()

try:
    file = open("custom-build-flags/build_flags_"+sys.argv[1:][0])
except IOError as e:
    print(u'No custom build flags file ', file=sys.stderr)
else:
    with file:
           for line in file.readlines():
               if not line.startswith('#'):
                    print (line)
           file.close()

sys.stdout.write("-DPIO_SRC_REV=")
sys.stdout.flush()
os.system("git log --pretty=format:%h_%ad -1 --date=short")

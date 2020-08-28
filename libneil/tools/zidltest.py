#!/usr/bin/env python
# encoding: latin-1
# zzub IDL parser test

import os

ret = os.system('./zidl --c-header test_zidl.h libzzub.zidl')
if ret != 0:
	sys.exit(1)

data = open("test_zidl.h","r").read()
print data


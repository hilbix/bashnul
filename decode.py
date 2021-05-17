#!/usr/bin/env python3

import sys
dang = False
while 1:
	a	= sys.stdin.buffer.read(102400);
	if not a: break
	if dang:
		a	= b'\1'+a
		dang	= False
	if a[-1] == 1:
		dang	= True
		a	= a[:-1]
	sys.stdout.buffer.write(a.replace(b'\1\2', b'\0').replace(b'\1\3', b'\1'))


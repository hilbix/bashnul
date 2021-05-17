#!/usr/bin/env python3

import sys
while 1:
	a = sys.stdin.buffer.read(102400);
	if not a: break
	sys.stdout.buffer.write(a.replace(b'\1', b'\1\3').replace(b'\0', b'\1\2'))


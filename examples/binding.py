import sys

sys.path.insert(0, 'build/lib.linux-x86_64-2.7')

import wsp

w = wsp.open('./test.wsp', wsp.MMAP)
w = wsp.Whisper()
w.open('./test.wsp')
w.update_point(0, 10.0)
print "done"

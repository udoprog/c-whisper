import sys

sys.path.insert(0, 'build/lib.linux-x86_64-2.7')

import wsp

#wsp.create('./test.wsp')
#w = wsp.open('./test.wsp', wsp.MMAP)
w = wsp.Whisper()
w.update_points([(0, 10.0), (100, 23.9)])
print "done"

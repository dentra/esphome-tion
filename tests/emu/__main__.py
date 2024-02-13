import os
import sys

sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from emu import o2
from emu.o2_main import main

if len(sys.argv) < 2:
    print("Usage: o2 port [port]")
    exit(1)

# main(sys.argv[1:])
flags = o2.ErrorFlags(0xFFFF)
print(flags)

# cmd = o2.DevModeCommand(bytes(b"\x02"))
# print("%s" % cmd.flags)
# flags = o2.StateFlags(0)
# print(flags)
# flags = flags | o2.StateFlags.POWER
# print(flags)
# flags = flags | o2.StateFlags.HEAT
# print(flags)
# flags = flags & o2.StateFlags.POWER
# print(flags)

# print(flags & o2.StateFlags.POWER != 0)
# print(flags & o2.StateFlags.HEAT != 0)
# print(flags & o2.StateFlags.UNKNOWN2 != 0)


# print(o2.WorkModeFlags(0xFF))
# print(o2.DevModeFlags(0xFF))

# flag = o2.DevModeFlags(o2.DevModeFlags.USER)
# print(flag)
# flag = o2.set_flag(flag, o2.DevModeFlags.PAIR, True)
# print(flag)
# flag = o2.set_flag(flag, o2.DevModeFlags.PAIR, False)
# print(flag)
# flag = o2.set_flag(flag, o2.DevModeFlags.PAIR, True)
# print(flag)
# flag = o2.set_flag(flag, o2.DevModeFlags.PAIR, False)
# print(flag)

# # flag = flag | o2.DevModeFlags.USER
# # print(flag)

# buf = bytearray([1])
# buf.extend(bytes([2, 3, 4]))
# buf.append(5)
# print(type(buf), buf)
# buf = bytes(buf)
# print(type(buf), buf)

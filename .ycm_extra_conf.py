import os
import ycm_core

flags = [
'-Wall',
'-Wextra',
'-pedantic',
'-m32',
'-O0',
'-std=c99',
'-finline-functions',
'-fno-stack-protector',
'-ffreestanding',
'-Wno-unused-function',
'-Wno-unused-parameter',
'-g',
'-Wno-gnu',
'-target',
'i386-pc-linux',
'-mno-sse',
'-mno-mmx',
'-DUSE_CLANG_COMPLETER'
]

def FlagsForFile( filename ):
  if "kernel" in filename:
    flags.append("-Ikernel/include")

  return {
    'flags': flags,
    'do_cache': True
  }

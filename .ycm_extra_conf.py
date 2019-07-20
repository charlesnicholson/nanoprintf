import os
import ycm_core
import ntpath

flags = [
    '-DNANOPRINTF_IMPLEMENTATION',
    '-DNANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS=1',
    '-DNANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS=1',
    '-DNANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS=1',
    '-DNANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS=1',
    '-DNANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS=1',
    '-Wall',
    '-Wextra',
    '-Wall',
    '-Weverything',
    '-Wpedantic',
    '-ansi',
    '-Wno-c++98-compat-pedantic '
    '-Wno-c++11-long-long',
    '-Wno-reserved-id-macro',
    '-Wno-old-style-cast',
    '-Wno-keyword-macro',
    '-Wno-disabled-macro-expansion',
    '-Wno-weak-vtables',
    '-Wno-global-constructors',
    '-Wno-exit-time-destructors',
    '-Wno-padded',
    '-I', 'external/CppUTest/src/CppUTest_external/include'
]

cpp_flags = [ '-std=c++11', '-x', 'c++' ]


def DirectoryOfThisScript():
  return os.path.dirname( os.path.abspath( __file__ ) )


def MakeRelativePathsInFlagsAbsolute( flags, working_directory ):
  if not working_directory:
    return list( flags )
  new_flags = []
  make_next_absolute = False
  path_flags = [ '-isystem', '-I', '-iquote', '--sysroot=' ]
  for flag in flags:
    new_flag = flag

    if make_next_absolute:
      make_next_absolute = False
      if not flag.startswith( '/' ):
        new_flag = os.path.join( working_directory, flag )

    for path_flag in path_flags:
      if flag == path_flag:
        make_next_absolute = True
        break

      if flag.startswith( path_flag ):
        path = flag[ len( path_flag ): ]
        new_flag = path_flag + os.path.join( working_directory, path )
        break

    if new_flag:
      new_flags.append( new_flag )
  return new_flags

def PathLeaf(path):
    head, tail = ntpath.split(path)
    return tail or ntpath.basename(head)

def FlagsForFile( filename, **kwargs ):
  relative_to = DirectoryOfThisScript()
  final_flags = MakeRelativePathsInFlagsAbsolute(flags, relative_to) + cpp_flags

  return {
    'flags': final_flags,
    'do_cache': True
  }

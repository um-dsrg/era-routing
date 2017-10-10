import os

homeDir = os.path.expanduser('~')

flags = [
        '-std=c++11',
        '-Werror',
        '-Wall',
        '-I'+homeDir+'/libraries/lemon/include'
        ]

def FlagsForFile( filename, **kwargs ):
    return { 'flags': flags }

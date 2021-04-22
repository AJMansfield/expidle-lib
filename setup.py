from distutils.core import setup, Extension
import numpy as np

ext_modules = []

ext = Extension('exptypes.hugenum.cHugenum',
                sources=['exptypes/hugenum/hugenum.c',
                         'exptypes/hugenum/cHugenum.c'],
                include_dirs=[np.get_include()],
                extra_compile_args=['-std=c99'])
ext_modules.append(ext)

setup(name='exptypes',
      version='0.1',
      description='NumPy type extensions',
      packages=['exptypes',
                'exptypes.hugenum',
                ],
      ext_modules=ext_modules)

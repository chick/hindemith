#!/usr/bin/env python

import distribute_setup
import hindemith

distribute_setup.use_setuptools()

from setuptools import setup

setup(name="hindemith",
      version=hindemith.__version__,
      description="A framework for efficiently composing hand-written OpenCL from Python",
      long_description="""
      Hindemith uses SEJITS methodology to optimally construct OpenCL
      code at runtime
      See https://github/chick/hindemith/wiki
      """,
      classifiers=[
          'Development Status :: 3 - Alpha',
          'Intended Audience :: Developers',
          'Intended Audience :: Other Audience',
          'Intended Audience :: Science/Research',
          'License :: OSI Approved :: BSD License',
          'Natural Language :: English',
          'Programming Language :: Python',
          'Topic :: Scientific/Engineering',
          'Topic :: Software Development :: Libraries',
          'Topic :: Utilities',
      ],

      author=u"Michael J. Anderson",
      url="http://github.com/chick/hindemith/wiki/",
      author_email="mjanderson09@eecs.berkeley.edu",
      license="BSD",

      packages=["hindemith"],
      install_requires=[
          "numpy",
          "asp",
          "unittest2",
          "mako",
          "discover",
      ],
)

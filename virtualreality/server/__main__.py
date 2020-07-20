"""
pyvr server.

usage:
  server [options]

options:
    -h --help               shows this message
"""
from virtualreality.server import server

from . import __version__

from docopt import docopt

args = docopt(__doc__, version=__version__)

server.run_til_dead()

"""
pyvr server.

usage:
  server [--show-messages]

options:
    -h --help               shows this message
    -d --show-messages      show messages

"""
from virtualreality.server import server

from . import __version__

from docopt import docopt

args = docopt(__doc__, version=__version__)

server.PRINT_MESSAGES = args["--show-messages"]

server.run_til_dead()

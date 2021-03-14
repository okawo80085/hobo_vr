"""
pyvr server.

usage:
  server [--show-messages]

options:
    -h --help               shows this message
    -d --show-messages      show messages

"""
from . import server

from . import __version__

from docopt import docopt

args = docopt(__doc__, version=__version__)

my_server = server.Server()
my_server.debug = args["--show-messages"]

server.run_til_dead(conn_handle=my_server)

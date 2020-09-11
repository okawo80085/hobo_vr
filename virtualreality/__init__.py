"""Virtual reality tools and drivers for python."""
from . import util
from . import templates

from .logging import log

logger = log.setup_custom_logger(name=__name__, level="INFO", file="./logs/app.log")

logger.debug("created app.log log file")

__version__ = "0.0.6"

"""Virtualreality Pose Server"""
from . import server
from ..logging import log

__version__ = "0.1"

logger = log.setup_custom_logger(name=__name__, 
    level="INFO",
    file="../logs/server.log",
    console_logging=True)

logger.debug("created server.log log file")
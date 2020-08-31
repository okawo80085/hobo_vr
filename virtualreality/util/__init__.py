"""Virtualreality Utility Module"""
from ..logging import log

logger = log.setup_custom_logger(name=__name__, level="INFO", file="../logs/util.log")

logger.debug("created util.log log file")

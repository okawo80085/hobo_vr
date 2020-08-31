"""Calibration functions."""
from ..logging import log

logger = log.setup_custom_logger(
    name=__name__, level="INFO", file="../logs/calibration.log", console_logging=False
)

logger.debug("created calibration.log log file")

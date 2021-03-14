"""Headset and controller tracers."""
from ..logging import log

logger = log.setup_custom_logger(
    name=__name__, level="INFO", file="../logs/trackers.log"
)

logger.debug("created trackers.log log file")

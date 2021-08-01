# (c) 2021 Okawo
# This code is licensed under MIT license (see LICENSE for details)

"""Headset and controller tracers."""
from ..logging import log

logger = log.setup_custom_logger(
    name=__name__, level="INFO", file="../logs/trackers.log"
)

logger.debug("created trackers.log log file")

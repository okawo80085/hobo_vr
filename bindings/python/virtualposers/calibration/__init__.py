# (c) 2021 Okawo
# This code is licensed under MIT license (see LICENSE for details)

"""Calibration functions."""
from ..logging import log

logger = log.setup_custom_logger(
    name=__name__, level="INFO", file="../logs/calibration.log", console_logging=False
)

logger.debug("created calibration.log log file")

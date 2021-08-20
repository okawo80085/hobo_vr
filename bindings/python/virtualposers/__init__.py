# (c) 2021 Okawo
# This code is licensed under MIT license (see LICENSE for details)

"""Virtual reality tools and drivers for python."""
try:
    from . import util

    from .logging import log

    logger = log.setup_custom_logger(
        name=__name__, level="INFO", file="./logs/app.log"
    )

    logger.debug("created app.log log file")

except Exception as e:
    print(f'failed to load submodules, reason: {e}')

__version__ = "0.2.0"

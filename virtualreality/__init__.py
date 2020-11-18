"""Virtual reality tools and drivers for python."""
try:
	from . import util
	from . import templates

	from .logging import log

	logger = log.setup_custom_logger(name=__name__, level="INFO", file="./logs/app.log")

	logger.debug("created app.log log file")

except Exception as e:
	print (f'failed to load submodules, reason: {e}')

__version__ = "0.1.2"

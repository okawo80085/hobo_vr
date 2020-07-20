"""Headset and controller tracers."""
import logging
import os

logger = logging.getLogger(__name__)

#logfile
log_path = os.path.join(os.path.dirname(__file__), "../logs/server.log")
logger_handler = logging.FileHandler(log_path)
logger_handler.setLevel(logging.DEBUG)

#console logging
console_logger_handler = logging.StreamHandler()
console_logger_handler.setLevel(logging.INFO)

#logging entry format
logger_formatter = logging.Formatter('[%(asctime)s] %(name)s %(levelname)s - %(message)s')

logger_handler.setFormatter(logger_formatter)
console_logger_handler.setFormatter(logger_formatter)

logger.addHandler(logger_handler)
logger.addHandler(console_logger_handler)
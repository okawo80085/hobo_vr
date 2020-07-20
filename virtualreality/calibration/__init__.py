"""Calibration functions."""
import logging
import os

logger = logging.getLogger(__file__)
logger.setLevel(logging.INFO)

log_file = os.path.join(os.path.dirname(__file__), "../logs/calibration.log")
logger_handler = logging.FileHandler(log_file)

logger_formatter = logging.Formatter('[%(asctime)s] %(name)s %(levelname)s - %(message)s')

logger_handler.setFormatter(logger_formatter)
logger.addHandler(logger_handler)
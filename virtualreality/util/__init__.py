import logging
import os

logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)

#logfile
log_path = os.path.join(os.path.dirname(__file__), "../logs/util.log")
logger_handler = logging.FileHandler(log_path)
logger_handler.setLevel(logging.INFO)

#logging entry format
logger_formatter = logging.Formatter('[%(asctime)s] %(name)s %(levelname)s - %(message)s')
logger_handler.setFormatter(logger_formatter)

logger.addHandler(logger_handler)
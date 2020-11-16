import logging
import os
import stat
import enum


class log_levels(enum.Enum):
    DEBUG = logging.DEBUG
    INFO = logging.INFO
    WARNING = logging.WARNING
    ERROR = logging.ERROR
    CRITICAL = logging.CRITICAL


def setup_custom_logger(
    name,
    level,
    file,
    format="[%(asctime)s] %(name)s %(levelname)s - %(message)s",
    console_logging=False,
):
    logger = logging.getLogger(name)
    logger.setLevel(level)

    log_path = os.path.normpath(os.path.join(os.path.dirname(__file__), file))
    dir_p = os.path.dirname(log_path)
    if not os.path.exists(dir_p):
        os.mkdir(dir_p)

    # logging entry format
    logger_formatter = logging.Formatter(
        "[%(asctime)s] %(name)s %(levelname)s - %(message)s"
    )

    # if i don't this bullshit, it creates root owned log files in some cases
    os.chmod(dir_p, stat.S_IRWXU | stat.S_IRWXG | stat.S_IRWXO)
    if not os.path.exists(log_path):
        with open(log_path, 'wt') as f:
            f.write('fuck you')
    os.chmod(log_path, stat.S_IRWXU | stat.S_IRWXG | stat.S_IRWXO)

    logger_handler = logging.FileHandler(log_path)
    logger_handler.setLevel(level)
    logger_handler.setFormatter(logger_formatter)
    logger.addHandler(logger_handler)

    if console_logging:
        console_logger_handler = logging.StreamHandler()
        console_logger_handler.setLevel(level)
        console_logger_handler.setFormatter(logger_formatter)
        logger.addHandler(console_logger_handler)

    return logger

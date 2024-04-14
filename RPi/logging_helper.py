import logging
import logging.handlers
from collections.abc import Iterable
from typing import Union
import os
from config import TEST

# change working directory to here
abspath = os.path.abspath(__file__)
dname = os.path.dirname(abspath) # directory of this file
del abspath

class LoggingLevelFilter(logging.Filter):
    def __init__(self, logging_levels: Union[int, Iterable[int]]):
        super().__init__()
        if isinstance(logging_levels, Iterable):
            self.logging_levels = logging_levels
        else:
            self.logging_levels = (logging_levels,)

    def filter(self, record: logging.LogRecord) -> bool:
        return any(record.levelno == l for l in self.logging_levels)

logger = logging.getLogger(__name__)
logger.setLevel(logging.DEBUG)
formatter_file = logging.Formatter(f'%(levelname)s: %(asctime)s > module: %(module)s > func_name: %(funcName)s > line_nr: %(lineno)d -> %(message)s')
formatter_print = logging.Formatter(f'%(levelname)s: %(asctime)s > module: %(module)s > func_name: %(funcName)s > line_nr: %(lineno)d -> %(message)s')

fname_debug = os.path.join(dname, 'log', 'labino.debug.log')
fname_info = os.path.join(dname, 'log', 'labino.info.log')
fname_error = os.path.join(dname, 'log', 'labino.err.log')

for fname_ in (fname_debug, fname_info, fname_error):
    dirname = os.path.dirname(os.path.abspath(fname_))
    if not os.path.isdir(dirname):
        os.makedirs(dirname)

rotating_handler_info = logging.handlers.RotatingFileHandler(filename=fname_info, mode='a', maxBytes=5*1024*1024, backupCount=10)
rotating_handler_info.setFormatter(formatter_file)
rotating_handler_info.addFilter(LoggingLevelFilter((logging.INFO, logging.WARNING)))
logger.addHandler(rotating_handler_info)

rotating_handler_debug = logging.handlers.RotatingFileHandler(filename=fname_debug, mode='a', maxBytes=5*1024*1024, backupCount=10)
rotating_handler_debug.setFormatter(formatter_file)
rotating_handler_debug.addFilter(LoggingLevelFilter(logging.DEBUG))
logger.addHandler(rotating_handler_debug)

rotating_handler_error = logging.handlers.RotatingFileHandler(filename=fname_error, mode='a', maxBytes=5*1024*1024, backupCount=10)
rotating_handler_error.setFormatter(formatter_file)
rotating_handler_error.addFilter(LoggingLevelFilter((logging.ERROR, logging.CRITICAL)))
logger.addHandler(rotating_handler_error)

stream_handler = logging.StreamHandler()
stream_handler.setFormatter(formatter_print)
if not TEST:
    stream_handler.addFilter(LoggingLevelFilter((logging.INFO, logging.WARNING, logging.ERROR, logging.CRITICAL)))
else:
    stream_handler.addFilter(LoggingLevelFilter((logging.WARNING, logging.ERROR, logging.CRITICAL)))
logger.addHandler(stream_handler)

def debug(msg: str, *args) -> None:
    logger.debug(msg, *args)

def info(msg: str, *args) -> None:
    logger.info(msg, *args)

def warning(msg: str, *args) -> None:
    logger.warning(msg, *args)

def error(msg: str, *args) -> None:
    logger.error(msg, *args)

def critical(msg: str, *args) -> None:
    logger.critical(msg, *args)

def exception(msg: str, *args) -> None:
    logger.exception(msg, *args)

def log(level: int, msg: str, *args) -> None:
    logger.log(level, msg, *args)

import sys
from typing import Type, Optional
from types import TracebackType
from traceback import format_exception

def _handle_unhandled_exception(exc_type: Type[BaseException], exc_value: BaseException, exc_traceback: Optional[TracebackType]):
    if issubclass(exc_type, KeyboardInterrupt):
        # Will call default excepthook
        sys.__excepthook__(exc_type, exc_value, exc_traceback)
    else:
        # Create a critical level log message with info from the except hook.
        msg = ''.join(format_exception(exc_type, exc_value, exc_traceback))
        logger.critical(msg)
sys.excepthook = _handle_unhandled_exception
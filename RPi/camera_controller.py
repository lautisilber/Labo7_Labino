from __future__ import annotations
from config import TEST
if not TEST:
    from picamera import PiCamera
else:
    from dummy_classes.dummy_picamera import PiCamera
from typing import Optional, Tuple, Union, Dict, Any
import os
from sched import scheduler
import subprocess
import threading
from dataclasses import dataclass
from collections import deque
from datetime import datetime
import re

from logging_helper import logger as lh

class CameraController:
    # https://picamera.readthedocs.io/en/release-1.13/api_camera.html
    recording = False
    converting = False
    
    @dataclass(frozen=True)
    class ConversionTarget:
        source: str
        output: str
        overwrite: bool=True
        timeout: Union[float, int]=60
        framerate: int=30
        args: Tuple[Any,...]=tuple()

    def __init__(self, save_dir: str='media', n_files: int=50, resolution: Tuple[int, int]=(640, 480), framerate: int=30) -> None:
        self.save_dir = save_dir
        self.n_files = n_files

        self.camera = PiCamera()
        self.camera.framerate = framerate
        self.camera.resolution = resolution
        self.camera.vflip = False
        
        self.conversion_queue: deque[CameraController.ConversionTarget] = deque(maxlen=30)
        
        self.last_conversion_path: Optional[str] = None
        self.files_re = re.compile('([0-9])*_([0-9])*_([0-9])*__([0-9])*_([0-9])*_([0-9])*(_[0-9]*)?.mp4')

    @property
    def framerate(self) -> int:
        return self.camera.framerate

    @property
    def resolution(self) -> int:
        return self.camera.resolution

    def __del__(self) -> None:
        if CameraController.recording:
            self.camera.stop_recording()

    def start_recording(self) -> bool:
        if CameraController.recording: return False

        path = os.path.join(self.save_dir, datetime.now().strftime('%Y_%m_%d__%H_%M_%S') + '.h264')

        # create dir if it doesnt exist
        if not os.path.isdir(self.save_dir):
            os.makedirs(self.save_dir)

        # dont overwrite files and numeric extensions
        i = 1
        while os.path.isfile(path):
            old_ending = f'_{i-1}'
            new_ending = f'_{i}'
            base = os.path.basename(path)
            fname, ext = os.path.splitext(base)
            if i == 1:
                fname += new_ending
            elif fname.endswith(old_ending):
                fname = fname[:-len(old_ending)]
                fname += new_ending
            path = os.path.join(self.save_dir, fname+ext)
            i += 1

        self.last_conversion_path = path
        CameraController.recording = True
        self.camera.start_recording(path)
        lh.debug(f'Started recording to file {path}')
        return True

    def stop_recording(self) -> None:
        if not CameraController.recording: return
        self.camera.stop_recording()
        CameraController.recording = False
        lh.debug(f'Stopped recording to file {self.last_conversion_path}')
        base, ext = os.path.splitext(self.last_conversion_path)
        new_path = base + '.mp4'
        
        files = [os.path.join(self.save_dir, f) for f in os.listdir(self.save_dir) if os.path.isfile(os.path.join(self.save_dir, f)) and bool(self.files_re.fullmatch(f))]
        files_to_delete = len(files)+1 - self.n_files
        if files_to_delete:
            files.sort()
            files = files[:files_to_delete]
            for f in files:
                os.remove(f)
        
        self._add_conversion_to_queue(CameraController.ConversionTarget(source=self.last_conversion_path, output=new_path))

    def schedule_start_recording(self, delay_s: Union[float, int]) -> None:
        if delay_s <= 0:
            raise ValueError()
        s = scheduler()
        s.enter(delay_s, priority=10, action=self.start_recording)
        s.run(blocking=False) # TODO: Test if this works

    def schedule_stop_recording(self, delay_s: Union[float, int]) -> None:
        if delay_s <= 0:
            raise ValueError()
        s = scheduler()
        s.enter(delay_s, priority=9, action=self.stop_recording)
        s.run(blocking=False)

    def record_for_time(self, length_s: float) -> None:
        self.start_recording()
        self.schedule_stop_recording(length_s)

    def _add_conversion_to_queue(self, ct: CameraController.ConversionTarget) -> bool:
        if len(self.conversion_queue) >= self.conversion_queue.maxlen: return False
        self.conversion_queue.appendleft(ct)
    
    def _h264_to_mp4(self, ct: CameraController.ConversionTarget) -> bool:
        if CameraController.converting: return False
        args = ['ffmpeg', '-i', ct.source, '-r', f'{ct.framerate}', '-c', 'copy', ct.output]
        if ct.overwrite:
            args.append('-y')
        if ct.args:
            args.extend(ct.args)
        
        CameraController.converting = True
        p = subprocess.Popen(args, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        lh.debug(f'begin converting')
        def cb():
            try:
                outs, errs = p.communicate(timeout=ct.timeout)
            except subprocess.TimeoutExpired:
                p.kill()
                outs, errs = p.communicate()
            CameraController.converting = False
            lh.debug(f'end converting')
            os.remove(ct.source)
            # TODO: error handling
            
        t = threading.Thread(target=cb)
        t.run()
        return True
        
    def tick(self) -> None:
        if (not CameraController.converting) and self.conversion_queue:
            s = self._h264_to_mp4(self.conversion_queue[-1])
            if s:
                self.conversion_queue.pop()
    
    def tick_until_done(self) -> None:
        while (not CameraController.converting) and self.conversion_queue:
            self.tick()


if __name__ == '__main__':
    from time import sleep
    cc = CameraController('media', 3)
    cc.start_recording()
    cc.schedule_stop_recording(5)
    sleep(6)
    cc.tick_until_done()

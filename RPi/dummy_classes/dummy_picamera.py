from typing import Optional

class PiCamera:
    def __init__(self, framerate: Optional[int]=None, resolution: Optional[tuple[int, int]]=None, vflip: Optional[bool]=None) -> None:
        self.framerate = framerate if framerate is not None else 30
        self.resolution = resolution if resolution is not None else (640, 480)
        self.vflip = vflip if vflip is not None else False
    
    def start_recording(self, path: str):
        pass

    def stop_recording(self):
        pass
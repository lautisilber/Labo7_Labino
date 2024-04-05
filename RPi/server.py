import socketserver
import socket
# from socketserver import _AfInetAddress
from typing import Callable, Any, Self, Union, Tuple
import threading

import os
if hasattr(os, "fork"):
    server_mixin = socketserver.ForkingMixIn
else:
    server_mixin = socketserver.ThreadingMixIn

class TCPHandler(socketserver.StreamRequestHandler):
    def handle(self) -> None:
        for line in self.rfile.readlines():
            print(line)
        self.wfile.write('OK')

class ThreadedTCPServer(server_mixin, socketserver.TCPServer):
    def __init__(self, RequestHandlerClass: Callable[[Any, Any, Self], socketserver.BaseRequestHandler], host: str='127.0.0.1', port: int=9090, bind_and_activate: bool=True) -> None:
        server_address = (host, port)
        super().__init__(server_address, RequestHandlerClass, bind_and_activate)

    def __del__(self) -> None:
        self.shutdown()
        super().__del__()


if __name__ == '__main__':
    def client(ip, port, message):
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
            sock.connect((ip, port))
            sock.sendall(bytes(message, 'ascii'))
            response = str(sock.recv(1024), 'ascii')
            print("Received: {}".format(response))

    with ThreadedTCPServer(TCPHandler, port=9091) as server:
        ip, port = server.server_address

        # Start a thread with the server -- that thread will then start one
        # more thread for each request
        server_thread = threading.Thread(target=server.serve_forever)
        # Exit the server thread when the main thread terminates
        server_thread.daemon = True
        server_thread.start()
        print("Server loop running in thread:", server_thread.name)

        client(ip, port, "Hello World 1")
        client(ip, port, "Hello World 2")
        client(ip, port, "Hello World 3")

        server.shutdown()


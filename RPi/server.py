from http.server import ThreadingHTTPServer, BaseHTTPRequestHandler
from socketserver import BaseRequestHandler
import socket
from typing import Callable, Any, Union
import threading
try:
    from typing import Self
except ImportError:
    from typing_extensions import Self


class RequestHandlerHandler(BaseHTTPRequestHandler):
    '''
    The idea is that one can inherit from this request handler
    One can add methods with the following naming scheme: "route_<name>_<method>"
    where name is the name of the route (if underscores are present they will be treated
    as sub directories of the url), and method is the request method: GET, POST, etc
    '''
    def handle(self) -> None:
        if hasattr(self, 'headers'):
            request_length = int(self.headers.getheader('Content-Length'))
            with self.rfile as f:
                a = f.read(request_length)
            data = a.decode('ascii')
        else:
            data = str(self.request.recv(1024), 'ascii')
        self.send_response(200, data)


if __name__ == '__main__':
    def client(ip: str, port: int, message: str):
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
            sock.connect((ip, port))
            sock.sendall(bytes(message, 'ascii'))
            response = str(sock.recv(1024), 'ascii')
            print("Received: {}".format(response))

    HOST, PORT = "localhost", 2000
    with ThreadingHTTPServer((HOST, PORT), RequestHandlerHandler) as server:

        # Start a thread with the server -- that thread will then start one
        # more thread for each request
        server_thread = threading.Thread(target=server.serve_forever)
        # Exit the server thread when the main thread terminates
        server_thread.daemon = True
        server_thread.start()
        print("Server loop running in thread:", server_thread.name)

        client(HOST, PORT, "Hello World 1")
        client(HOST, PORT, "Hello World 2")
        client(HOST, PORT, "Hello World 3")

        server.shutdown()


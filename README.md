# audioserv

Minimal HTTP/WebDAV audio file server.

## build

`make all`

Produce the `audioserv` executable.

`make clean`

Remove compilation file, including the executable.

## configuration

Create a `.audioserv` directory in your home directory and `cd`. Then create a symbolic link to your music directory.

## usage

`./audioserv`

It will display the server's ip and port.

In a web browser, enter address: `http://localhost:8200`.

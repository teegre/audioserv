# audioserv

**audioserv** is a minimal HTTP/WebDAV server for serving audio files. It allows you to access your music collection via a web browser or any WebDAV-compatible client.

---

## Features

- Lightweight and simple
- Serves audio files over HTTP/WebDAV
- Easy to configure and run
- No dependencies

---

## Build

To compile the `audioserv` executable:

```sh
make all
```

To clean up all generated files:

```sh
make clean
```

---

## Installation

Copy the audioserv binary to a directory in your `$PATH`. For example:

```sh
cp audioserv ~/.local/bin/
```

---

## Configuration

### Create the configuration directory:

```sh
mkdir ~/.audioserv
cd ~/.audioserv
```

### Create a symbolic link to your music directory

```sh
ln -s /path/to/your/music music
```

**audioserv** will serve files from this linked directory.

---

## Usage

To start the server:

```sh
audioserv
```

It will display the local IP address and port it's running on.

Open a web browser or a WebDAV-compatible app and enter the display address to access your music.

### Example

```sh
$ audioserv
o audioserv version 0.1 linux/6.16.10-arch1-1
o server address: http://192.168.1.105:8200
* listening on port 8200
```

Then open that address in your browser or app.

---

## Exit

<kbd>CTRL</kbd>+<kbd>C</kbd> to close the server.

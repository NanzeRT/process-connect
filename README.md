# Process Connect

Process Connect is a C program that facilitates communication between multiple instances of the same program through shared memory. It creates pairs of processes where one acts as a server and the other as a client, transferring data from the server's `stdin` to the client's `stdout`.

## Usage

### Building the Program

Ensure you have CMake installed on your system.

1. Clone the repository:
   ```bash
   git clone https://github.com/NanzeRT/process-connect.git
   cd process-connect
   ```

2. Create a build directory and build the program:
   ```bash
   mkdir build
   cd build
   cmake ..
   make
   ```

### Running the Program

1. **Server Mode**: Launch the program without any arguments to initiate the server mode:
   ```bash
   ./process-connect
   ```

   The server mode waits for a client to connect and transfers data from its stdin to the connected client's stdout.

2. **Client Mode**: On subsequent executions, the program starts as a client:
   ```bash
   ./process-connect
   ```

   The client mode attempts to connect to an existing server. Upon successful connection, it receives data from the server and sends an "OK" signal to confirm successful connection.

3. **Repeating the Cycle**: Subsequent launches alternate between server and client modes, creating pairs of processes to continue the communication cycle.

## Example

### Server Execution

```bash
$ ./process-connect 
trying server
waiting for client
got data
sending OK
transfer successful
server: fd = 5
[user input]
server: closed fd
```

### Client Execution

```bash
$ ./process-connect 
trying server
trying client
connected
sending data
waiting for OK
received OK
client: fd = 4
[data received]
client: closing fd
```


## Running Server

1. From the directory of server, run `gcc server.c && ./a.out`
2. Wait for the client to get connected
3. Any error in client will cause the server to exit

## Running Client

1. From the directory of server, run `gcc client.c && ./a.out`
2. If the server was already running, the client will connect to it automatically
3. Any error in server will cause the client to exit

## Downloading Files

1. Make sure that the client and server are in different directories
2. Run `get [files]` in the client like `get file1 file2` to download those files from server
3. Run `exit` to exit the client (and server)

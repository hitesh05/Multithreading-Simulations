# Question 3

- Implemented client-server communication through sockets.
- Network created in the file *server.cpp*
- Dijkstra's shortest path algorithm is used to calculate the shortest paths between nodes.
- Each edge has a thread that is used to communicate with the nodes.
- The client communications are handled by the function *handle connection()*
- *send* system call:
    1. print the data as mentioned in the doc
    2. Next, we identify the edge that joins source (0) and forwarded destination (0).
    3. varibales are updated as required
    4. After printing the received data, we send the message to the edge thread, which then sends it to other edge threads along the shortest path (as determined by Dijkstra) from 0 to n.
    5. Except for the destination node, all nodes that receive the data print the data.
- Join all threads
- Tutorial code has been used extensively.

## Follow Up Question

1. In the event of a server failure, we can notify the client with the proper error message, letting them know what went wrong. Failures on the server may be caused by segfaults, timed connections, addresses that have already been utilised, etc. When a server malfunctions, we can resend the client's request.
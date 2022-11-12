#include <bits/stdc++.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <pthread.h>

/////////////////////////////
#include <iostream>
#include <assert.h>
#include <tuple>
using namespace std;
/////////////////////////////

// Regular bold text
#define BBLK "\e[1;30m"
#define BRED "\e[1;31m"
#define BGRN "\e[1;32m"
#define BYEL "\e[1;33m"
#define BBLU "\e[1;34m"
#define BMAG "\e[1;35m"
#define BCYN "\e[1;36m"
#define ANSI_RESET "\x1b[0m"

typedef long long LL;

#define pb push_back
#define debug(x) cout << #x << " : " << x << endl
#define part cout << "-----------------------------------" << endl;

///////////////////////////////
#define MAX_CLIENTS 4
// #define PORT_ARG 1111111
#define PORT_ARG 8001

const int initial_msg_len = 256;

////////////////////////////////////

const LL buff_sz = 1048576;
///////////////////////////////////////////////////

int port = 121001;

typedef struct edge_thread
{
    int index;
    int sender;
    int receiver;
    pthread_t thread;
    int sender_fd;
    int receiver_fd;
    int port_number;
} * edge;

struct temp
{
    int *index;
    edge thread;
    vector<vector<int>> v;
    vector<vector<pair<int, int>>> v1;
    vector<edge> v2;
};

pair<string, int>
read_string_from_socket(const int &fd, int bytes)
{
    std::string output;
    output.resize(bytes);

    int bytes_received = read(fd, &output[0], bytes - 1);
    debug(bytes_received);
    if (bytes_received <= 0)
    {
        cerr << "Failed to read data from socket. \n";
    }

    output[bytes_received] = 0;
    output.resize(bytes_received);
    // debug(output);
    return {output, bytes_received};
}

int send_string_on_socket(int fd, const string &s)
{
    // debug(s.length());
    int bytes_sent = write(fd, s.c_str(), s.length());
    if (bytes_sent < 0)
    {
        cerr << "Failed to SEND DATA via socket.\n";
    }

    return bytes_sent;
}

int source = 0;
int backward = 0;
int forward_dest = 0;
int destination = 0;

///////////////////////////////

void handle_connection(int client_socket_fd, vector<vector<int>> &frw, vector<vector<pair<int, int>>> &edges_index, vector<edge> &edges, vector<int> &d)
{
    // int client_socket_fd = *((int *)client_socket_fd_ptr);
    //####################################################

    int received_num, sent_num;

    /* read message from client */
    int ret_val = 1;

    while (true)
    {
        string cmd;
        tie(cmd, received_num) = read_string_from_socket(client_socket_fd, buff_sz);
        ret_val = received_num;
        // debug(ret_val);
        // printf("Read something\n");
        if (ret_val <= 0)
        {
            // perror("Error read()");
            printf("Server could not read msg sent from client\n");
            goto close_client_socket_ceremony;
        }
        cout << "Client sent : " << cmd << endl;
        if (cmd == "exit")
        {
            cout << "Exit pressed by client" << endl;
            goto close_client_socket_ceremony;
        }
        else if (cmd == "pt")
        {
            string s;
            string s1 = "dest";
            string s2 = "forw";
            string s3 = "delay";
            s += s1;
            s += " ";
            s += s2;
            s += " ";
            s += s3;
            s += "\n";

            send_string_on_socket(client_socket_fd, s);

            for (int i = 1; i < d.size(); i++)
            {
                string temp;
                temp += to_string(i);
                temp += "   ";
                temp += to_string(frw[0][i]);
                temp += "   ";
                temp += to_string(d[i]);
                temp += "\n";

                send_string_on_socket(client_socket_fd, temp);
            }
        }
        else
        {
            string token;
            int index;
            for (int i = 0; i < cmd.size(); i++)
            {
                if (cmd[i] == ' ')
                {
                    index = i;
                    for (int j = i + 2; j < cmd.size(); j++)
                    {
                        token.push_back(cmd[j]);
                    }
                    break;
                }
            }

            destination = cmd[index + 1] - '0';

            forward_dest = frw[source][destination];
            cout << "Data received at node : " << source << " ; "
                 << "Source : " << backward << " ; "
                 << "Destination : " << destination << " ; "
                 << " Forwaded_Destination : " << forward_dest << " ; "
                 << "Message : " << token << " ; " << endl;

            int edge_index;
            for (int i = 0; i < edges_index[source].size(); i++)
            {
                if (edges_index[source][i].first == forward_dest)
                {
                    edge_index = edges_index[source][i].second;
                    break;
                }
            }
            backward = source;
            source = forward_dest;
            send_string_on_socket(edges[edge_index]->sender_fd, token);

            while (source != destination)
            {
                continue;
            }
        }
        string msg_to_send_back = "Ack: " + cmd;

        ////////////////////////////////////////
        // "If the server write a message on the socket and then close it before the client's read. Will the client be able to read the message?"
        // Yes. The client will get the data that was sent before the FIN packet that closes the socket.

        int sent_to_client = send_string_on_socket(client_socket_fd, msg_to_send_back);
        // debug(sent_to_client);
        if (sent_to_client == -1)
        {
            perror("Error while writing to client. Seems socket has been closed");
            goto close_client_socket_ceremony;
        }
    }

close_client_socket_ceremony:
    close(client_socket_fd);
    printf(BRED "Disconnected from client" ANSI_RESET "\n");
    // return NULL;
}

int get_socket_fd(struct sockaddr_in *ptr, int port)
{
    struct sockaddr_in server_obj = *ptr;

    // socket() creates an endpoint for communication and returns a file
    //        descriptor that refers to that endpoint.  The file descriptor
    //        returned by a successful call will be the lowest-numbered file
    //        descriptor not currently open for the process.
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0)
    {
        perror("Error in socket creation for CLIENT");
        exit(-1);
    }
    /////////////////////////////////////////////////////////////////////////////////////
    int port_num = port;

    memset(&server_obj, 0, sizeof(server_obj)); // Zero out structure
    server_obj.sin_family = AF_INET;
    server_obj.sin_port = htons(port_num); // convert to big-endian order

    // Converts an IP address in numbers-and-dots notation into either a
    // struct in_addr or a struct in6_addr depending on whether you specify AF_INET or AF_INET6.
    // https://stackoverflow.com/a/20778887/6427607

    /////////////////////////////////////////////////////////////////////////////////////////
    /* connect to server */

    if (connect(socket_fd, (struct sockaddr *)&server_obj, sizeof(server_obj)) < 0)
    {
        perror("Problem in connecting to the server");
        exit(-1);
    }

    // part;
    //  printf(BGRN "Connected to server\n" ANSI_RESET);
    //  part;
    return socket_fd;
}

void *thread_func(void *arg)
{
    struct temp tmp = *(struct temp *)arg;

    int i, j, k, t;

    int wel_socket_fd, client_socket_fd, port_number;
    socklen_t clilen;

    struct sockaddr_in serv_addr_obj, client_addr_obj;
    /////////////////////////////////////////////////////////////////////////
    /* create socket */
    /*
    The server program must have a special door—more precisely,
    a special socket—that welcomes some initial contact
    from a client process running on an arbitrary host
    */
    // get welcoming socket
    // get ip,port
    /////////////////////////
    wel_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (wel_socket_fd < 0)
    {
        perror("ERROR creating welcoming socket");
        exit(-1);
    }

    //////////////////////////////////////////////////////////////////////
    /* IP address can be anything (INADDR_ANY) */
    bzero((char *)&serv_addr_obj, sizeof(serv_addr_obj));
    port_number = PORT_ARG;
    serv_addr_obj.sin_family = AF_INET;
    // On the server side I understand that INADDR_ANY will bind the port to all available interfaces,
    serv_addr_obj.sin_addr.s_addr = INADDR_ANY;
    serv_addr_obj.sin_port = htons(tmp.thread->port_number); // process specifies port

    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    /* bind socket to this port number on this machine */
    /*When a socket is created with socket(2), it exists in a name space
       (address family) but has no address assigned to it.  bind() assigns
       the address specified by addr to the socket referred to by the file
       descriptor wel_sock_fd.  addrlen specifies the size, in bytes, of the
       address structure pointed to by addr.  */

    // CHECK WHY THE CASTING IS REQUIRED
    if (bind(wel_socket_fd, (struct sockaddr *)&serv_addr_obj, sizeof(serv_addr_obj)) < 0)
    {
        perror("Error on bind on welcome socket: ");
        exit(-1);
    }
    //////////////////////////////////////////////////////////////////////////////////////

    /* listen for incoming connection requests */

    listen(wel_socket_fd, MAX_CLIENTS);
    clilen = sizeof(client_addr_obj);

// new 
    tmp.thread->receiver_fd = wel_socket_fd;

    struct sockaddr_in server_obj;
    int socket_fd = get_socket_fd(&server_obj, tmp.thread->port_number);

    tmp.thread->sender_fd = socket_fd;
//

    while (1)
    {
        int client_socket_fd = accept(tmp.thread->receiver_fd, (struct sockaddr *)&client_addr_obj, &clilen);
        if (client_socket_fd < 0)
        {
            perror("ERROR while accept() system call occurred in SERVER");
            exit(-1);
        }

        int received_num, sent_num;

        /* read message from client */
        int ret_val = 1;

        while (true)
        {
            string cmd;
            tie(cmd, received_num) = read_string_from_socket(client_socket_fd, buff_sz);
            ret_val = received_num;

            if (source != destination)
            {
                forward_dest = tmp.v[source][destination];
                cout << "Data received at node : " << source << " ; "
                     << "Source : " << backward << " ; "
                     << "Destination : " << destination << " ; "
                     << " Forwaded_Destination : " << forward_dest << " ; "
                     << "Message : " << cmd << " ; " << endl;

                int edge_index;
                for (int i = 0; i < tmp.v1[source].size(); i++)
                {
                    if (tmp.v1[source][i].first == forward_dest)
                    {
                        edge_index = tmp.v1[source][i].second;
                        break;
                    }
                }
                send_string_on_socket(tmp.v2[edge_index]->sender_fd, cmd);
                backward = source;
                source = forward_dest;
            }
        }
    }

    return NULL;
}

int restore_path(int s, int t, vector<int> const &p)
{
    vector<int> path;

    for (int v = t; v != s; v = p[v])
        path.push_back(v);
    path.push_back(s);

    reverse(path.begin(), path.end());
    return path[1];
}

const int INF = 1000000000;

void dijkstra(int s, vector<int> &d, vector<int> &p, vector<vector<pair<int, int>>> &adj)
{
    int n = adj.size();
    d.assign(n, INF);
    p.assign(n, -1);
    vector<bool> u(n, false);

    d[s] = 0;
    for (int i = 0; i < n; i++)
    {
        int v = -1;
        for (int j = 0; j < n; j++)
        {
            if (!u[j] && (v == -1 || d[j] < d[v]))
                v = j;
        }

        if (d[v] == INF)
            break;

        u[v] = true;
        for (auto edge : adj[v])
        {
            int to = edge.first;
            int len = edge.second;

            if (d[v] + len < d[to])
            {
                d[to] = d[v] + len;
                p[to] = v;
            }
        }
    }
}

int main(int argc, char *argv[])
{
    int n, m;
    cin >> n >> m;

    vector<edge> edges(2 * m);
    vector<vector<pair<int, int>>> edges_index(n);
    vector<vector<pair<int, int>>> adj(n);
    vector<int> d(n);
    vector<int> p(n);

    for (int i = 0; i < m; i++)
    {
        int a, b, d;
        cin >> a >> b >> d;
        int i1 = i * 2;
        int i2 = (2 * i) + 1;
        edges[i1] = (edge)malloc(sizeof(struct edge_thread));
        edges[i2] = (edge)malloc(sizeof(struct edge_thread));
        edges[i1]->index = i1;
        edges[i2]->index = i2;
        edges[i1]->receiver = a;
        edges[i2]->receiver = b;
        edges[i1]->sender = b;
        edges[i2]->sender = a;
        edges[i1]->port_number = ++port;
        edges[i2]->port_number = ++port;
        edges_index[a].push_back({b, i1});
        edges_index[b].push_back({a, i2});
        adj[a].push_back({b, d});
        adj[b].push_back({a, d});
    }

    vector<vector<int>> frw;

    for (int i = 0; i < n; i++)
    {
        vector<int> temp;
        for (int i = 0; i < n; i++)
        {
            temp.push_back(-1);
        }
        frw.push_back(temp);
    }

    for (int i = 0; i < n; i++)
    {
        dijkstra(i, d, p, adj);
        for (int j = 0; j < n; j++)
        {
            if (i != j)
            {
                frw[i][j] = restore_path(i, j, p);
            }
            else
            {
                frw[i][j] = j;
            }
        }
    }

    dijkstra(0, d, p, adj);

    int i, j, k, t;

    int wel_socket_fd, client_socket_fd, port_number;
    socklen_t clilen;

    struct sockaddr_in serv_addr_obj, client_addr_obj;
    /////////////////////////////////////////////////////////////////////////
    /* create socket */
    /*
    The server program must have a special door—more precisely,
    a special socket—that welcomes some initial contact
    from a client process running on an arbitrary host
    */
    // get welcoming socket
    // get ip,port
    /////////////////////////
    wel_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (wel_socket_fd < 0)
    {
        perror("ERROR creating welcoming socket");
        exit(-1);
    }

    //////////////////////////////////////////////////////////////////////
    /* IP address can be anything (INADDR_ANY) */
    bzero((char *)&serv_addr_obj, sizeof(serv_addr_obj));
    port_number = PORT_ARG;
    serv_addr_obj.sin_family = AF_INET;
    // On the server side I understand that INADDR_ANY will bind the port to all available interfaces,
    serv_addr_obj.sin_addr.s_addr = INADDR_ANY;
    serv_addr_obj.sin_port = htons(port_number); // process specifies port

    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    /* bind socket to this port number on this machine */
    /*When a socket is created with socket(2), it exists in a name space
       (address family) but has no address assigned to it.  bind() assigns
       the address specified by addr to the socket referred to by the file
       descriptor wel_sock_fd.  addrlen specifies the size, in bytes, of the
       address structure pointed to by addr.  */

    // CHECK WHY THE CASTING IS REQUIRED
    if (bind(wel_socket_fd, (struct sockaddr *)&serv_addr_obj, sizeof(serv_addr_obj)) < 0)
    {
        perror("Error on bind on welcome socket: ");
        exit(-1);
    }
    //////////////////////////////////////////////////////////////////////////////////////

    /* listen for incoming connection requests */

    listen(wel_socket_fd, MAX_CLIENTS);
    cout << "Server has started listening on the LISTEN PORT" << endl;
    clilen = sizeof(client_addr_obj);

    for (int i = 0; i < 2 * m; i++)
    {
        struct temp *tmp = (struct temp *)malloc(sizeof(struct temp));
        tmp->index = &i;
        tmp->thread = edges[i];
        tmp->v = frw;
        tmp->v1 = edges_index;
        tmp->v2 = edges;
        pthread_create(&edges[i]->thread, NULL, thread_func, tmp);
    }

    while (1)
    {
        /* accept a new request, create a client_socket_fd */
        /*
        During the three-way handshake, the client process knocks on the welcoming door
of the server process. When the server “hears” the knocking, it creates a new door—
more precisely, a new socket that is dedicated to that particular client.
        */
        // accept is a blocking call
        printf("Waiting for a new client to request for a connection\n");
        client_socket_fd = accept(wel_socket_fd, (struct sockaddr *)&client_addr_obj, &clilen);
        if (client_socket_fd < 0)
        {
            perror("ERROR while accept() system call occurred in SERVER");
            exit(-1);
        }

        printf(BGRN "New client connected from port number %d and IP %s \n" ANSI_RESET, ntohs(client_addr_obj.sin_port), inet_ntoa(client_addr_obj.sin_addr));

        handle_connection(client_socket_fd, frw, edges_index, edges, d);
    }

    for (int i = 0; i < 2 * m; i++)
    {
        pthread_join(edges[i]->thread, NULL);
    }

    close(wel_socket_fd);
    return 0;
}
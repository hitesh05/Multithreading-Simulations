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
#include <semaphore.h>
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
#define zero 0
#define debug(x) cout << #x << " : " << x << endl
#define part cout << "-----------------------------------" << endl;

#define pp pair<int, int>
#define vi vector<int>
#define vb vector<bool>
#define vv vector<pp>

///////////////////////////////
#define MAX_CLIENTS 4
#define PORT_ARG 1111111
// #define PORT_ARG 8001

const int initial_msg_len = 256;

////////////////////////////////////

const LL buff_sz = 1048576;
///////////////////////////////////////////////////

int port = 121001;

pthread_mutex_t mutex_lock;
sem_t semaphore;
pthread_cond_t cond;

// typedef struct edge_thread
// {
//     int index;
//     int sender;
//     int receiver;
//     pthread_t thread;
//     int sender_fd;
//     int receiver_fd;
//     int port_number;
// } * edge;

typedef struct edge
{
    pthread_t t;
    int index;
    int port_num;
    int receiver;
    int receiver_fd;
    int sender;
    int sender_fd;
    edge(int a, int b, int c, int d)
    {
        index = a;
        sender = b;
        receiver = c;
        port_num = d;
    }
} edge;

typedef struct thread_arg
{
    edge *t;
    int n;
    int m;
    vector<edge *> edge_links;
    vector<vv> edges_index;
    vector<vi> frw;
    int *index;
    thread_arg(vector<edge *> a, vector<vv> b, vector<vi> c, edge *d, int e, int f)
    {
        edge_links = a;
        edges_index = b;
        frw = c;
        t = d;
        n = e;
        m = f;
    }
} thread_st;

int source = zero;
int backward = zero;
int forward_dest = zero;
int destination = zero;

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

///////////////////////////////
void print_statement(string cmd)
{
    cout << "Data received at node : " << source << " : ";
    cout << "Source : " << backward << "; ";
    cout << "Destination : " << destination << "; ";
    cout << "Forwarded Destination : " << forward_dest << "; ";
    cout << "Message : " << cmd << endl;
    // cout << "here" << endl;
    return;
}

void handle_connection(int client_socket_fd, int n, int m, vector<vv> adj, vi d, vi p, vector<vi> frw, vector<edge *> edges, vector<vv> edges_index)
{
    // int client_socket_fd = *((int *)client_socket_fd_ptr);
    //####################################################

    int received_num, sent_num;

    /* read message from client */
    int ret_val = 1;

    while (1)
    {
        string cmd;
        tie(cmd, received_num) = read_string_from_socket(client_socket_fd, buff_sz);
        ret_val = received_num;
        string final_str = "dest forw delay\n";

        // debug(ret_val);
        // printf("Read something\n");
        string s[2];
        s[0] = "pt";
        s[1] = "send";
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
        else if (cmd[0] == s[1][0])
        {
            string empty_str = "";
            string token = empty_str;
            int index = 0;
            index--;
            int size = cmd.size();
            for (int i = 0; i < size; i++)
            {
                char emp = ' ';
                if (cmd[i] == emp)
                {
                    index = i;
                    i += 2;
                    for (int j = i; j < size; j++)
                    {
                        token += cmd[j];
                    }
                    break;
                }
            }

            index++;
            // int abc = '0';
            destination = cmd[index] - '0';

            forward_dest = frw[source][destination];
            print_statement(token);

            int edge_index;
            int size2 = edges_index[source].size();
            int y = 0;
            while (y < size2)
            {
                int checker = edges_index[source][y].first;
                if (checker == forward_dest)
                {
                    edge_index = edges_index[source][y].second;
                    break;
                }
                y++;
            }
            // cout << "edge index: " << edge_index << endl;
            backward = source;
            source = forward_dest;
            int x2 = edges[edge_index]->sender_fd;
            // cout << "x2: " << x2 << endl;
            send_string_on_socket(x2, token);

            while (source != destination)
            {
                continue;
            }
            source = zero;
            backward = zero;
            forward_dest = zero;
            destination = zero;
        }
        else if (cmd == s[0])
        {
            string empty_str = "";

            // send_string_on_socket(client_socket_fd, s);
            int y = 1;
            int size = d.size();
            while (y < size)
            {
                string temp = empty_str;
                string y_str = to_string(y);
                string frw_str = to_string(frw[0][y]);
                string d_str = to_string(d[y]);
                temp.append(y_str);
                temp.append(" ");
                temp.append(frw_str);
                temp.append(" ");
                temp.append(d_str);
                temp.append("\n");
                final_str.append(temp);
                y++;
            }
        }
        //////////////////
        ////////////////////
        string msg_to_send_back = "Ack: " + cmd;
        final_str.append(msg_to_send_back);

        ////////////////////////////////////////
        // "If the server write a message on the socket and then close it before the client's read. Will the client be able to read the message?"
        // Yes. The client will get the data that was sent before the FIN packet that closes the socket.

        int sent_to_client = send_string_on_socket(client_socket_fd, final_str);
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

void *func_thread(void *arg)
{
    // struct temp tmp = *(struct temp *)arg;
    thread_st *args = (thread_st *)arg;

    // int i, j, k, t;

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
    serv_addr_obj.sin_port = htons(args->t->port_num); // process specifies port

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
    args->t->receiver_fd = wel_socket_fd;

    struct sockaddr_in server_obj;
    int port_num = args->t->port_num;
    int socket_fd = get_socket_fd(&server_obj, port_num);

    args->t->sender_fd = socket_fd;
    //

    while (1)
    {
        int x = args->t->receiver_fd;
        int client_socket_fd = accept(x, (struct sockaddr *)&client_addr_obj, &clilen);
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
            int check = source - destination;
            if (check == 0)
            {
                ;
            }
            else
            {
                forward_dest = args->frw[source][destination];
                print_statement(cmd);

                int edge_index;
                int size = args->edges_index[source].size();
                int y = 0;
                while (y < size)
                {
                    int checker = args->edges_index[source][y].first;
                    if (checker == forward_dest)
                    {
                        edge_index = args->edges_index[source][y].second;
                        break;
                    }
                    y++;
                }
                backward = source;
                source = forward_dest;
                int x2 = args->edge_links[edge_index]->sender_fd;
                send_string_on_socket(x2, cmd);
            }
        }
    }

    return NULL;
}

int restore_path(int s, int t, vi const &p)
{
    vi path;
    int v = t - 1;
    v++;
    while (v != s)
    {
        path.push_back(v);
        v = p[v];
    }
    path.push_back(s);

    reverse(path.begin(), path.end());
    int x = 1;
    // frw[s][t] = path[x];
    return path[x];
}

const int INF = 1000000000;

void dijkstra(int s, vi &d, vi &p, vector<vector<pair<int, int>>> &adj)
{
    int n = adj.size();
    // p.assign(n, -1);
    // vector<bool> u(n, false);
    vi visited(n);
    for (int i = 0; i < n; i++)
    {
        visited[i] = 0;
        p[i] = -1;
    }
    d.assign(n, 1000000);

    int x = 0;
    d[s] = x;

    int i = 0;
    while (i < n)
    {
        int u = -1;
        int j = 0;
        while (j < n)
        {
            if (visited[j] == 0 && (u == -1 || d[j] < d[u]))
                u = j;
            j++;
        }
        if (d[u] != 1000000)
        {
            visited[u] = 1;
            for (auto x : adj[u])
            {
                if (d[u] + x.second >= d[x.first])
                {
                    ;
                }
                else
                {
                    p[x.first] = u;
                    d[x.first] = d[u] + x.second;
                }
            }
        }
        else
        {
            break;
        }
        i++;
    }
}

void sem_initialisation()
{
    sem_init(&semaphore, 0, 0);
    pthread_mutex_init(&mutex_lock, NULL);

    pthread_cond_init(&cond, NULL);
}

int main(int argc, char *argv[])
{
    int n, m;
    cin >> n >> m;

    int x = 2 * m;
    vector<edge *> edges(x);
    vector<vv> edges_index(n);
    vector<vv> adj(n);
    vi d(n);
    vi p(n);
    sem_initialisation();

    for (int i = 0; i < m; i++)
    {
        int a, b;
        int u, v, w;
        cin >> u >> v >> w;

        a = 2 * i;
        b = a + port;
        edge *e1 = new edge(a, u, v, b);
        // cout << "here1" << endl;
        edges[a] = e1;
        // cout << "here2" << endl;

        a += 1;
        b = a + port;
        edge *e2 = new edge(a, u, v, b);
        edges[a] = e2;

        a = 2 * i;
        edges_index[u].pb({v, a});
        a++;
        edges_index[v].pb({u, a});

        adj[v].pb({u, w});
        adj[u].pb({v, w});
    }

    vector<vi> frw(n, vi(n, -1));
    int y = 0;
    while (y < n)
    {
        dijkstra(y, d, p, adj);
        int z = 0;
        while (z < n)
        {
            if (y == z)
            {
                frw[y][z] = z;
            }
            else if (y != z)
            {
                frw[y][z] = restore_path(y, z, p);
            }
            z++;
        }
        y++;
    }
    y = 0;
    dijkstra(y, d, p, adj);

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

    x = 2 * m;
    y = 0;
    while (y < x)
    {
        thread_arg *arg = new thread_arg(edges, edges_index, frw, edges[y], n, m);
        pthread_create(&edges[y]->t, NULL, func_thread, (void *)arg);
        y++;
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

        handle_connection(client_socket_fd, n, m, adj, d, p, frw, edges, edges_index);
    }

    y = 0;
    x = 2 * m;
    while (y < x)
    {
        pthread_join(edges[y]->t, NULL);
        y++;
    }

    close(wel_socket_fd);
    return 0;
}
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <algorithm>
#include <fcntl.h>
#include <map>
#include <vector>
#include <iostream>
#include <sstream>
#include <thread>
#include <map>

#define SERVER_ID "V_Group_45"
#define BACKLOG  5          // Allowed length of queue of waiting connections

using namespace std;

// Simple class for handling connections from clients.
//
// Client(int socket) - socket to send/receive traffic from client.
class Client
{
  public:
    int sock;              // socket of client connection
    string name;           // Limit length of name of client's user
    string ip;
    int port;

    Client(int socket) : sock(socket){} 

    ~Client(){}            // Virtual destructor defined for base class
};

// Note: map is not necessarily the most efficient method to use here,
// especially for a server with large numbers of simulataneous connections,
// where performance is also expected to be an issue.
//
// Quite often a simple array can be used as a lookup table, 
// (indexed on socket no.) sacrificing memory for speed.

std::map<int, Client*> Servers; // Lookup table for per Client information 

// Open socket for specified port.
//
// Returns -1 if unable to create the socket for any reason.

int open_TcpSock(int portno)
{
   struct sockaddr_in sk_addr;   // address settings for bind()
   int sock;                     // socket opened for this port
   int set = 1;                  // for setsockopt

   // Create socket for connection

   if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
   {
      perror("Failed to open TCPsocket");
      return(-1);
   }
#ifdef APPLE
   fcntl(sock, F_SETFL,O_NONBLOCK);
#endif

   // Turn on SO_REUSEADDR to allow socket to be quickly reused after 
   // program exit.

   if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &set, sizeof(set)) < 0)
   {
      perror("Failed to set SO_REUSEADDR:");
   }

   memset(&sk_addr, 0, sizeof(sk_addr));

   sk_addr.sin_family      = AF_INET;
   sk_addr.sin_addr.s_addr = INADDR_ANY;
   sk_addr.sin_port        = htons(portno);

   // Bind to socket to listen for connections from clients

   if(bind(sock, (struct sockaddr *)&sk_addr, sizeof(sk_addr)) < 0)
   {
      perror("Failed to bind to socket:");
      return(-1);
   }
   else
   {
      return(sock);
   }
}

int open_UdpSock(int portno) {
    struct sockaddr_in sk_addr; // address settings for bind()
    int sock;                   // socket opened for this port
    int set = -1;               // for setsockopt

    // Create socket for connection
    if((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("Failed to open UDPsocket");
        return(-1);
    }

       // Turn on SO_REUSEADDR to allow socket to be quickly reused after 
   // program exit.

   if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &set, sizeof(set)) < 0)
   {
      perror("Failed to set SO_REUSEADDR:");
   }

   memset(&sk_addr, 0, sizeof(sk_addr));

   sk_addr.sin_family      = AF_INET;
   sk_addr.sin_addr.s_addr = INADDR_ANY;
   sk_addr.sin_port        = htons(portno);

   // Bind to socket to listen for connections from clients

   if(bind(sock, (struct sockaddr *)&sk_addr, sizeof(sk_addr)) < 0)
   {
      perror("Failed to bind to socket:");
      return(-1);
   }
   else
   {
      return(sock);
   }
}

// gets the serverid 
string getServerId() {
    string sendId = SERVER_ID;
    return sendId;
}

// Close a client's connection, remove it from the client list, and
// tidy up select sockets afterwards.

void closeClient(int clientSocket, fd_set *openSockets, int *maxfds)
{
     // Remove client from the clients list
     Servers.erase(clientSocket);

     // If this client's socket is maxfds then the next lowest
     // one has to be determined. Socket fd's can be reused by the Kernel,
     // so there aren't any nice ways to do this.

     if(*maxfds == clientSocket)
     {
        for(auto const& p : Servers)
        {
            *maxfds = std::max(*maxfds, p.second->sock);
        }
     }

     // And remove from the list of open sockets.

     FD_CLR(clientSocket, openSockets);
}

// Process command from client on the server

void Commands(int clientSocket, fd_set *openSockets, int *maxfds, 
                  char *buffer) 
{
  std::vector<std::string> tokens;
  std::string token;

  // Split command from client into tokens for parsing
  std::stringstream stream(buffer);

  while(stream >> token)
      tokens.push_back(token);

  if((tokens[0].compare("CONNECT") == 0) && (tokens.size() == 2))
  {
     Servers[clientSocket]->name = tokens[1];
  }
  else if(tokens[0].compare("LEAVE") == 0)
  {
      // Close the socket, and leave the socket handling
      // code to deal with tidying up clients etc. when
      // select() detects the OS has torn down the connection.
 
      closeClient(clientSocket, openSockets, maxfds);
  }
  else if(tokens[0].compare("WHO") == 0)
  {
     std::cout << "Who is logged on" << std::endl;
     std::string msg;

     for(auto const& names : Servers)
     {
        msg += names.second->name + ", ";

     }
     // Reducing the msg length by 1 loses the excess "," - which
     // granted is totally cheating.
     send(clientSocket, msg.c_str(), msg.length()-1, 0);

  }
  //Sends server id
  else if(tokens[0].compare("ID") == 0) {
      string msg;
      cout << "ID" << endl;
      msg += getServerId(); 

      send(clientSocket, msg.c_str(), msg.length(), 0);
  }
  // This is slightly fragile, since it's relying on the order
  // of evaluation of the if statement.
  else if((tokens[0].compare("MSG") == 0) && (tokens[1].compare("ALL") == 0))
  {
      std::string msg;
      for(auto i = tokens.begin()+2;i != tokens.end();i++) 
      {
          msg += *i + " ";
      }

      for(auto const& pair : Servers)
      {
          send(pair.second->sock, msg.c_str(), msg.length(),0);
      }
  }
  else if(tokens[0].compare("MSG") == 0)
  {
      for(auto const& pair : Servers)
      {
          if(pair.second->name.compare(tokens[1]) == 0)
          {
              std::string msg;
              for(auto i = tokens.begin()+2;i != tokens.end();i++) 
              {
                  msg += *i + " ";
              }
              send(pair.second->sock, msg.c_str(), msg.length(),0);
          }
      }
  }
  //Virkar ekki rétt
  //vantar ip og port uppls
  else if(tokens[0].compare("LISTSERVERS") == 0)
  {
      cout << "Servers connected" << std::endl;
      string msg;

     for(auto const& names : Servers)
     {
        msg += names.second->name + ",";
        msg += names.second->ip + ",";
        msg += names.second->port + ",";
        msg += ";";
     }
     send(clientSocket, msg.c_str(), msg.length(), 0);
  }
  else if(tokens[0].compare("KEEPALIVE") == 0)
  {
      //TODO
  }
  else if(tokens[0].compare("LISTROUTES") == 0)
  {
      //TODO
  }
  //CMD,<TOSERVERID>,<FROMSERVERID>,<command>
  //RSP,<TOSERVERID>,<FORMSERVERID>,<command response>
  //FETCH,<no.>
  else
  {
      std::cout << "Unknown command from client" << buffer << std::endl;
  }
}

int main(int argc, char* argv[])
{
    bool finished;
    int listenTcpSock;                 // Socket for connections to server
    int UdpSock;
    int clientSock;                 // Socket of connecting client
    fd_set openSockets;             // Current open sockets 
    fd_set readSockets;             // Socket list for select()        
    fd_set exceptSockets;           // Exception socket list
    int maxfds;                     // Passed to select() as max fd in set
    struct sockaddr_in client;
    socklen_t clientLen;
    char buffer[1025];              // buffer for reading from clients

//./tsamvgroup1 8888 8889
// 8888->TCP, 8889 ->UDP(Should only send LISTSERVERS)
    /*
    if(argc != 2)
    {
        printf("Usage: chat_server <ip port>\n");
        exit(0);
    }
    */
      if(argc <= 2)
    {
        printf("Usage: Server <TCP port> <UDP port>\n");
        exit(0);
    }

    // Setup socket for server to listen to

    listenTcpSock = open_TcpSock(atoi(argv[1])); 
    printf("Listening on tcp: %s\n", argv[1]);    

    if(listen(listenTcpSock, BACKLOG) < 0)
    {
        printf("Listen failed on port %s\n", argv[1]);
        exit(0);
    }
    else 
    // Add listen socket to socket set
    {
        FD_SET(listenTcpSock, &openSockets);
        maxfds = listenTcpSock;
    }

    UdpSock = open_UdpSock(atoi(argv[2]));
    printf("Listening on udp: %s\n", argv[2]); 

    finished = false;

    while(!finished)
    {
        // Get modifiable copy of readSockets
        readSockets = exceptSockets = openSockets;
        memset(buffer, 0, sizeof(buffer));

        int n = select(maxfds + 1, &readSockets, NULL, &exceptSockets, NULL);

        if(n < 0)
        {
            perror("select failed - closing down\n");
            finished = true;
        }
        else
        {
            // Accept  any new connections to the server
            if(FD_ISSET(listenTcpSock, &readSockets))
            {
               clientSock = accept(listenTcpSock, (struct sockaddr *)&client,
                                   &clientLen);
               FD_SET(clientSock, &openSockets);
               maxfds = std::max(maxfds, clientSock);

               Servers[clientSock] = new Client(clientSock);
               n--;

               printf("Client connected on server: %d\n", clientSock);
            }
            // Now check for commands from clients
            while(n-- > 0)
            {
               for(auto const& pair : Servers)
               {
                  Client *client = pair.second;

                  if(FD_ISSET(client->sock, &readSockets))
                  {
                      if(recv(client->sock, buffer, sizeof(buffer), MSG_DONTWAIT) == 0)
                      {
                          printf("Client closed connection: %d", client->sock);
                          close(client->sock);      

                          closeClient(client->sock, &openSockets, &maxfds);

                      }
                      else
                      {
                          std::cout << buffer << std::endl;
                          Commands(client->sock, &openSockets, &maxfds, 
                                        buffer);
                      }
                  }
               }
            }
        }
    }
}
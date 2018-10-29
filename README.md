# README
# SC-T-409-TSAM
### Project 3 - Botnet
This README is about our Botnet and with accompanying client.  
The whole system was compiled on Linux Ubuntu.

## Server commands  
ID - Server ID  
CONNECT - To join the chat, you have to connect first.  
LEAVE - To leave the server.  
WHO - Lists all the users on the server.  
MSG <usernmane> - Sends private message to user.  
MSG ALL - Sends message to all.    
LISTSERVERS - Provides a list of directly connected servers.  
KEEPALIVE - 
LISTROUTES - Provides a list of all servers in the network.  
CMD,<ToServerID>,<FromServerID>,<command> - Sends a command to another server.  
RSP,<ToServerID>,<FromServerID>,<command> - Reply to a CMD from another server.     
FETCH,<no> - Fecthes a hasded string from this server with supplied index.    

## To build
### prebuilds
xxxxx
### Server  
g++ tsamvgroup45.cpp -o tsamvgroup45 
### Client  
g++ client.cpp -o client -lpthred
## To Run  
### Server
./tsamvgroup45 <ServerPort> <InfoPort> <ClientPort>  
### Client
./client <IP> <Port>  


## Known Issues
xxxxxx

## Team Members - V_Project_3 45
Ívar Kristinn Hallsson: ivar17@ru.is  
Vilhjálmur Rúnar vilhjálmsson: vilhjalmur12@ru.is  

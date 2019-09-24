# Internet Banking System

## Server Implementation and Usage
The server first checks to see if it has received all the execution parameters, otherwise, it will return an error and the program will end.
The next step is to store the entire client database in an array structure: Client. The structure keeps the data in the file for each one
client, if it is logged on one of the sessions connected to the server and if this was blocked.
The server keeps a Session structure vector in which it holds data about each session connected, on which client was connected, if he / she performs a bank transfer, last login attempt, number of consecutive failed login attempts a card.
There are two sockets for UDP and TCP created and added to the list file-descriptor. The server will then listen until it receives information on one from file descriptors. If someone connects via TCP, a new session will be createdwhich will be added to the list of descriptive files. 

Also, to each new connection the system will instantiate a new Session-type structure to keep the information regarding that session.
If orders are received from a customer through TCP they will be processed in accordance with the requirements of the theme and will be answered to the client. If it is during a bank transfer the order received will be considered as confirmation or refusal to transfer. For each order it is checked if the session is in advance this one was connected to one of the possible clients. In case of bearing tests multiple will take into account the consecutive number of attempts on the same card, and in in case of three consecutive attempts, the card will be blocked. 

If orders are received from a customer through UDP, the customer will receive a response all through the UDP. If the command starts with "unlock" it is considered as the session it wants to unlock a card, otherwise it is considered to be the secret password
offered for unlocking the card. To keep track of simultaneous processes Unlocking the cards, a vector of Unlock structures is used
keeps the card that is desired to be unlocked, the client whose card and port corresponds to it from which came the UDP request.

## Client Implementation and Usage

The client first checks if he has received all the parameters at execution, otherwise, it will return an error and the program will end.
The next step is to create a log file to store the commands received and sent throughout the session. The two sockets are created for
UDP and TCP and are added to the list of file descriptors.

The client checks if he receives data from the keyboard and sends them further to the server via UDP or TCP. If he receives an unlock order, he will attach the last card to him who tried the login action and will send it to the server via UDP.

If from the server receives the message type IBANK> Welcome ..., the client will remember that it successfully authenticated to the server. Also, the client also keeps you the record of the last order sent, if the previous order was the transfer you will know that it is going to give the server an answer whether or not the transaction continues. In case of which the previous command was unlock, then you will know that it will send to the server through UDP secret word.

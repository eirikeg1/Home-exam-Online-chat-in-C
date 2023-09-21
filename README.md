# Home exam for the subject: IN2140 - Introduction to Operating Systems

* One program for client, and one for the server
* Implements the _Stop and Wait_ protocol for communication
* Users can block and add other users
* If users don't send a 'heartbeat' signal to the server after 30 seconds they are deleted from memory and will be registered again if they connect later.
* Implements a simplified version of the TCP protocol, using the UDP protocol
* Uses precode to simulate packet loss, the rest is implemented by me

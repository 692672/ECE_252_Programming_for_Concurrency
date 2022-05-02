# ECE 252 Assignment 6: Rogue Two

### Intended to be completed after lecture: 26


This revisits the is based on the previous "Rogue One: A Star Wars Story" assignment, in which the traitorous rebels on Scarif wished to upload the plans for the Death Star to the rebel fleet. To make this work, of course, there needs to be a receiving server-side program (the rebel fleet). You will complete the provided code to implement the behaviour described below. This approach uses `select()` or `poll()` (your choice) to allow multiple concurrent connections.

### Primary Objective
The primary objective of this exercise is to use asynchronous I/O system calls to implement a minimal server.

### Description of Behaviour

* The server will listen on for incoming connections on a given port. To reduce conflicts, choose a port number that is the last 4 digits of your student ID (so if your student ID is 12345678 then your port is 5678). **Howeverthis will not work if the last 4 digits are <= 1024, so in that case add 2000 to it**, so if your student number is 12340123 then your port is 2123. If at the time of testing your port is "busy", try a different machine or add a random number to your port ID.

* Use `select` or `poll` to wait for events (connections from clients). 
* When new connections from clients are received, the server will add it to the set of sockets being managed by `select/poll`
* Clients will send data all at once, so when a socket activates, you can receive the incoming data. 
* Check the length of received data: if it's too short, send back the message `err_inc`; if it's too long then send back `err_long`. 
* Check the data itself (compare it against `plansbuf`); if it's the same send back `msg_ok`; if not, send `err_corr`.
* Once the answer has been sent back, you can close the socket and take it out of the set being watched by `select/poll`
* To let the user know that things are still happening (the program is not frozen), the `select/poll` call should have a timeout of 5 seconds; if nothing happened print `Still listening...` to the console so that one may see that the program is still working.
* You should also add a signal handler that catches the `Ctrl-C` signal to allow for clean termination of the program.

### Testing Your Program

Use your Rogue One assignment code to test this program. If you're testing on the same machine, use the IP address of `127.0.0.1` and be sure to update the port number of the client program. You might also try to "sabotage" the program to try out all possibilities for how the server might respond.

### General Guidance 

If you're having trouble, try checking these things, and it may resolve your problem.

* Check the documentation for how functions work if you are unfamiliar with them (google is your friend!)
* Check return values (and possibly `errno`) for network functions if things are going wrong.
* Have you initialized all variables? It is easy to forget; `malloc` does not initialize the value...
* Pay attention to compiler warnings. Warnings can sometimes be ignored, but may be a hint about a semantic error that you will want to resolve. If the compiler is trying to tell you something, listen.
* You need a buffer for receiving the data, and the receive function tells you how many bytes the server sent.
* Don't forget null terminators for strings!
* Is there a missing or extra `*` (dereference) on a pointer somewhere?
* Does every memory allocation have a matching deallocation?
* It may be helpful to put `printf()` statements to follow along what the program is doing and it may help you narrow down where the issue is.

### Submitting Your Code. 
When you're done, use the command `git commit -a` to add all files to your commit and enter a commit message. Then `git push` to upload your changes to your repository. You can commit and push as many times as you like; we'll look at whatever was last pushed.

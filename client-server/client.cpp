/*
* socket-client
*
* Simple socket client implementation
* 2015 Yuzo(Huang Yuzhong)
*
* Client program arch:
* 		Master thread: Recv message from server and print to cout. Block on recv().
* 		Child thread: Read user input from cin and send to server. Block on getline().
*/

#include <iostream>
#include <thread>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <boost/format.hpp>

#ifdef _WIN32
	#define _WINSOCK_DEPRECATED_NO_WARNINGS
	#include <winsock2.h>
	#define close closesocket
	#pragma comment(lib, "ws2_32.lib")
#else
	#include <unistd.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <netdb.h>
#endif

using namespace std;
using namespace boost;

const size_t BUFFER_SIZE = 1024;

void die(const char* message);
void show_usage();
void read_input(int client_fd);

int main(int argc, char* argv[]) {
	if (argc < 3) {
		cerr << "Error: hostname port required" << endl;
		cout << format("Usage: %1% <hostname> <port>") % argv[0] << endl;
		exit(EXIT_FAILURE);
	}

#ifdef _WIN32
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		cerr << format("Error initializing winsock: %1%") % WSAGetLastError() << endl;
		exit(EXIT_FAILURE);
	}
#endif

	int client_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (client_fd < 0) {
		die("Error opening socket");
	}

	struct hostent* server = gethostbyname(argv[1]);
	if (server == NULL) {
		cerr << format("Can't resolve server: %1%") % argv[1] << endl;
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
	server_addr.sin_port = htons(atoi(argv[2]));

	if (connect(client_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
		die("Error connecting server");
	}

	thread input(read_input, client_fd);
	input.detach();

	show_usage();
	cout << format("Connected to %1%:%2%") % argv[1] % argv[2] << endl;
	char* buffer = new char[BUFFER_SIZE];

	while (true) {
		memset(buffer, 0, BUFFER_SIZE);
		int bytes = recv(client_fd, buffer, BUFFER_SIZE, 0);
		if (bytes > 0) {
			cout << '\r' << buffer << "> " << flush;
		} else {
			if (bytes == 0) {
				cout << '\r' << "Server disconnected" << endl;
				break;
			} else {
				die("Error on recv()");
			}
		}
	}

	close(client_fd);

#ifdef _WIN32
	WSACleanup();
#endif

	return EXIT_SUCCESS;
}

void die(const char* message) {
	perror(message);
	exit(EXIT_FAILURE);
}

void show_usage() {
	cout <<
		"Usage:\n"
		"\ttime\t\t\tget server time\n"
		"\tname\t\t\tget server name\n"
		"\tlist\t\t\tget client list\n"
		"\tsend <ID> <text>\tsend text to client#ID\n"
		"\tquit\t\t\tquit and bye\n\n";
}

void read_input(int client_fd) {
	char* buffer = new char[BUFFER_SIZE];

	while (true) {
		if (fgets(buffer, BUFFER_SIZE, stdin) != NULL) {
			if (strcmp(buffer, "quit\n") == 0) {
				cout << "bye~" << endl;
				exit(EXIT_SUCCESS);
			}
			if (send(client_fd, buffer, strlen(buffer), 0) < 0) {
				die("Error on send()");
			}
		} else {
			die("Error on fgets()");
		}
	}
}

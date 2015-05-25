/* socket-client
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
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <boost/format.hpp>

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
			cout << '\r' << buffer << flush;
			cout << "> " << flush;
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
		"\tsend <ID> <text>\tsend text to client@ID\n"
		"\tquit\t\t\tquit and bye\n\n";
}

void read_input(int client_fd) {
	string command;

	while (true) {
		getline(cin, command);

		if (command == "quit") {
			cout << "bye~" << endl;
			exit(EXIT_SUCCESS);
		}

		command += '\n';
		if (send(client_fd, command.c_str(), command.length(), 0) < 0) {
			die("Error on send()");
		}
	}
}

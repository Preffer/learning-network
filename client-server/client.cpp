#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

using namespace std;
const size_t BUFFER_SIZE = 1024;

void die(const char* message);

int main(int argc, char* argv[]) {
	if (argc < 3) {
		cerr << "Error: hostname port required" << endl;
		cout << "Usage: ./client <hostname> <port>" << endl;
		exit(EXIT_FAILURE);
	}

	int client_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (client_fd < 0) {
		die("Error opening socket");
	}

	struct hostent* server = gethostbyname(argv[1]);
	if (server == NULL) {
		cerr << "Can't resolve server: " << argv[1] << endl;
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
	server_addr.sin_port = htons(atoi(argv[2]));

	int result = connect(client_fd, (struct sockaddr *) &server_addr, sizeof(server_addr));
	if (result < 0) {
		die("Error connecting server");
	}

	cout << "Connected to " << argv[1] << ":" << argv[2] << endl;
	cout << "Usage: " << endl;
	cout << "\ttime\t\t\tget server time" << endl;
	cout << "\tname\t\t\tget server name" << endl;
	cout << "\tlist\t\t\tget client list" << endl;
	cout << "\tsend <id> <text>\tsend text to client@id" << endl;
	cout << "\tquit\t\t\tquit and bye" << endl;

	string command;
	char* buffer = new char[BUFFER_SIZE];

	while (true) {
		cout << "> ";
		getline(cin, command);

		if (command.find("quit") == 0) {
			cout << "bye~" << endl;
			break;
		}

		if (command.find("time") != string::npos || command.find("name") != string::npos || command.find("list") != string::npos || command.find("send") != string::npos) {
			result = send(client_fd, command.c_str(), command.length(), 0);
			if (result < 0) {
				die("Error on send()");
			}

			memset(buffer, 0, BUFFER_SIZE);
			result = recv(client_fd, buffer, BUFFER_SIZE, 0);
			if (result < 0) {
				die("Error on recv()");
			}

			cout << "Client: " << buffer << endl;
		} else {
			cout << "Invalid command" << endl;
		}
	}

	close(client_fd);
	return EXIT_SUCCESS;
}

void die(const char* message) {
	perror(message);
	exit(EXIT_FAILURE);
}

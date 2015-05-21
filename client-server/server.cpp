#include <iostream>
#include <vector>
#include <map>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <boost/format.hpp>

using namespace std;
using namespace boost;
typedef pair <char*, uint16_t> ClientInfo;
typedef map <int, ClientInfo> ClientMap;

void die(const char* message);
string onCommand(const string& command);
string onTime();
string onName();
string onList();

const size_t BUFFER_SIZE = 1024;
const int POLL_TIMEOUT = 60 * 1000;
ClientMap onlineClient;

int main(int argc, char *argv[]) {
	if (argc < 2) {
		cerr << "Error: port required" << endl;
		cout << format("Usage: %1% <port>") % argv[0] << endl;
		exit(EXIT_FAILURE);
	}

	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0) {
		die("Error opening socket");
	}

	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(atoi(argv[1]));

	int result = bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr));
	if (result < 0) {
		die("Error binding address");
	}
	
	result = listen(server_fd, SOMAXCONN);
	if (result < 0) {
		die("Error listening socket");
	}

	vector<struct pollfd> watch_fd(1);
	watch_fd[0].fd = server_fd;
	watch_fd[0].events = POLLIN;

	char* buffer = new char[BUFFER_SIZE];

	while (true) {
		cout << "Polling..." << endl;

		result = poll(watch_fd.data(), watch_fd.size(), POLL_TIMEOUT);
		if (result <= 0) {
			if (result == 0) {
				cout << "poll() time out, no event happend" << endl;
				continue;
			} else {
				die("Error on poll()");
			}
		}

		for (size_t i = 1; i < watch_fd.size(); i++) {
			if (watch_fd[i].revents == 0) {
				continue;
			} else {
				if (watch_fd[i].revents == POLLIN) {
					memset(buffer, 0, BUFFER_SIZE);

					result = recv(watch_fd[i].fd, buffer, BUFFER_SIZE, 0);
					if (result > 0) {
						string response = onCommand(buffer);
						result = send(watch_fd[i].fd, response.c_str(), response.length(), 0);

						if (result > 0) {
							watch_fd[i].revents = 0;
							continue;
						} else {
							perror("Error on send()");
						}
					} else {
						if (result == 0) {
							cout << "Client disconnected" << endl;
						} else {
							perror("Error on recv()");
						}
					}
				} else {
					cerr << "Unexpected events happend: " << watch_fd[i].revents << endl;
				}

				close(watch_fd[i].fd);
				watch_fd.erase(watch_fd.begin() + i);
				cout << "A socket closed" << endl;
			}

		}

		if (watch_fd[0].revents == POLLIN) {
			struct sockaddr_in client_addr;
			socklen_t client_addr_length =  sizeof(client_addr);

			int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_length);
			if (client_fd < 0) {
				perror("Error on accept()");
			}

			char* ip = inet_ntoa(client_addr.sin_addr);
			uint16_t port = ntohs(client_addr.sin_port);
			onlineClient[client_fd] = ClientInfo(ip, port);

			cout << format("Client connected, ID: %1%, IP: %2%, Port: %3%") % client_fd % ip % port << endl;			

			struct pollfd poll_fd;
			poll_fd.fd = client_fd;
			poll_fd.events = POLLIN;

			watch_fd.push_back(poll_fd);

			watch_fd[0].revents = 0;
		}
	}

	close(server_fd);
	return EXIT_SUCCESS; 
}

void die(const char* message) {
	perror(message);
	exit(EXIT_FAILURE);
}

string onCommand(const string& command) {
	if (command == "time") {
		return onTime();
	}
	if (command == "name") {
		return onName();
	}
	if (command == "list") {
		return onList();
	}

	return "Invalid Command";
}

string onTime() {
	time_t now = time(NULL);
	return ctime(&now);
}

string onName() {
	char hostname[128];
	gethostname(hostname, 127);
	return hostname;
}

string onList() {
	string response;
	for (auto client : onlineClient) {
		response.append((format("ID: %1%,\tIP: %2%,\tPort: %3%\n") % client.first % client.second.first % client.second.second).str());
	}
	return response;
}

#include <iostream>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <netinet/in.h>

using namespace std;
const size_t BUFFER_SIZE = 1024;
const int POLL_TIMEOUT = 60 * 1000;

void die(const char *message);

int main(int argc, char *argv[]) {
	if (argc < 2) {
		cerr << "Error: port required" << endl;
		cerr << "Usage: ./server <port>" << endl;
		return EXIT_FAILURE;
	}

	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0) {
		die("Error on opening socket");
	}

	struct sockaddr_in server_addr, client_addr;

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(atoi(argv[1]));

	char* buffer = new char[BUFFER_SIZE];
	int res;

	res = bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr));
	if (res < 0) {
		die("Error on binding");
	}
	
	res = listen(server_fd, SOMAXCONN);
	if (res < 0) {
		die("Error on listening");
	}

	vector<struct pollfd> watch_fd(1);
	watch_fd[0].fd = server_fd;
	watch_fd[0].events = POLLIN;

	while (true) {
		cout << "polling..." << endl;

		res = poll(watch_fd.data(), watch_fd.size(), POLL_TIMEOUT);
		if (res < 0) {
			die("Error on poll()");
		}

		if (res == 0) {
			cout << "poll() timed out, nothing happend" << endl;
			continue;
		}

		for (size_t i = 1; i < watch_fd.size(); i++) {
			if (watch_fd[i].revents == 0) {
				continue;
			} else {
				if (watch_fd[i].revents == POLLIN) {
					memset(buffer, 0, BUFFER_SIZE);

					res = recv(watch_fd[i].fd, buffer, BUFFER_SIZE, 0);
					if (res > 0) {
						cout << "Message: " << buffer << " Length: " << res << endl;

						string response = "Recv: ";
						response += buffer;

						res = send(watch_fd[i].fd, response.c_str(), response.length(), 0);
						if (res > 0) {
							watch_fd[i].revents = 0;
							continue;
						} else {
							perror("Error on send()");
						}
					} else {
						if (res == 0) {
							cout << "Close connection" << endl;
						} else {
							perror("Error on recv()");
						}
					}
				} else {
					cerr << "Unexpected events happend" << endl;
					cout << watch_fd[i].revents << endl;
				}

				close(watch_fd[i].fd);
				watch_fd.erase(watch_fd.begin() + i);
			}

		}

		if (watch_fd[0].revents == POLLIN) {
			socklen_t client_addr_length =  sizeof(client_addr);
			int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_length);
			if (client_fd < 0) {
				perror("Error on accept()");
			}

			struct pollfd new_fd;
			new_fd.fd = client_fd;
			new_fd.events = POLLIN;

			watch_fd.push_back(new_fd);

			watch_fd[0].revents = 0;
		}
	}

	close(server_fd);
	return EXIT_SUCCESS; 
}

void die(const char *message) {
	perror(message);
	exit(EXIT_FAILURE);
}

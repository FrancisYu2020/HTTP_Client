/*
** client.c -- a stream socket client demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <string>
#include <iostream>
#include <arpa/inet.h>
#include <fstream>

using namespace std;

#define MAXDATASIZE 1030 // max number of bytes we can get at once 

typedef struct URL {
	string protocol = "HTTP/1.1";
	string hostname;
	string port = "80";
	string path;
	int invalid;
} URL;

string CleanString(const char* url_raw) {
	// wget allows space in the input url, so we first skip the spaces
	// in the urls
	string s = "";
	while(url_raw && *url_raw) {
		if (*url_raw == ' ') url_raw ++;
		else {
			s += *url_raw;
			url_raw ++;
		}
	}
	return s;
}

bool IsValidProtocol(string *url_ptr) {
    string url = *url_ptr;
	for (int i=0; i < url.length() - 3; i++) {
		if (!url.compare(i, 3, "://")) {
			if (i != 4 || url.compare(0, 7, "http://")) {
                return 0;
            } else {
                *url_ptr = (*url_ptr).substr(7);
                // cout << *url_ptr << endl;
                break;
            }
		}
	}
	return 1;
}

URL* ParseURL(string url) {
	URL* ret = new URL();

	if (!IsValidProtocol(&url)) {
        ret->invalid = 1;
    }

    int slash_pos = url.find('/');
    int colon_pos = url.find(':');
    if (slash_pos > url.length()) {
        ret->hostname = url;
        return ret;
    }

    if (url.find(':') < url.length()) {
        int colon_pos = url.find(':');
        ret->hostname = url.substr(0, colon_pos);
        ret->port = url.substr(colon_pos + 1, slash_pos - colon_pos - 1);
    } else {
        ret->hostname = url.substr(0, slash_pos);
    }
    ret->path = url.substr(slash_pos);
	return ret;
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
	string clean_url = CleanString(argv[1]); // TODO: finish this later, this does not actually handle when the url has space in command line
	URL* info = ParseURL(clean_url);
	if (info->invalid) {
		ofstream output;
		output.open("output");
		output << "INVALIDPROTOCOL";
		output.close();
		exit(1);
	}

	ofstream output; //create output file
	output.open("output", ios::out | ios::binary);
	int sockfd, numbytes;  
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];

	if (argc != 2) {
	    fprintf(stderr,"usage: client hostname\n");
		output.close();
	    exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(info->hostname.c_str(), (info->port).c_str(), &hints, &servinfo)) != 0) {
		output.close();
		output.open("output");
		output << "NOCONNECTION";
		output.close();
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("client: connect");
			continue;
		}

		break;
	}

	if (p == NULL) {
		output.close();
		output.open("output");
		output << "NOCONNECTION";
		output.close();
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
	printf("client: connecting to %s\n", s);

	freeaddrinfo(servinfo); // all done with this structure

	// send the GET request here
	string tmp = "GET " + info->path + " " + info->protocol + "\r\nHost: " + info->hostname + "\r\n\r\n";
	char *msg = const_cast<char*>(tmp.c_str());
	int len = strlen(msg);
	if ((numbytes = send(sockfd, msg, len, 0)) == -1) {
		output.close();
		perror("send");
		exit(1);
	}

	int http_response = 1;
	int total_received = 0;
	int expected_number;
	if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
		output.close();
	    perror("recv");
	    exit(1);
	}
	buf[numbytes] = '\0';
	cout << buf << endl;
	if (http_response) {
		// this code snippet handles when the buffer is still reading the header
		// cout << buf << endl;
		if (strstr(buf, "404")) {
			//only when we find 404 code in the first line will we say filenotfound
			output.close();
			output.open("output");
			output << "FILENOTFOUND";
			output.close();
			close(sockfd);
			delete(info);
			return 0;
			// break;
		}
		char* length = strstr(buf, "Content-Length: ");
		length += strlen("Content-Length: ");
		char* length_end = strstr(length, "\n");
		*length_end = '\0';
		expected_number = atoi(length);
		*length_end = '\n';

		// first_line = 0;
		if (char *body = strstr(buf, "\r\n\r\n")) {
			http_response = 0;
            // cout << body + 4 << "   " << strlen(body + 4) << "  " << numbytes << endl;
			int counter = 0;
			char* t = buf + numbytes;
			while (t != (body + 4)) {
				counter ++;
				t --;
			}
			output.write(body + 4, counter);
			total_received += counter;
		} else {
			output.close();
			delete(info);
			close(sockfd);
			perror("No body found!");
			exit(1);
		}
		// continue;
	}
	cout << total_received << endl;
	while (total_received < expected_number)
	{
		if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
			output.close();
		    perror("recv");
		    exit(1);
		}
		// cout << numbytes << endl;

		buf[numbytes] = '\0';
		int old_total = total_received;
		total_received += numbytes;
		if (total_received > expected_number) {
			cout << total_received << " vs. " << expected_number << endl;
			output.write(buf, expected_number - old_total);
		}
		else output.write(buf, numbytes);
		// break;
	}
	
    cout << "total received bytes = " << total_received << endl;
	close(sockfd);
	
	delete(info);

	output.close();

	return 0;
}


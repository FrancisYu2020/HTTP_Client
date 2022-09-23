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

#define MAXDATASIZE 512 // max number of bytes we can get at once 

typedef struct URL {
	std::string protocol = "HTTP/1.1";
	std::string hostname;
	std::string port = "80";
	std::string path;
	int invalid;
} URL;

std::string CleanString(const char* url_raw) {
	// wget allows space in the input url, so we first skip the spaces
	// in the urls
	std::string s = "";
	while(url_raw && *url_raw) {
		if (*url_raw == ' ') url_raw ++;
		else {
			s += *url_raw;
			url_raw ++;
		}
	}
	return s;
}

bool IsValidProtocol(std::string *url_ptr) {
    std::string url = *url_ptr;
	for (int i=0; i < url.length() - 3; i++) {
		if (!url.compare(i, 3, "://")) {
			if (i != 4 || url.compare(0, 7, "http://")) {
                return 0;
            } else {
                *url_ptr = (*url_ptr).substr(7);
                cout << *url_ptr << endl;
                break;
            }
		}
	}
	return 1;
}

URL* ParseURL(std::string url) {
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

void HandleError(FILE* fp, const char* msg, int socketfd, URL* info) {
	close(socketfd);
	if (info) delete(info);
	fclose(fp);
	fp = fopen("output", "wb");
	int ret = fwrite(msg, 1, strlen(msg), fp);
	if (ret < strlen(msg)) {
		perror("error when write message error message");
	}
	fclose(fp);
}

void HandleUnexpectedError(FILE* fp, const char* msg, int socketfd, URL* info) {
	//This handles unexpected errors that are not mentioned in the MP instruction
	close(socketfd);
	if (info) delete(info);
	fclose(fp);
	perror(msg);
	exit(1);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void printer(const char* msg) {std::cout << *msg << std::endl;}
void int_printer(int x) { std::cout << x << std::endl; }


int main(int argc, char *argv[])
{
	std::string clean_url = CleanString(argv[1]); // TODO: finish this later, this does not actually handle when the url has space in command line
	URL* info = ParseURL(clean_url);
	if (info->invalid) {
		ofstream output;
		output.open("output");
		output << "INVALIDPROTOCOL";
		output.close();
		exit(1);
	}

	// ofstream output; //create output file
	FILE* fp;
	fp = fopen("output", "wb");
	int sockfd, numbytes;  
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];

	if (argc != 2) {
	    fprintf(stderr,"usage: client hostname\n");
		fclose(fp);
	    exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(info->hostname.c_str(), (info->port).c_str(), &hints, &servinfo)) != 0) {
		HandleError(fp, "NOCONNECTION", sockfd, info);
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

		// cout << sockfd << endl;

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("client: connect");
			continue;
		}

		break;
	}

	if (p == NULL) {
		HandleError(fp, "NOCONNECTION", sockfd, info);
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
	printf("client: connecting to %s\n", s);

	freeaddrinfo(servinfo); // all done with this structure

	// send the GET request here
	std::string tmp = "GET " + info->path + " " + info->protocol + "\r\nHost: " + info->hostname + "\r\n\r\n";
	// cout << tmp << endl;
	char *msg = const_cast<char*>(tmp.c_str());
	int len = strlen(msg);
	if ((numbytes = send(sockfd, msg, len, 0)) == -1) {
		fclose(fp);
		perror("send");
		exit(1);
	}

        // cout << numbytes << tmp.length() << endl;
	// int http_response = 1;
	int first_line = 1;
	int total_received = 0;

	if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
		fclose(fp);
	    perror("recv");
	    exit(1);
	}
	buf[numbytes] = '\0';
	if (strstr(buf, "404")) HandleError(fp, "FILENOTFOUND", sockfd, info);
	char* length;
	if ((length = strstr(buf, "Content-Length: "))) length = length + strlen("Content-Length: ");
	char* length_end = strstr(length, "\n");
	if (!length_end) {
		fclose(fp);
		perror("Failed to get the content length!");
		exit(1);
	}
	char t = *length_end;
	// printer(length);
	*length_end = '\0';
	// printer(length);
	int expected_length = atoi(length);
	*length_end = t;

	if (char *body = strstr(buf, "\r\n\r\n")) {
		int_printer(strlen(buf));
		int_printer(strlen(body));
		int body_pos;
		char *tmp1 = buf;
		while (tmp1 != (body + 4)) {
			body_pos ++;
			tmp1 ++;
		}
		int_printer(body_pos);
		fwrite(buf, 1, numbytes - body_pos + 1, fp);
		total_received += numbytes - body_pos + 1;
	} else HandleUnexpectedError(fp, "Failed to get the end of the header!", sockfd, info);

	while (total_received < expected_length)
	{
		// std::cout << total_received << "    " << expected_length<< endl;
		if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
			// output.close();
			fclose(fp);
		    perror("recv");
		    exit(1);
		}
		// cout << numbytes << endl;

		buf[numbytes] = '\0';
		total_received += numbytes;
		// if (!strlen(buf)) break;

		fwrite(buf, 1, numbytes, fp);
		// break;
	}
	
        cout << "total received bytes = " << total_received << endl;
	close(sockfd);
	
	delete(info);

	// output.close();
	fclose(fp);

	return 0;
}


/**
 * ---------------------------------------------------------------------------
 * Salam Programming Language
 * ---------------------------------------------------------------------------
 *
 * Welcome to the Salam Programming Language! Salam is a modern, efficient,
 * and developer-friendly language created to inspire and empower programmers
 * of all ages. With its unique features and Persian roots, Salam aims to make
 * programming more accessible and enjoyable for everyone.
 *
 * This file is part of the core implementation of Salam, including its runtime
 * and compiler components. By contributing to or using this codebase, you are
 * part of a growing community dedicated to innovation and inclusivity in
 * programming.
 *
 * Explore Salam further:
 * - Website: https://www.salamlang.ir/
 * - GitHub: https://github.com/SalamLang/Salam
 *
 * Thank you for being part of this journey!
 * ---------------------------------------------------------------------------
 */

#include "downloader.h"

/**
 *
 * @function parse_url
 * @brief Parse URL
 * @params {const char*} url
 * @params {char*} hostname
 * @params {char*} path
 * @returns {void}
 *
 */

void parse_url(const char *url, char *hostname, char *path) {
    DEBUG_ME;
    const char *start;
    const char *end;

    if (strncmp(url, "https://", 8) == 0) {
        fprintf(stderr, "Error: HTTPS protocol is not supported.\n");
        exit(1);
    }

    if (strncmp(url, "http://", 7) == 0) {
        start = url + 7;
    } else {
        start = url;
    }

    end = strchr(start, '/');
    if (end != NULL) {
        strncpy(hostname, start, end - start);
        hostname[end - start] = '\0';
        strcpy(path, end);
    } else {
        strcpy(hostname, start);
        path[0] = '/';
        path[1] = '\0';
    }
}

/**
 *
 * @function download
 * @brief Download a file from a HTTP URL
 * @params {FILE*} fp - File pointer
 * @params {const char*} port - Port
 * @params {const char*} hostname - Hostname
 * @params {const char*} path - Path
 * @returns {bool} - True if the download was successful, false otherwise
 *
 */

bool download(FILE *fp, const char *port, const char *hostname,
              const char *path) {
    DEBUG_ME;
#define BUFFER_SIZE 1024

    char buffer[BUFFER_SIZE];

    size_t request_size = 1024;
    char *request = (char *)malloc(request_size);

    if (!request) {
        fprintf(stderr, "Memory allocation failed for request\n");
        return false;
    }

    size_t written =
        snprintf(request, request_size, "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n",
                 path, hostname);

    if (written >= request_size) {
        request_size = written + 1;
        request = (char *)realloc(request, request_size);

        if (!request) {
            fprintf(stderr, "Memory reallocation failed for request\n");
            return false;
        }

        snprintf(request, request_size, "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n",
                 path, hostname);
    }

    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int bytes_received;

    memset_s(&hints, sizeof(hints), 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(hostname, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        free(request);
        return false;
    }

    for (p = servinfo; p != NULL; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

        if (sockfd == -1) {
            perror("socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            perror("connect");
            CLOSE_SOCKET(sockfd);
            continue;
        }
        break;
    }

    if (p == NULL) {
        fprintf(stderr, "Failed to connect.\n");
        free(request);
        return false;
    }

    freeaddrinfo(servinfo);

    if (send(sockfd, request, strlen(request), 0) == -1) {
        perror("send");
        free(request);
        CLOSE_SOCKET(sockfd);
        return false;
    } else if (!fp) {
        perror("fopen");
        free(request);
        CLOSE_SOCKET(sockfd);
        return false;
    }

    int header_found = 0;
    while ((bytes_received = recv(sockfd, buffer, BUFFER_SIZE, 0)) > 0) {
        if (!header_found) {
            char *header_end = strstr(buffer, "\r\n\r\n");

            if (header_end) {
                size_t header_length = header_end - buffer + 4;
                header_found = 1;

                fwrite(header_end + 4, 1, bytes_received - header_length, fp);
            }
        } else {
            fwrite(buffer, 1, bytes_received, fp);
        }
    }

    if (bytes_received == -1) {
        perror("recv");
        free(request);
        return false;
    }

    fclose(fp);
    free(request);
    CLOSE_SOCKET(sockfd);

    return true;
}

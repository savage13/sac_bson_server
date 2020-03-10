#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <signal.h>
#include <ctype.h>
#include <bson/bson.h>

//char *j = bson_as_canonical_extended_json (arr, NULL);
//debug("Commands:\n %s\n", j);

#define DEBUG 0

#define debug(fmt, ...) \
            do { if (DEBUG) fprintf(stderr, fmt, ##__VA_ARGS__); } while (0)

void zombie_killer(int v) {
    pid_t kidpid;
    int status;

    debug("ZOMBIE: killer: %d is SIGCHLD: %d\n", v, v == SIGCHLD);
    while ((kidpid = waitpid(-1, &status, WNOHANG)) > 0) {
         debug("ZOMBIE: Child %d terminated\n", kidpid);
    }
    debug("ZOMBIE: Cleanup Done\n");
}

void
setup_signal() {
    struct sigaction sa;
    sigfillset(&sa.sa_mask);
    sa.sa_handler = zombie_killer;
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa, NULL);
}

void
rstrip(char *s) {
    char *p = s + strlen(s) - 1;
    while(p > s && isspace(*p)) {
        *p = 0;
        p--;
    }
}

char *
get_kv(const uint8_t *data, size_t n, char *s, size_t s_n) {
    memset(s, 0, s_n);
    memcpy(s, data + (70 + 40) * 4 + n * 8, 8);
    rstrip(s);
    return s;
}

const char **
bson_array_to_strings(bson_iter_t *iter, size_t *n) {
    uint32_t len = 0;
    const uint8_t *array = NULL;
    // Get Array data as raw
    bson_iter_array(iter, &len, &array);
    // Construct a new bson from the raw data
    bson_t * arr = bson_new_from_data(array, len);
    // Get the number of keys of the array (its a dict/hash/...)
    uint32_t alen = bson_count_keys(arr);
    debug("ARRAY COUNT: %d \n", alen);
    bson_iter_t aiter;
    bson_iter_init(&aiter, arr);

    const char ** out = calloc(alen, sizeof(char *));
    for(uint32_t i = 0; i < alen; i++) {
        char tmp[16];
        uint32_t len = 0;
        snprintf(tmp, sizeof(tmp), "%d", i);
        bson_iter_find(&aiter, tmp);
        out[i] = bson_iter_dup_utf8(&aiter, &len);
        debug("  %d/%d: %s\n", i, alen, out[i]);
    }
    *n = alen;
    return out;

}

typedef struct {
    const uint8_t *data;
    uint32_t n;
} raw;

raw *
bson_array_to_raw(bson_iter_t *iter, size_t *n) {
    uint32_t len = 0;
    const uint8_t *array = NULL;
    // Get Array data as raw
    bson_iter_array(iter, &len, &array);
    // Construct a new bson from the raw data
    bson_t * arr = bson_new_from_data(array, len);
    // Get the number of keys of the array (its a dict/hash/...)
    uint32_t alen = bson_count_keys(arr);
    debug("ARRAY COUNT: %d \n", alen);
    bson_iter_t aiter;
    bson_iter_init(&aiter, arr);

    raw * out = calloc(alen, sizeof(raw ));
    for(uint32_t i = 0; i < alen; i++) {
        char tmp[16];
        bson_subtype_t btype = 0;
        snprintf(tmp, sizeof(tmp), "%d", i);
        bson_iter_find(&aiter, tmp);
        bson_iter_binary(&aiter, &btype, &out[i].n, &out[i].data);
    }
    *n = alen;
    return out;
}

int
bson_data(const uint8_t *data, size_t len) {
    bson_t *b;

    // Convert raw data to BSON
    b = bson_new_from_data (data, len);
    if (!b) {
        fprintf (stderr,
                 "The specified length embedded in <my_data> did not match "
                 "<my_data_len>\n");
        return -1;
    }
    bson_iter_t baz;
    bson_iter_t iter;
    // Form an iterator to search for keys in the BSON data
    if (bson_iter_init(&iter, b)) {

        // DATA Section [ sac-file, sac-file ]
        if(bson_iter_find_descendant (&iter, "data", &baz) &&
           BSON_ITER_HOLDS_ARRAY (&baz)) {
            // Grab all data in the data section
            size_t n = 0;
            raw *out = bson_array_to_raw(&baz, &n);
            for(size_t i = 0; i < n; i++) {
                const uint8_t *data = out[i].data;

                // Simple way to prove the data was transferred
                float *delta = (float *) data;
                char sta[9],net[9],loc[9],cha[9];

                get_kv(data, 0,  sta, sizeof(sta));
                get_kv(data, 3,  loc, sizeof(loc));
                get_kv(data, 20, cha, sizeof(cha));
                get_kv(data, 21, net, sizeof(net));

                printf("FILE: %s.%s.%s.%s delta: %f\n", sta, net, loc, cha, *delta);
            }
        }
        // PROCESS Section [ commmand, command ]
        if (bson_iter_find_descendant (&iter, "process", &baz) &&
            BSON_ITER_HOLDS_ARRAY (&baz)) {
            // Grab all commands in the process section
            size_t n = 0;
            debug("GOT ARRAY\n");
            const char **out = bson_array_to_strings(&baz, &n);
            for(size_t i = 0; i < n; i++) {
                printf("  [%3zu/%3zu]: %s\n", i,n,out[i]);
            }
        }
    }

    bson_destroy (b);
    return 0;
}

int
worker(int connfd, struct sockaddr_in client_addr) {
    //time_t ticks;
    char sendBuff[1025];
    memset(sendBuff, 0, sizeof(sendBuff)); 

    // Convert Address into readable string
    char *ip = inet_ntoa(client_addr.sin_addr);
    debug("CHILD:  Conneciton accepted: %s\n", ip);
    //ticks = time(NULL);

    // Send OK\r\n
    snprintf(sendBuff, sizeof(sendBuff), "OK\r\n");
    write(connfd, sendBuff, strlen(sendBuff));

    // Read "Size" from Client, convert to actual number
    memset(sendBuff, 0, sizeof(sendBuff));
    debug("READING %s\n", ip);
    int n = read(connfd, sendBuff, sizeof(sendBuff));
    debug("BYTES: %s\n", sendBuff);
    sscanf(sendBuff, "%d\r\n", &n);
    debug("EXPECTING: %d bytes\n", n);

    // Send OK\r\n
    snprintf(sendBuff, sizeof(sendBuff), "OK\r\n");
    write(connfd, sendBuff, strlen(sendBuff));

    // Read Data (length n) from Client
    //  An extra 2 bytes are the \r\n
    // Allocate space for the data
    uint8_t *data = calloc(n, sizeof(uint8_t));
    uint8_t tmp[1024]; // buffer for reading
    int nbytes = 0;
    while (nbytes < n) {
        // Read a buffer of bytes
        debug("READING data: %d %d\n", n, nbytes);
        int m = read(connfd, tmp, sizeof(tmp));
        debug("  GOT %d\n", m);
        // Copy bytes onto the storage array
        memcpy(data + nbytes, tmp, m);
        nbytes += m;
    }
    // Process data
    if(nbytes == n){
        bson_data(data, nbytes-2);
    }

    // At this point we might want to send data back
    //   with an error code in BSON

    // Close Connection
    close(connfd);
    debug("CHILD:  Turing into a ZOMBIE\n");

    return 0;
}

int main() {

    int listenfd = 0, connfd = 0;
    struct sockaddr_in serv_addr;
    struct sockaddr_in client_addr;
    socklen_t address_len = sizeof client_addr;
    pid_t pid;

    // Setup SIGCHLD to run waitpid on Terminated children
    setup_signal();

    // Open the Socket
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, '0', sizeof(serv_addr));

    // Accept Connections on port 5555
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(5555); 

    bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)); 

    // Listen for Connections
    listen(listenfd, 10);
    while(1) {

        // Accept Connections
        connfd = accept(listenfd, (struct sockaddr*)&client_addr, &address_len);

        // Connection established - Fork child and do something
        debug("PARENT: Connection accepted \n");
        if ((pid = fork()) == -1) {
            // Error
            debug("PARENT: Fork error\n");
            close(connfd);
        } else if(pid > 0) {
            // PID returns to the parent process
            close(connfd);
            debug("PARENT: Forked %d\n", pid);
        } else if (pid == 0) {
            // Child Laborer
            return worker(connfd, client_addr);
        }
    }
    return 0;
}

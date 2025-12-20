#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

#define DEFAULT_CLIENTS 1
#define DEFAULT_HOST "127.0.0.1"
#define DEFAULT_PORT 5000
#define DEFAULT_PIPE 1

struct bench_conf {
    long total_ops;
    int key_count;
    int clients;
    int pipeline;
    const char *host;
    int port;
};

struct kv_pair {
    char *key;
    char *value;
    int sz;
};

typedef struct {
    int id;
    int ops;
    int pipeline;
    int cmd;
    struct kv_pair* pairs;
    const char* host;
    int port;
    int key_count;
} client_arg_t;

static double now_sec(void) {
    struct timespec ts;
    clock_gettime(1, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

static void die(const char* mssg){
    perror(mssg);
    exit(EXIT_FAILURE);
}

static ssize_t write_full(int fd, const void *buf, size_t len) {
    size_t off = 0;
    while (off < len) {
        ssize_t n = write(fd, (char *)buf + off, len - off);
        if (n <= 0) die("write");
        off += n;
    }
    return off;
}

static ssize_t read_full(int fd, void *buf, size_t len) {
    size_t off = 0;
    while (off < len) {
        ssize_t n = read(fd, (char *)buf + off, len - off);
        if (n <= 0) die("read");
        off += n;
    }
    return off;
}

static void print_help(const char *prog) {
    printf(
        "Usage: %s [options]\n"
        "\n"
        "Options:\n"
        "  -n <ops>        Total number of operations (required)\n"
        "  -k <keys>       Number of unique keys (default: ops/5)\n"
        "  -c <clients>    Number of clients (default: 1)\n"
        "  -p <pipeline>   Pipeline depth (default: 1)\n"
        "  --host <host>   Server address (default: 127.0.0.1)\n"
        "  --port <port>   Server port (default: 5000)\n"
        "  -h              Show this help\n"
        "\n",
        prog
    );
}

static void parse_args(int argc, char **argv, struct bench_conf *conf) {
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-n") && i + 1 < argc) {
            conf->total_ops = atol(argv[++i]);
        }
        else if (!strcmp(argv[i], "-k") && i + 1 < argc) {
            conf->key_count = atol(argv[++i]);
        }
        else if (!strcmp(argv[i], "-c") && i + 1 < argc) {
            conf->clients = atol(argv[++i]);
        }
        else if (!strcmp(argv[i], "-p") && i + 1 < argc) {
            conf->pipeline = atoi(argv[++i]);
        }
        else if (!strcmp(argv[i], "--host") && i + 1 < argc) {
            conf->host = argv[++i];
        }
        else if (!strcmp(argv[i], "--port") && i + 1 < argc) {
            conf->port = atoi(argv[++i]);
        }
        else if (!strcmp(argv[i], "-h")) {
            print_help(argv[0]);
            exit(EXIT_SUCCESS);
        }
        else {
            fprintf(stderr, "Unknown or invalid argument: %s\n", argv[i]);
            print_help(argv[0]);
            exit(EXIT_FAILURE);
        }
    }
}

static struct kv_pair *generate_kv_pairs(long count) {
    struct kv_pair *pairs = calloc(count, sizeof(struct kv_pair));
    if (!pairs) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    for (long i = 0; i < count; i++) {
        char keybuf[64];
        char valbuf[64];

        snprintf(keybuf, sizeof(keybuf), "key:%ld", i + 1);
        snprintf(valbuf, sizeof(valbuf), "val:%ld", i + 1);

        pairs[i].key = strdup(keybuf);
        pairs[i].value = strdup(valbuf);
        pairs[i].sz = strlen(keybuf);

        if (!pairs[i].key || !pairs[i].value) {
            perror("strdup");
            exit(EXIT_FAILURE);
        }
    }

    return pairs;
}

static void free_kv_pairs(struct kv_pair *pairs, long count) {
    for (long i = 0; i < count; i++) {
        free(pairs[i].key);
        free(pairs[i].value);
    }
    free(pairs);
}

static inline int rand_key(int key_count) {
    return rand() % key_count;
}

static size_t build_ping(uint8_t *buf) {
    uint8_t *p = buf;

    uint32_t nstr = 1;
    memcpy(p, &nstr, 4); p += 4;
    uint32_t tmp = 4;
    memcpy(p, &tmp, 4); p += 4;
    memcpy(p, "PING", 3); p += 4;

    return p - buf;
}

static size_t build_set(uint8_t *buf, int key_id, struct kv_pair *pairs) {
    uint8_t *p = buf;

    uint32_t nstr = 3;
    memcpy(p, &nstr, 4); p += 4;
    uint32_t tmp = 3;
    memcpy(p, &tmp, 4); p += 4;
    memcpy(p, "SET", 3); p += 3;

    uint32_t klen = pairs[key_id].sz;

    memcpy(p, &klen, 4); p += 4;
    memcpy(p, pairs[key_id].key, klen); p += klen;

    memcpy(p, &klen, 4); p += 4;
    memcpy(p, pairs[key_id].value, klen); p += klen;

    return p - buf;
}

static size_t build_get(uint8_t *buf, int key_id, struct kv_pair *pairs) {
    uint8_t *p = buf;

    uint32_t nstr = 2;
    memcpy(p, &nstr, 4); p += 4;
    uint32_t tmp = 3;
    memcpy(p, &tmp, 4); p += 4;
    memcpy(p, "GET", 3); p += 3;

    uint32_t klen = pairs[key_id].sz;
    memcpy(p, &klen, 4); p += 4;
    memcpy(p, pairs[key_id].key, klen); p += klen;

    return p - buf;
}

static size_t build_del(uint8_t *buf, int key_id, struct kv_pair *pairs) {
    uint8_t *p = buf;

    uint32_t nstr = 2;
    memcpy(p, &nstr, 4); p += 4;
    uint32_t tmp = 3;
    memcpy(p, &tmp, 4); p += 4;
    memcpy(p, "DEL", 3); p += 3;

    uint32_t klen = pairs[key_id].sz;
    memcpy(p, &klen, 4); p += 4;
    memcpy(p, pairs[key_id].key, klen); p += klen;

    return p - buf;
}

enum ramora_cmd {
    CMD_PING = 0,
    CMD_GET = 1,
    CMD_SET = 2,
    CMD_DEL = 3
};

static size_t build_cmd(uint8_t *buf, int cmd, int key_id, struct kv_pair *pairs) {
    switch (cmd) {
        case CMD_PING: return build_ping(buf);
        case CMD_SET: return build_set(buf, key_id, pairs);
        case CMD_GET: return build_get(buf, key_id, pairs);
        case CMD_DEL: return build_del(buf, key_id, pairs);
        default:
            return 0;
    }
}

static void read_reply(int fd, int* counter) {
    uint8_t rc;
    uint32_t cnt;

    read_full(fd, &rc, 1);
    read_full(fd, &cnt, 4);

    if (rc == 0){
        *counter += 1;
    }

    if (cnt == 1) {
        uint8_t tag;
        uint32_t len;
        read_full(fd, &tag, 1);
        read_full(fd, &len, 4);
        char tmp[256];
        read_full(fd, tmp, len);
    }
}

void *client_thread(void *arg) {
    client_arg_t *cfg = arg;

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) die("socket");

    struct sockaddr_in sa = {
        .sin_family = AF_INET,
        .sin_port = htons(cfg->port),
    };
    inet_pton(AF_INET, cfg->host, &sa.sin_addr);

    if (connect(fd, (struct sockaddr *)&sa, sizeof(sa)) < 0)
        die("connect");
    
    int sent = 0;
    uint8_t* wbuf = malloc(cfg->pipeline * 1024);

    int* counter = malloc(sizeof(int));

    while (sent < cfg->ops) {
        int batch = cfg->pipeline;
        if (sent + batch > cfg->ops)
            batch = cfg->ops - sent;

        size_t len = 0;
        for (int i = 0; i < batch; i++) {
            size_t off;
            off = build_cmd((wbuf + len), cfg->cmd, rand() % cfg->key_count, cfg->pairs);
            len += off;
        }
        write_full(fd, wbuf, len);

        for (int i = 0; i < batch; i++) {
            read_reply(fd, counter);
        }

        sent += batch;
    }

    close(fd);
    return counter;
}

int main(int argc, char **argv) {
    struct bench_conf conf = {
        .total_ops  = 0,
        .key_count = -1,
        .clients = DEFAULT_CLIENTS,
        .pipeline    = DEFAULT_PIPE,
        .host        = DEFAULT_HOST,
        .port        = DEFAULT_PORT
    };

    parse_args(argc, argv, &conf);

    if (conf.total_ops <= 0) {
        fprintf(stderr, "Error: -n <ops> must be > 0\n");
        exit(EXIT_FAILURE);
    }
    if (conf.key_count != -1 && conf.key_count <= 0) {
        fprintf(stderr, "Error: -k <keys> must be > 0\n");
        exit(EXIT_FAILURE);
    }
    if (conf.key_count == -1){
        conf.key_count = conf.total_ops / 5;
        if (conf.key_count == 0){
            conf.key_count = 1;
        }
    }

    int cmd;
    printf("Operation type (PING: %d; GET: %d; SET: %d; DEL: %d): ", CMD_PING, CMD_GET, CMD_SET, CMD_DEL);
    scanf("%d", &cmd);
    if (cmd < 0 || cmd > 3){
        printf("Incorrect Command\n");
        exit(EXIT_FAILURE);
    }

    struct kv_pair *pairs = generate_kv_pairs(conf.key_count);

    printf("Ramora test config:\n");
    printf("  ops        : %ld\n", conf.total_ops);
    printf("  clients    : %d\n", conf.clients);
    printf("  pipeline   : %d\n", conf.pipeline);
    printf("  host       : %s\n", conf.host);
    printf("  port       : %d\n", conf.port);
    printf("  keys       : %d\n", conf.key_count);

    pthread_t threads[conf.clients];
    client_arg_t args[conf.clients];

    int ops_per_client = conf.total_ops / conf.clients;

    double t0 = now_sec();

    for (int i = 0; i < conf.clients; i++) {
        args[i] = (client_arg_t){
            .id = i,
            .ops = ops_per_client,
            .pipeline = conf.pipeline,
            .cmd = cmd,
            .pairs = pairs,
            .host = conf.host,
            .port = conf.port,
            .key_count = conf.key_count
        };
        pthread_create(&threads[i], NULL, client_thread, &args[i]);
    }

    int tot = 0;
    for (int i = 0; i < conf.clients; i++){
        void* ret;
        pthread_join(threads[i], &ret);
        int* value = (int*)ret;
        tot += *value;
    }

    double t1 = now_sec();

    double secs = t1 - t0;
    double rps  = conf.total_ops / secs;

    printf("\n=== RESULTS ===\n");
    printf("Total ops: %ld\n", conf.total_ops);
    printf("Time: %.3f sec\n", secs);
    printf("Throughput: %.1f ops/sec\n", rps);

    if (cmd == CMD_GET){
        printf("\nNon Nil Output: %d\n", tot);
    }
    else if (cmd == CMD_SET){
        printf("\nSuccessful Set Count: %d\n", tot);
    }
    else {
        printf("\nSuccessful Del Count: %d\n", tot);
    }

    free_kv_pairs(pairs, conf.key_count);
    return 0;
}

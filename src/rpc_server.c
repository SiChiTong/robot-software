#include <lwip/api.h>
#include <simplerpc/service_call.h>
#include <serial-datagram/serial_datagram.h>

#define RPC_SERVER_STACKSIZE 2048
#define RPC_SERVER_PORT 20001
THD_WORKING_AREA(wa_rpc_server, RPC_SERVER_STACKSIZE);

static uint8_t input_buffer[1024];
static uint8_t output_buffer[1024];

static bool method_called;
static size_t output_bytes_written;

static void ping_cb(int argc, cmp_ctx_t *input, cmp_ctx_t *output)
{
    (void) argc;
    (void) input;
    cmp_write_str(output, "pong", 4);
}

service_call_method service_call_callbacks[] = {
    {.name = "ping", .cb = ping_cb}
};


/* Callback fired when a serial datagram is received via TCP. */
static void serial_datagram_recv_cb(const void *data, size_t len, void *arg)
{
    (void) arg;
    method_called = true;

    output_bytes_written = service_call_process(data, len, output_buffer, sizeof output_buffer,
                                                service_call_callbacks,
                                                sizeof(service_call_callbacks) / sizeof (service_call_callbacks[0]));

}

msg_t rpc_server_thread(void *p)
{
    struct netconn *conn, *client_conn;
    int error;

    (void)p;

    struct netbuf *buf;
    char *data;
    u16_t len;
    err_t err;

    serial_datagram_rcv_handler_t handler;

    chRegSetThreadName("rpc_server");

    /* Creates a TCP server */
    conn = netconn_new(NETCONN_TCP);
    netconn_bind(conn, IP_ADDR_ANY, RPC_SERVER_PORT);
    netconn_listen(conn);

    while (1) {
        serial_datagram_rcv_handler_init(&handler,
                                         input_buffer, sizeof input_buffer,
                                         serial_datagram_recv_cb,
                                         NULL);

        error = netconn_accept(conn, &client_conn);

        if (error != ERR_OK) {
            continue;
        }

        /* method_called will be set to true once a callback is fired. */
        method_called = false;
        output_bytes_written = 0;

        while (!method_called) {
            /* Tries to receive something from the connection. */
            err = netconn_recv(client_conn, &buf);

            /* If connection closed early */
            if (err != ERR_OK) {
                break;
            }

            do {
                netbuf_data(buf, (void **)&data, &len);
                err = serial_datagram_receive(&handler, data, len);
            } while (netbuf_next(buf) >= 0);
            netbuf_delete(buf);
        }

        if (output_bytes_written > 0) {
            netconn_write(client_conn, output_buffer, output_bytes_written, NETCONN_COPY);
        }

        netconn_close(client_conn);
        netconn_delete(client_conn);
    }

    return MSG_OK;
}

void rpc_server_init(void)
{
    chThdCreateStatic(wa_rpc_server,
                      RPC_SERVER_STACKSIZE,
                      RPC_SERVER_PRIO,
                      rpc_server_thread,
                      NULL);
}
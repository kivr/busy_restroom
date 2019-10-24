//  Created by sukinull on 2/14/14.
//  Copyright (c) 2014 sukinull. All rights reserved.
//
//  Simplest websocks example using nopoll - http://www.aspl.es/nopoll/
//  remark nopoll_conn_set_sock_block (session, nopoll_false); in nopoll_conn_sock_connect under OS X
//  use nopoll_conn_set_sock_block to set sock non-block after connect
//

#include <stdio.h>
#include <openssl/ssl.h>
#include <nopoll.h>
#include <objc/objc.h>
#include <objc/message.h>

#define SDK_PAYLOAD "{\"t\":\"d\",\"d\":{\"r\":1,\"a\":\"s\",\"b\":{\"c\":{\"sdk.js.2-3-1\":1}}}}"
#define LOG_OK_PAYLOAD "{\"t\":\"d\",\"d\":{\"r\":1,\"b\":{\"s\":\"ok\",\"d\":\"\"}}}"
#define QUERY_PAYLOAD "{\"t\":\"d\",\"d\":{\"r\":3,\"a\":\"q\",\"b\":{\"p\":\"/farDoor/state\",\"h\":\"\"}}}"

typedef enum _ConnState {
    INITIAL,
    LOGGING_IN,
    WAITING
} ConnState;

static ConnState state = INITIAL;
static noPollCtx * ctx;
static noPollConn * conn;
static id appDelegate;

void msgHandler(noPollCtx  * ctx,
        noPollConn * conn,
        noPollMsg  * msg,
        noPollPtr    user_data)
{
    static void (*objc_msgSendTyped)(id self, SEL sel, const char*) = (void*)objc_msgSend;
    
    const unsigned char* m = nopoll_msg_get_payload(msg);
    int n = nopoll_msg_get_payload_size(msg);
    fprintf(stderr, "connection mesg: %s[%d]\n", m, n);

    switch(state) {
        case INITIAL:
            nopoll_conn_send_text (conn, SDK_PAYLOAD, strlen(SDK_PAYLOAD));
            state = LOGGING_IN;
            break;
        case LOGGING_IN:
            if (strncmp(m, LOG_OK_PAYLOAD, strlen(LOG_OK_PAYLOAD)) == 0) {
                nopoll_conn_send_text (conn, QUERY_PAYLOAD,
                        strlen(QUERY_PAYLOAD));
                state = WAITING;
            }
            break;
            
        case WAITING:
            objc_msgSendTyped(appDelegate, sel_getUid("messageReceived:"), m);
            break;

        default:
            break;

    }
}

static int always_true_callback(X509_STORE_CTX *ctx, void *arg)
{
    return 1;
}

SSL_CTX *ssl_context_creator(noPollCtx       * ctx,
    noPollConn      * conn,
    noPollConnOpts  * opts,
    nopoll_bool       is_client,
    noPollPtr         user_data)
{
    SSL_CTX *ssl_ctx = SSL_CTX_new (TLS_client_method());
    SSL_CTX_set_cert_verify_callback(ssl_ctx, always_true_callback, NULL);
    
    return ssl_ctx;
}

bool firebase_ping() {
    bool result;
    
    fprintf(stderr, "Ping sent\n");
    fprintf(stderr, "Conn OK: %d\n", nopoll_conn_is_ok (conn));
    
    if (nopoll_conn_is_ok(conn)) {
        nopoll_conn_send_text (conn, "0", 1);
        
        result = true;
    } else {
        result = false;
    }
    
    return result;
}

void firebase_disconnect() {
    nopoll_conn_unref(conn);
    nopoll_loop_stop (ctx);
    nopoll_ctx_unref (ctx);
}

int firebase_connect(id appDel)
{
    appDelegate = appDel;
    
    ctx = nopoll_ctx_new ();
    nopoll_ctx_set_ssl_context_creator(ctx, ssl_context_creator, NULL);
    if (! ctx) {
        // error some handling code here
        fprintf(stderr, "nopoll_ctx_new is failed\n");
        nopoll_ctx_unref (ctx);
        return 1;
    }

    //nopoll_log_enable (ctx, nopoll_true);

    // call to create a connection
    nopoll_conn_connect_timeout(ctx, 10000l);

    conn = nopoll_conn_tls_new (ctx, NULL, "busyrestroom.firebaseio.com", "443", NULL, "/.ws?ns=busyrestroom&v=5", NULL, NULL);
    if (! nopoll_conn_is_ok (conn)) {
        // some error handling here
        fprintf(stderr, "nopoll_conn_new is failed\n");
        nopoll_ctx_unref (ctx);
        return 2;
    }

    nopoll_conn_set_on_msg(conn, msgHandler, NULL);

    while(nopoll_true != nopoll_conn_is_ready(conn)) {
        fprintf(stderr, "#");
    }

    if (! nopoll_conn_is_ok (conn)) {
        printf ("ERROR: received websocket connection close during wait reply..\n");
        return nopoll_false;
    }

    nopoll_loop_wait (ctx, 0);
    nopoll_ctx_unref (ctx);

    return 0;
}

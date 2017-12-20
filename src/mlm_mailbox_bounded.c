/*  =========================================================================
    mlm_mailbox_bounded - Simple bounded mailbox engine

    Copyright (c) the Contributors as noted in the AUTHORS file.
    This file is part of the Malamute Project.

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
    =========================================================================
*/

/*
@header
    This class implements a simple bounded mailbox engine. It is built
    as CZMQ actor.
@discuss
@end
*/

#include "mlm_classes.h"

//  --------------------------------------------------------------------------
//  The self_t structure holds the state for one actor instance

typedef struct {
    zsock_t *pipe;              //  Actor command pipe
    zpoller_t *poller;          //  Socket poller
    mlm_msgq_cfg_t *queue_cfg;  //  Mailbox queue limits
    bool terminated;            //  Did caller ask us to quit?
    bool verbose;               //  Verbose logging enabled?
    zhashx_t *mailboxes;        //  Mailboxes as queues of messages
} self_t;

static void
s_self_destroy (self_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        self_t *self = *self_p;
        zpoller_destroy (&self->poller);
        zhashx_destroy (&self->mailboxes);
        mlm_msgq_cfg_destroy (&self->queue_cfg);
        free (self);
        *self_p = NULL;
    }
}

static self_t *
s_self_new (zsock_t *pipe)
{
    self_t *self = (self_t *) zmalloc (sizeof (self_t));
    if (self) {
        self->pipe = pipe;
        self->queue_cfg = mlm_msgq_cfg_new ("mlm_server/mailbox");
        if (self->queue_cfg)
            self->poller = zpoller_new (self->pipe, NULL);
        if (self->poller)
            self->mailboxes = zhashx_new ();
        if (self->mailboxes)
            zhashx_set_destructor (self->mailboxes, (czmq_destructor *) mlm_msgq_destroy);
        else
            s_self_destroy (&self);
    }
    return self;
}


//  --------------------------------------------------------------------------
//  Here we implement the mailbox API

static int
s_self_handle_command (self_t *self)
{
    char *method = zstr_recv (self->pipe);
    if (!method)
        return -1;              //  Interrupted; exit zloop
    if (self->verbose)
        zsys_debug ("mlm_mailbox_bounded: API command=%s", method);

    if (streq (method, "VERBOSE"))
        self->verbose = true;       //  Start verbose logging
    else
    if (streq (method, "$TERM"))
        self->terminated = true;    //  Shutdown the engine
    else
    if (streq (method, "CONFIGURE")) {
        mlm_msgq_cfg_t *config;
        zsock_recv (self->pipe, "p", &config);
        mlm_msgq_cfg_update (self->queue_cfg, &config);
    }
    if (streq (method, "STORE")) {
        char *address;
        mlm_msg_t *msg;
        zsock_recv (self->pipe, "sp", &address, &msg);
        mlm_msgq_t *mailbox =
            (mlm_msgq_t *) zhashx_lookup (self->mailboxes, address);
        if (!mailbox) {
            mailbox = mlm_msgq_new ();
            mlm_msgq_set_cfg (mailbox, self->queue_cfg);
            zhashx_insert (self->mailboxes, address, mailbox);
        }
        mlm_msgq_enqueue (mailbox, msg);
        zstr_free (&address);
    }
    else
    if (streq (method, "QUERY")) {
        char *address;
        mlm_msg_t *msg = NULL;
        zsock_recv (self->pipe, "s", &address);
        mlm_msgq_t *mailbox =
                (mlm_msgq_t *) zhashx_lookup (self->mailboxes, address);
        if (mailbox)
            msg = mlm_msgq_dequeue (mailbox);
        zsock_send (self->pipe, "p", msg);
        zstr_free (&address);
    }
    //  Cleanup pipe if any argument frames are still waiting to be eaten
    if (zsock_rcvmore (self->pipe)) {
        zsys_error ("mlm_mailbox_bounded: trailing API command frames (%s)", method);
        zmsg_t *more = zmsg_recv (self->pipe);
        zmsg_print (more);
        zmsg_destroy (&more);
    }
    zstr_free (&method);
    return self->terminated? -1: 0;
}


//  --------------------------------------------------------------------------
//  This method implements the mlm_mailbox actor interface

void
mlm_mailbox_bounded (zsock_t *pipe, void *args)
{
    self_t *self = s_self_new (pipe);
    //  Signal successful initialization
    zsock_signal (pipe, 0);

    while (!self->terminated) {
        zsock_t *which = (zsock_t *) zpoller_wait (self->poller, -1);
        if (which == self->pipe)
            s_self_handle_command (self);
        else
        if (zpoller_terminated (self->poller))
            break;          //  Interrupted
    }
    s_self_destroy (&self);
}


//  --------------------------------------------------------------------------
//  Selftest for mlm_mailbox_bounded, indirectly also testing mlm_msgq

static const char *words[] = {
    "anchovies ", "bacon     ", "codfish   ", "duck      ", "eggplant  ",
    "foie gras ", "gnocchi   ", "hummus    ", "ice cream ", "jalfrezi  ",
    "kimchi    ", "lobster   ", "mackerel  ", "naan      ", "octopus   ",
    "penne     ", "quiche    ", "ramen     ", "salmon    ", "trout     ",
    "udon      ", "vindaloo  ", "watermelon", "xacuti    ", "yogurt    ",
    "zucchini  "
};
static const char *sender = "Alice", *address = "Bob", *subject = "food";

// Use macros instead of functions so that a failed assert prints
// a useful line number
#define snd(str)                                             \
do {                                                         \
    zmsg_t *zmsg = zmsg_new ();                              \
    zmsg_addstr (zmsg, str);                                 \
    mlm_msg_t *msg = mlm_msg_new (sender, address, subject,  \
            "tracker", 0, zmsg);                             \
    zsock_send (mailbox, "ssp", "STORE", address, msg);      \
} while (0)

#define rcv(str)                                             \
do {                                                         \
    mlm_msg_t *msg;                                          \
    zsock_send (mailbox, "ss", "QUERY", address);            \
    zsock_recv (mailbox, "p", &msg);                         \
    if (!str) {                                              \
        assert (!msg);                                       \
        break;                                               \
    }                                                        \
    assert (msg);                                            \
    assert (streq (mlm_msg_subject (msg), subject));         \
    assert (streq (mlm_msg_address (msg), address));         \
    zmsg_t *zmsg = mlm_msg_content (msg);                    \
    char *str2 = zmsg_popstr (zmsg);                         \
    assert (streq (str2, str ? str : ""));                   \
    free (str2);                                             \
    mlm_msg_destroy (&msg);                                  \
} while (0)

void
mlm_mailbox_bounded_test (bool verbose)
{
    printf (" * mlm_mailbox_bounded: ");
    if (verbose)
        printf ("\n");

    //  @selftest
    zactor_t *mailbox = zactor_new (mlm_mailbox_bounded, NULL);
    assert (mailbox);
    if (verbose)
        zstr_sendx (mailbox, "VERBOSE", NULL);

    // Store 26*10 bytes of payload without any limit
    for (int i = 0; i < sizeof(words) / sizeof(words[0]); i++)
        snd (words[i]);
    for (int i = 0; i < sizeof(words) / sizeof(words[0]); i++)
        rcv (words[i]);

    zconfig_t *root_config, *config;
    assert ((root_config = zconfig_new ("root", NULL)));
    assert ((config = zconfig_new ("mlm_server", root_config)));
    assert ((config = zconfig_new ("mailbox", config)));
    assert ((config = zconfig_new ("size-limit", config)));
    zconfig_set_value (config, "100");
    mlm_msgq_cfg_t *mbox_config = mlm_msgq_cfg_new ("mlm_server/mailbox");
    mlm_msgq_cfg_configure  (mbox_config, root_config);
    zsock_send (mailbox, "sp", "CONFIGURE", mbox_config);

    // Store 10*10 bytes of payload, should fit exactly
    for (int i = 0; i < 10; i++)
        snd (words[i]);
    for (int i = 0; i < 10; i++)
        rcv (words[i]);

    // One message over the limit
    for (int i = 10; i < 21; i++)
        snd (words[i]);
    for (int i = 10; i < 20; i++)
        rcv (words[i]);
    rcv (NULL);

    // Changing the limit at runtime
    // store 0..9
    for (int i = 0; i < 11; i++)
        snd (words[i]);
    zconfig_set_value (config, "120");
    mbox_config = mlm_msgq_cfg_new ("mlm_server/mailbox");
    mlm_msgq_cfg_configure  (mbox_config, root_config);
    zsock_send (mailbox, "sp", "CONFIGURE", mbox_config);
    // store 11..12
    for (int i = 11; i < 15; i++)
        snd (words[i]);
    for (int i = 0; i < 10; i++)
        rcv (words[i]);
    for (int i = 11; i < 13; i++)
        rcv (words[i]);
    rcv (NULL);

    zactor_destroy (&mailbox);
    zconfig_destroy (&root_config);
    //  @end
    printf ("OK\n");
}

/*  =========================================================================
    mlm_msgq - Message queue implementation

    Copyright (c) the Contributors as noted in the AUTHORS file.
    This file is part of the Malamute Project.

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
    =========================================================================
*/

/*
@header
    mlm_msgq - Message queue implementation
@discuss
@end
*/

#include "mlm_classes.h"

//  Structure of our class

struct _mlm_msgq_t {
    zlistx_t *queue;       // qeue of mlm_msg_t objects. This must be the first
                           // member to make the iteration functions work
    size_t messages_size;  // total sizes of messages
    const size_t *size_limit;     // maximum total size of messages
};


//  --------------------------------------------------------------------------
//  Create a new mlm_msgq

mlm_msgq_t *
mlm_msgq_new (void)
{
    mlm_msgq_t *self = (mlm_msgq_t *) zmalloc (sizeof (mlm_msgq_t));
    if (!self)
        return NULL;
    self->queue = zlistx_new ();
    if (self->queue)
        zlistx_set_destructor (self->queue, (czmq_destructor *) mlm_msg_destroy);
    else
        mlm_msgq_destroy (&self);

    return self;
}


//  --------------------------------------------------------------------------
//  Destroy the mlm_msgq

void
mlm_msgq_destroy (mlm_msgq_t **self_p)
{
    assert (self_p);
    if (!*self_p)
        return;
    mlm_msgq_t *self = *self_p;
    zlistx_destroy(&self->queue);
    free (self);
    *self_p = NULL;
}

// Sets maximum total size of messages in the queue. -1 means unlimited.
void
mlm_msgq_set_size_limit(mlm_msgq_t *self, const size_t *limit)
{
    self->size_limit = limit;
}

// Enqueue a message
void
mlm_msgq_enqueue (mlm_msgq_t *self, mlm_msg_t *msg)
{
    const size_t msg_content_size =
        zmsg_content_size (mlm_msg_content (msg));
    if (!self->size_limit || *self->size_limit == (size_t) -1 ||
            self->messages_size + msg_content_size <= *self->size_limit) {
        zlistx_add_end (self->queue, msg);
        self->messages_size += msg_content_size;
    } else {
        mlm_msg_unlink (&msg);
    }
}

static mlm_msg_t *
s_dequeue (mlm_msgq_t *self, void *cursor)
{
    mlm_msg_t *res = (mlm_msg_t *) zlistx_detach (self->queue, cursor);
    if (res) {
        const size_t msg_content_size =
             zmsg_content_size (mlm_msg_content (res));
        self->messages_size -= msg_content_size;
    }
    return res;
}

// Remove the first message from the queue and return it.
mlm_msg_t *
mlm_msgq_dequeue (mlm_msgq_t *self)
{
    return s_dequeue(self, NULL);
}

// Remove the message pointed at by the cursor and return it.
mlm_msg_t *
mlm_msgq_dequeue_cursor (mlm_msgq_t *self)
{
    return s_dequeue (self, zlistx_cursor (self->queue));
}

mlm_msg_t *
mlm_msgq_first (mlm_msgq_t *self)
{
    return (mlm_msg_t *)zlistx_first (self->queue);
}

mlm_msg_t *
mlm_msgq_next (mlm_msgq_t *self)
{
    return (mlm_msg_t *)zlistx_next (self->queue);
}

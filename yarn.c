#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <pthread.h>
#include <cairo-xlib.h>
#include <string.h>

#include "yarn.h"
#include "datatypes.h"
#include "parse.h"
#include "draw.h"
#include "queue.h"
#include "cairo.h"
#include "cfg.h"

//TODO; look up thread safety things.

// pthread types.
pthread_t split_notification;
pthread_mutex_t stack_mutex = PTHREAD_MUTEX_INITIALIZER;

extern Queue queuespec;
extern Message MessageArray[QUEUESIZE];
extern Variables opt;

static bool THREAD_ALIVE = false;

void
notification_destroy(Notification *n)
{
    assert(n != NULL);

    free(n->app_name);
    free(n->app_icon);
    free(n->summary);
    free(n->body);
    free(n);
}

// Create messages on the stack.
Message
message_create(Notification *n, int textx, int texty, int x, int y, double fuse)
{
    Message m;
    char *summary = n->summary,
         *body = n->body;

    // Boop boop.
    summary = parse_strip_markup(summary);
    summary = parse_quote_markup(summary);

    body = parse_strip_markup(body);
    body = parse_quote_markup(body);

    m.summary = strdup(summary);
    m.body = strdup(body);
    m.swidth = 0;
    m.bwidth = 0;
    m.textx = textx;
    m.texty = texty;
    m.x = x;
    m.y = y;
    m.visible = 1;
    m.fuse = fuse;

    return m;
}

void *
run(void *arg)
{
    Notification *n = (Notification*) arg;

    // string, text x, text y, x, y, fuse.
    Message message = message_create(n,             // Notification struct.
                                     0,             // text x
                                     0,             // text y
                                     0,             // x
                                     0,             // y
                                     opt.timeout); // fuse
    queue_insert(&queuespec, message);
    notification_destroy(n);

    // Draw...
    draw();

    // TODO, lowercase, its not a constant...
    THREAD_ALIVE = false;

    return NULL;
}

void
prepare(Notification *n)
{
    // If there aren't any notifications being shown, we need to create a new thread.
    if (THREAD_ALIVE == false) {
        // 1 == true == error.
        if (pthread_create(&split_notification, NULL, &run, (void *)n)) {
            perror("Could not create pthread");
            return;
        } else {
            pthread_detach(split_notification);
            THREAD_ALIVE = true;
        }
    // If there are notifications being shown, simply add the new notification to the queue.
    } else {
        // Queue full, remove one first.
        if (queuespec.rear == opt.max_notifications) {
            queue_delete(&queuespec, 0);
            queue_align(queuespec);
        }

        Message message = message_create(n,
                                         0,
                                         queuespec.rear * (opt.height + opt.gap),
                                         0,
                                         queuespec.rear * (opt.height + opt.gap),
                                         opt.timeout);

        queue_insert(&queuespec, message);
        notification_destroy(n);
    }
}

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <cairo.h>

#include "datatypes.h"
#include "queue.h"
#include "cairo.h"

extern Message MessageArray[QUEUESIZE];
extern Variables opt;

/* Insert an item into the queue(s).
 * The global queue actually comprises of 2 separate arrays, but they're
 * pretty much tied to each other. */
void
queue_insert(Queue *queuespec, Message message)
{
    if (queuespec->rear == QUEUESIZE-1)
        fprintf(stderr, "Queue is full, skipped.\n");
    else
    {
        // If queue is initially empty.
        if (queuespec->front == -1)
            queuespec->front = 0;

        // Add item to array.
        // There is a new item, the end is pushed back.
        MessageArray[queuespec->rear++] = message;
    }
}

/* Delete an item from the queue. */
void
queue_delete(Queue *queuespec, int position)
{
    int i = 0;

    // Nothing in queue.
    if (queuespec->front == - 1)
        fprintf(stderr, "Queue is empty -- nothing to delete.\n");

    // Move each item down one.
    // Is there a leak?  Even though the rear is decreased it does not remove the message on the end really.
    // No, because pointers are singular -> they don't get 'copied'.
    else {
        // TODO make this cleaner.
        free(MessageArray[position].summary);
        free(MessageArray[position].body);
        for (i = position; i < queuespec->rear-1; i++)
            MessageArray[i] = MessageArray[i+1];
        queuespec->rear--;
    }
}

/* Align message coordinates to match up with the queue.
 * Could actually add this to the loop in queue_delete(), but
 * I don't think that it would really be that beneficial. */
void
queue_align (Queue queuespec)
{
    for (int i = 0; i < queuespec.rear; i++) {
        MessageArray[i].y = i * (opt.height + opt.gap);
        MessageArray[i].texty = i * (opt.height + opt.gap);
        MessageArray[i].redraw = 1;
    }
}

/* Simply return the amount of items in the queue. */
int
in_queue(Queue queuespec)
{
    return queuespec.rear;
}


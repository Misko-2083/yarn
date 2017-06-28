#include <cairo.h>
#include <cairo-xlib.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <pango/pangocairo.h>
#include <pthread.h>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <assert.h>

#include "draw.h"
#include "datatypes.h"
#include "cairo.h"
#include "x.h"
#include "parse.h"
#include "queue.h"

// Interval = 33 = 30fps.
#define INTERVAL_BASELINE 33

extern Variables opt;
extern pthread_mutex_t lock;
// Nanosleep helper.
//extern timespec req;// = {0, opt.interval*1000000};


/* The queuespec and MessageArray are global so that they can be
 * accessed easily by different threads.
 * The queue is an essential part of yarn. */
Queue queuespec = { 0, -1 };
Message MessageArray[QUEUESIZE] = { NULL };

/* Check on each message's timeouts and delete burnt ones from the queue */
void
draw_check_fuses(void)
{
    for (int i = 0; i < queuespec.rear; i++)
    {
        //printf("Fuse: %Lf, Taking away: %f\n", MessageArray[i].fuse, (double) INTERVAL/1000);
        MessageArray[i].fuse = MessageArray[i].fuse - opt.interval/1000.0;
        if (MessageArray[i].fuse <= 0) {
            queue_delete(&queuespec, i);
            queue_align(queuespec);
        }
    }
}

/* Initialise a set of cairo "objects" to use as tools. */
void
draw_setup_toolbox(Toolbox *t)
{
    // Figure out the total surface area that will be drawn on.
    int width = opt.width + abs(opt.shadow_xoffset);
    int height = ((opt.height * opt.max_notifications) + (opt.gap * (opt.max_notifications - 1)) + abs(opt.shadow_yoffset));
    // Figure out where exactly the surface needs to be positioned (can change with different shadow geometry).
    int xpos = opt.xpos + (opt.shadow_xoffset < 0 ? opt.shadow_xoffset : 0);
    int ypos = opt.ypos + (opt.shadow_yoffset < 0 ? opt.shadow_yoffset : 0);


    t->sfc = cairo_create_x11_surface(xpos, ypos, width, height);
    t->ctx = cairo_create(t->sfc);
    t->lyt = pango_cairo_create_layout(t->ctx);
    t->dsc = pango_font_description_from_string(opt.font);

    pango_layout_set_font_description(t->lyt, t->dsc);
    pango_font_description_free(t->dsc);
    pango_layout_set_ellipsize(t->lyt, PANGO_ELLIPSIZE_END);
}

/* Get some useful variables for the message and calculate some helper things. */
void
draw_setup_message(Message *m, Toolbox box) {
    // TODO; different settings for different types of messages?
    m->step = (opt.interval / INTERVAL_BASELINE) * opt.scroll_speed;

    pango_layout_set_markup(box.lyt, m->body, -1);
    pango_layout_set_width(box.lyt, opt.body_width*PANGO_SCALE);
    pango_layout_get_pixel_extents(box.lyt, &box.bextents, NULL);
    m->bwidth = box.bextents.width;

    pango_layout_set_markup(box.lyt, m->summary, -1);
    pango_layout_set_width(box.lyt, opt.summary_width*PANGO_SCALE);
    pango_layout_get_pixel_extents(box.lyt, &box.sextents, NULL);
    m->swidth = box.sextents.width;

    m->total_bw = opt.bw * 2;
    m->total_swidth = opt.lmargin + m->swidth;
    m->total_bheight = opt.height - m->total_bw;
    m->total_bwidth = (((opt.width - m->total_bw) - m->total_swidth) - opt.mmargin) - opt.rmargin;
    m->bwidth_startx = m->x + opt.bw + m->total_swidth + opt.mmargin;
    m->bwidth_starty = m->texty + opt.overline;
}

/* Redraw the "backgrounds" of all notifications -- useful when changing locations/coordinates, and things like that. */
void
draw_redraw(Toolbox box)
{
    // If we need to redraw, clear the surface and redraw notifications.
    draw_clear_surface(box.ctx);

    // It's possible that more messages get added while we're running this, and we don't want to set redraw to 0 before it draws.
    pthread_mutex_lock(&lock);

    int i;
    for (i = 0; i < in_queue(queuespec); i++)
    {
        // Set text dimensions and calculate some helper things ^.
        draw_setup_message(&MessageArray[i], box);

        // Draw the shadow first, so we can draw over it.
        cairo_set_operator(box.ctx, CAIRO_OPERATOR_SOURCE);
        draw_panel_shadow_fill(box.ctx, opt.shadow_color,
                MessageArray[i].x + opt.shadow_xoffset,
                MessageArray[i].y + opt.shadow_yoffset,
                opt.width, opt.height);

        // Draw the panel (borders included).
        draw_panel_fill(box.ctx, opt.bdcolor, opt.bgcolor, MessageArray[i].x, MessageArray[i].y, opt.width, opt.height, opt.bw);

        // Draw the summary (it never changes).
        pango_layout_set_markup(box.lyt, MessageArray[i].summary, -1);
        pango_layout_set_width(box.lyt, opt.summary_width*PANGO_SCALE);
        cairo_set_source_rgba(box.ctx, opt.summary_color.red, opt.summary_color.green, opt.summary_color.blue, opt.summary_color.alpha);
        cairo_move_to(box.ctx, MessageArray[i].x + opt.lmargin + opt.bw, MessageArray[i].texty + opt.overline);
        pango_cairo_show_layout(box.ctx, box.lyt);

        // Draw the body portion.
        draw_panel_body_fill(box.ctx, opt.bgcolor, MessageArray[i].bwidth_startx, MessageArray[i].y + opt.bw,
                MessageArray[i].total_bwidth, opt.height-opt.bw*2, opt.rounding);

        // Don't do this again until we should.
        MessageArray[i].redraw = 0;
    }

    //int width = opt.width + abs(opt.shadow_xoffset);
    //int height = ((opt.height * opt.max_notifications) + (opt.gap * (opt.max_notifications - 1)) + abs(opt.shadow_yoffset));

    //x_resize_window(box.sfc, width, ((opt.height * in_queue(queuespec)) + (opt.gap * (in_queue(queuespec) - 1)) + abs(opt.shadow_yoffset)));

    pthread_mutex_unlock(&lock);

    //TODO resize window? with ...
    //gotta change opt.width and stuff so its easy.
    // Resize the window.
    //x_resize_window()
    //cairo_xlib_surface_get_drawable(sfc);
}

/* Clear surface to a blank/fresh state */
void
draw_clear_surface(cairo_t *context)
{
    cairo_save(context);

    cairo_set_operator(context, CAIRO_OPERATOR_CLEAR);
    cairo_paint(context);

    cairo_restore(context);
}

/* Draw/update everything in a loop */
void
draw(void)
{
    int i;
    Toolbox box;
    draw_setup_toolbox(&box);

    int running;
    for (running = 1; running == 1;)
    {
        // TODO, could have a separate flag for first draw, if you wanna be really efficient.
        for (int i = 0; i < in_queue(queuespec); i++) {
            if (MessageArray[i].redraw) {
                draw_redraw(box);
                break;
            }
        }

        // Draw body text in each panel.
        for (i = 0; i < in_queue(queuespec); i++)
        {
            // Progress the text if it has not reached the end yet.
            MessageArray[i].textx < MessageArray[i].total_bwidth ?
                MessageArray[i].textx += MessageArray[i].step : false;

            // Make sure that we dont draw out of the box after this point.
            cairo_save(box.ctx);

            // Set the text & get its bounds.
            pango_layout_set_markup(box.lyt, MessageArray[i].body, -1);
            pango_layout_set_width(box.lyt, MessageArray[i].total_bwidth*PANGO_SCALE);
            pango_layout_get_pixel_extents(box.lyt, &box.bextents, NULL);

            cairo_set_operator(box.ctx, CAIRO_OPERATOR_SOURCE);
            // TODO, move box.bextents to message perhaps.
            cairo_rectangle(box.ctx, MessageArray[i].bwidth_startx, MessageArray[i].bwidth_starty, MessageArray[i].total_bwidth, MessageArray[i].total_bheight);
            cairo_fill_preserve(box.ctx);
            cairo_clip(box.ctx);

            // Push the body to the soure.
            cairo_set_source_rgba(box.ctx, opt.body_color.red, opt.body_color.green, opt.body_color.blue, opt.body_color.alpha);
            cairo_move_to(box.ctx, (opt.width - MessageArray[i].textx), MessageArray[i].bwidth_starty);
            pango_cairo_show_layout(box.ctx, box.lyt);

            // We should be able to draw out of the box next time.
            cairo_restore(box.ctx);
        }

        cairo_surface_flush(box.sfc);

        // Clue in for x events (allows to check for hotkeys, stuff like that).
        // They're all mouse events... position is useful for deciding which notification was clicked.
        int notification_no = 0, eventpos = 0;
        switch (check_x_event(box.sfc, &eventpos, 0))
        {
            case -3053:         // exposed.
                break;
            case -1:
                // Find out which notification was clicked.
                notification_no = parse_xy_to_notification(eventpos, opt.height+opt.gap, opt.max_notifications);

                // Delete and move below notifications up, if there are any.
                if (in_queue(queuespec) != 1) {
                    queue_delete(&queuespec, notification_no);
                    queue_align(queuespec);
                } else {
                    queue_delete(&queuespec, notification_no);
                }

                break;
        }

        // Check message fuses/remove from queue accordingly.
        draw_check_fuses();

        // If the queue is empty, kill this thread basically.
        if (in_queue(queuespec) == 0) {
            running = 0;
        }

        // Finally sleep ("animation").
        nanosleep(&opt.tspec, &opt.tspec);
    }

    // Destroy once done.
    yarn_destroy(box);
}

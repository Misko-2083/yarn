#include <stdio.h>
#include <stdlib.h>
#include <libconfig.h>

#include "config.h"
#include "datatypes.h"
#include "parse.h"

// Settings...
extern Variables opt;

int read_config(config_t cfg, char *file, Config *c)
{
    config_setting_t *setting;

    /* Read the file. If there is an error, report it and exit. */
    //if (!config_read_file(&cfg, file))
    if (!config_read_file(&cfg, "config"))
    {
        fprintf(stderr, "%s:%d - %s\n", config_error_file(&cfg), config_error_line(&cfg), config_error_text(&cfg));
        config_destroy(&cfg);

        return(EXIT_FAILURE);
    }

    /* Get the max notifications. */
    if (!config_lookup_int(&cfg, "max_notifications", &c->max_notifications))
        fprintf(stderr, "No 'max_notifications' setting in configuration file.\n");

    if (!config_lookup_int(&cfg, "gap", &c->gap))
        fprintf(stderr, "No 'gap' setting in configuration file.\n");

    setting = config_lookup(&cfg, "notification");
    if (setting != NULL)
    {
        config_setting_lookup_string(setting, "geometry", &c->geometry);

        config_setting_lookup_int(setting, "shadow", &c->shadow);
        config_setting_lookup_int(setting, "shadow_xoffset", &c->shadow_xoffset);
        config_setting_lookup_int(setting, "shadow_yoffset", &c->shadow_yoffset);

        config_setting_lookup_string(setting, "summary_color", &c->summary_color);
        config_setting_lookup_string(setting, "body_color", &c->body_color);
        config_setting_lookup_string(setting, "shadow_color", &c->shadow_color);
        config_setting_lookup_string(setting, "bgcolor", &c->bgcolor);
        config_setting_lookup_string(setting, "bdcolor", &c->bdcolor);

        config_setting_lookup_string(setting, "font", &c->font);

        config_setting_lookup_float(setting, "interval", &c->interval);
        config_setting_lookup_float(setting, "rounding", &c->rounding);
        config_setting_lookup_float(setting, "timeout", &c->timeout);

        config_setting_lookup_int(setting, "scroll_speed", &c->scroll_speed);
        config_setting_lookup_int(setting, "margin", &c->margin);
        config_setting_lookup_int(setting, "overline", &c->overline);
        config_setting_lookup_int(setting, "bw", &c->bw);

        /*
        printf("max_notif: \t%d\n \
                gap: \t%d\n \
                dimensions: \t%s\n \
                summary_color: \t%s\n \
                body_color: \t%s\n \
                bgcolor: \t%s\n \
                bdcolor: \t%s\n \
                font: \t%s\n \
                interval: \t%f\n \
                timeout: \t%f\n \
                rounding: \t%f\n \
                scroll_speed: \t%d\n \
                margin: \t%d\n \
                overline: \t%d\n \
                bw: \t%d \n", 
                c->max_notifications,
                c->gap,
                c->geometry,
                c->summary_color,
                c->body_color,
                c->bgcolor,
                c->bdcolor,
                c->font,
                c->interval,
                c->timeout,
                c->rounding,
                c->scroll_speed,
                c->margin,
                c->overline,
                c->bw);
                */
    }

    return(EXIT_SUCCESS);
}

int parse_config(Config *c)
{
    // TODO, check these options first before applying.
    opt.max_notifications = c->max_notifications;
    opt.gap = c->gap;

    parse(c->geometry, &opt.xpos, &opt.ypos, &opt.width, &opt.height);

    opt.shadow = c->shadow;
    opt.shadow_xoffset = c->shadow_xoffset;
    opt.shadow_yoffset = c->shadow_yoffset;

    opt.summary_color = hex_to_rgba(c->summary_color);
    opt.body_color = hex_to_rgba(c->body_color);
    opt.shadow_color = hex_to_rgba(c->shadow_color);
    opt.bgcolor = hex_to_rgba(c->bgcolor);
    opt.bdcolor = hex_to_rgba(c->bdcolor);

    opt.font = c->font;

    opt.interval = c->interval;
    opt.timeout = c->timeout;

    opt.scroll_speed = c->scroll_speed;
    opt.margin = c->margin;
    opt.overline = c->overline;
    opt.bw = c->bw;
    opt.rounding = c->rounding;

    free(c);

    return(EXIT_SUCCESS);
}

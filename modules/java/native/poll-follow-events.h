#ifndef POLL_FOLLOW_EVENTS_H
#define POLL_FOLLOW_EVENTS_H 1

#include "poll-events.h"
#include <iv.h>

typedef struct _PollFollowEvents {
  PollEvents super;
  gint follow_freq;
  struct iv_timer follow_timer;
  gpointer cookie;
  gboolean (*is_readable)(gpointer);
} PollFollowEvents;

PollEvents *poll_follow_events_new(gint follow_freq, gpointer cookie, gboolean (*is_readable)(gpointer));
#endif

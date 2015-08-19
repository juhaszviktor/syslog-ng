#include "poll-follow-events.h"
#include "timeutils.h"
#include "messages.h"


static void
poll_follow_events_rearm_timer(PollFollowEvents *self)
{
  iv_validate_now();
  self->follow_timer.expires = iv_now;
  timespec_add_msec(&self->follow_timer.expires, self->follow_freq);
  iv_timer_register(&self->follow_timer); 
}

static void
poll_follow_events_stop_watches(PollEvents *s)
{
  PollFollowEvents *self = (PollFollowEvents *)s;
  if (iv_timer_registered(&self->follow_timer))
    iv_timer_unregister(&self->follow_timer);
}

static void
poll_follow_events_update_watches(PollEvents *s, GIOCondition cond)
{
  PollFollowEvents *self = (PollFollowEvents *)s;
  /* we can only provide input events */
  g_assert((cond & ~G_IO_IN) == 0);

  poll_follow_events_stop_watches(s);

  if (cond & G_IO_IN)
    poll_follow_events_rearm_timer(self); 
}

static void
poll_follow_events_is_readable(gpointer s)
{
  PollFollowEvents *self = (PollFollowEvents *)s;
  if (self->is_readable(self->cookie))
    {
      poll_events_invoke_callback(s); 
    }
  else
    {
      poll_events_update_watches(s, G_IO_IN); 
    }
}

PollEvents *
poll_follow_events_new(gint follow_freq, gpointer cookie, gboolean (*is_readable)(gpointer))
{
  PollFollowEvents *self = g_new0(PollFollowEvents, 1);

  self->follow_freq = follow_freq;
  self->cookie = cookie;
  self->is_readable = is_readable;

  self->super.stop_watches = poll_follow_events_stop_watches;
  self->super.update_watches = poll_follow_events_update_watches;
  self->super.free_fn = NULL;

  IV_TIMER_INIT(&self->follow_timer);
  self->follow_timer.cookie = self;
  self->follow_timer.handler = poll_follow_events_is_readable;

  return &self->super;
}

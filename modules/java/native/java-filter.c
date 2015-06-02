#include "java-filter.h"
#include "logmsg.h"

typedef struct _JavaFilter
{
    FilterExprNode super;
    guint32 valid;
} JavaFilter;

static gboolean
java_filter_eval(FilterExprNode *s, LogMessage **msg, gint num_msg)
{
    JavaFilter *self = (JavaFilter*) s;
    return TRUE ^ s->comp;
}

void
java_filter_set_otion(FilterExprNode *s, gchar* key, gchar* value)
{
    JavaFilter *self = (JavaFilter*) s;
}

FilterExprNode *
java_filter_new(const gchar *name)
{
    JavaFilter *self = g_new0(JavaFilter, 1);
    filter_expr_node_init_instance(&self->super);
    self->super.eval = java_filter_eval;

    return &self->super;
}

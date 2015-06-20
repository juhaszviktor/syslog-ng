#ifndef JAVA_FILTER_H_INCLUDED
#define JAVA_FILTER_H_INCLUDED

#include "filter/filter-expr.h"
#include "proxies/java-filter-proxy.h"

typedef struct
{
    FilterExprNode super;
    JavaFilterProxy *proxy;
    GString *class_path;
    gchar *class_name;
    GHashTable *options;
} JavaFilter;

FilterExprNode* java_filter_new();
void java_filter_set_class_path(FilterExprNode *s, const gchar *class_path);
void java_filter_set_class_name(FilterExprNode *s, const gchar *class_name);
void java_filter_set_option(FilterExprNode *s, const gchar* key, const gchar* value);

#endif

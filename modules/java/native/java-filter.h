#ifndef JAVA_FILTER_H_INCLUDED
#define JAVA_FILTER_H_INCLUDED

#include "filter/filter-expr.h"

FilterExprNode *java_filter_new(const gchar* name);
void java_filter_set_option(FilterExprNode *s, gchar* key, gchar* value);

#endif

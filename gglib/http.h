/*
 * Copyright (c) 2013 - 2018 Filip "widelec" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef __HTTP_H__
#define __HTTP_H__

#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

UBYTE *HttpGetRequest(STRPTR url, LONG *data_len, STRPTR user_agent);
UBYTE *HttpPostRequest(STRPTR url, struct TagItem *postdata, LONG *data_len, STRPTR user_agent);

#endif /* __HTTP_H__ */

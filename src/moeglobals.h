#ifndef MOEGLOBALS_H
#define MOEGLOBALS_H

#include <qstring.h>

/* g_thumbWidth - max width to resize thumbnails to in MoeTag */
extern int g_thumbWidth;

/* g_thumbWidth - max height to resize thumbnails to in MoeTag */
extern int g_thumbHeight;


/* g_userAgent - Agent to be used by all network requests from MoeTag */
extern QString g_userAgent;

/* g_endpointDirectory - Directory to search for all endpoints to load */
extern QString g_endpointDirectory;

/* g_tagDirectory - Direct directory to load for tag data */
extern QString g_tagDirectory;

#endif // MOEGLOBALS_H

/* Copyright (c) 2013 by Wayne Wright, Round Rock, Texas.
** See license at http://github.com/w5xd/plm/blob/master/LICENSE.md */
#ifndef W5XDINSTEON_EXPORTDEFINTION_H
#define W5XDINSTEON_EXPORTDEFINTION_H
#if defined(WIN32)
#if defined (W5XDINSTEON_EXPORTS)
#define W5XD_EXPORT __declspec(dllexport)
#else
#define W5XD_EXPORT __declspec(dllimport)
#endif
#elif defined (LINUX32)
#define W5XD_EXPORT
#endif
#endif


/*
 * parg - parse argv
 *
 * Written in 2015 by Joergen Ibsen
 *
 * To the extent possible under law, the author(s) have dedicated all
 * copyright and related and neighboring rights to this software to the
 * public domain worldwide. This software is distributed without any
 * warranty. <http://creativecommons.org/publicdomain/zero/1.0/>
 */

#include "parg.h"

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

/*
 * Check if state is at end of argv.
 */
static int
is_argv_end(const struct parg_state *ps, int argc, char *const argv[])
{
	return ps->optind >= argc || argv[ps->optind] == NULL;
}

/*
 * Match nextchar against optstring.
 */
static int
match_short(struct parg_state *ps, int argc, char *const argv[],
            const char *optstring)
{
	const char *p = strchr(optstring, *ps->nextchar);

	if (p == NULL) {
		ps->optopt = *ps->nextchar++;
		return '?';
	}

	/* If no option argument, return option */
	if (p[1] != ':') {
		return *ps->nextchar++;
	}

	/* If more characters, return as option argument */
	if (ps->nextchar[1] != '\0') {
		ps->optarg = &ps->nextchar[1];
		ps->nextchar = NULL;
		return *p;
	}

	/* If option argument is optional, return option */
	if (p[2] == ':') {
		return *ps->nextchar++;
	}

	/* Option argument required, so return next argv element */
	if (is_argv_end(ps, argc, argv)) {
		ps->optopt = *ps->nextchar++;
		return optstring[0] == ':' ? ':' : '?';
	}

	ps->optarg = argv[ps->optind++];
	ps->nextchar = NULL;
	return *p;
}

/*
 * Match string at nextchar against longopts.
 */
static int
match_long(struct parg_state *ps, int argc, char *const argv[],
           const char *optstring,
           const struct parg_option *longopts, int *longindex)
{
	size_t len;
	int num_match = 0;
	int match = -1;
	int i;

	len = strcspn(ps->nextchar, "=");

	for (i = 0; longopts[i].name; ++i) {
		if (strncmp(ps->nextchar, longopts[i].name, len) == 0) {
			match = i;
			num_match++;
			/* Take if exact match */
			if (longopts[i].name[len] == '\0') {
				num_match = 1;
				break;
			}
		}
	}

	/* Return '?' on no or ambiguous match */
	if (num_match != 1) {
		ps->nextchar = NULL;
		return '?';
	}

	assert(match != -1);

	if (longindex) {
		*longindex = match;
	}

	if (ps->nextchar[len] == '=') {
		/* Option argument present, check if extraneous */
		if (longopts[match].has_arg == PARG_NOARG) {
			ps->optopt = longopts[match].flag ? 0 : longopts[match].val;
			ps->nextchar = NULL;
			return optstring[0] == ':' ? ':' : '?';
		}
		else {
			ps->optarg = &ps->nextchar[len + 1];
		}
	}
	else if (longopts[match].has_arg == PARG_REQARG) {
		/* Option argument required, so return next argv element */
		if (is_argv_end(ps, argc, argv)) {
			ps->optopt = longopts[match].flag ? 0 : longopts[match].val;
			ps->nextchar = NULL;
			return optstring[0] == ':' ? ':' : '?';
		}

		ps->optarg = argv[ps->optind++];
	}

	ps->nextchar = NULL;

	if (longopts[match].flag != NULL) {
		*longopts[match].flag = longopts[match].val;
		return 0;
	}

	return longopts[match].val;
}

void
parg_init(struct parg_state *ps)
{
	ps->optarg = NULL;
	ps->optind = 1;
	ps->optopt = '?';
	ps->nextchar = NULL;
}

int
parg_getopt(struct parg_state *ps, int argc, char *const argv[],
            const char *optstring)
{
	return parg_getopt_long(ps, argc, argv, optstring, NULL, NULL);
}

int
parg_getopt_long(struct parg_state *ps, int argc, char *const argv[],
                 const char *optstring,
                 const struct parg_option *longopts, int *longindex)
{
	assert(ps != NULL);
	assert(argv != NULL);
	assert(optstring != NULL);

	ps->optarg = NULL;

	if (argc < 2) {
		return -1;
	}

	/* Advance to next element if needed */
	if (ps->nextchar == NULL || *ps->nextchar == '\0') {
		if (is_argv_end(ps, argc, argv)) {
			return -1;
		}

		ps->nextchar = argv[ps->optind++];

		/* Check for nonoption element (including '-') */
		if (ps->nextchar[0] != '-' || ps->nextchar[1] == '\0') {
			ps->optarg = ps->nextchar;
			ps->nextchar = NULL;
			return 1;
		}

		/* Check for '--' */
		if (ps->nextchar[1] == '-') {
			if (ps->nextchar[2] == '\0') {
				return -1;
			}

			if (longopts != NULL) {
				ps->nextchar += 2;

				return match_long(ps, argc, argv, optstring,
				                  longopts, longindex);
			}
		}

		ps->nextchar++;
	}

	/* Match nextchar */
	return match_short(ps, argc, argv, optstring);
}

int
parg_reorder(int argc, char *argv[],
             const char *optstring,
             const struct parg_option *longopts)
{
	struct parg_state ps;
	char **new_argv = NULL;
	int start = 1;
	int end = argc;
	int curind;
	int i;

	assert(argv != NULL);
	assert(optstring != NULL);

	if (argc < 2) {
		return argc;
	}

	new_argv = calloc((size_t) argc, sizeof(argv[0]));

	if (new_argv == NULL) {
		return -1;
	}

	new_argv[0] = argv[0];

	parg_init(&ps);

	for (;;) {
		int c;

		curind = ps.optind;

		c = parg_getopt_long(&ps, argc, argv, optstring, longopts, NULL);

		if (c == -1) {
			break;
		}
		else if (c == 1) {
			assert(ps.optind - curind == 1);

			/* Add nonoption to end of new_argv */
			new_argv[--end] = (char *) ps.optarg;
		}
		else {
			/* Add option with any argument to start of new_argv */
			while (curind < ps.optind) {
				new_argv[start++] = argv[curind++];
			}
		}
	}

	if (end > start) {
		assert(strcmp(argv[curind], "--") == 0);

		/* Copy '--' element */
		new_argv[start++] = argv[curind++];

		/* Copy remaining nonoptions to end of new_argv */
		for (i = curind; end > start; ++i) {
			new_argv[--end] = argv[i];
		}
	}

	/* Copy options back to argv */
	for (i = 1; i < end; ++i) {
		argv[i] = new_argv[i];
	}

	/* Copy nonoptions in reverse order to argv */
	for (; i < argc; ++i) {
		argv[i] = new_argv[argc - i + end - 1];
	}

	free(new_argv);

	return end;
}

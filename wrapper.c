/****************************************************************
* wrapper.c - ensures that sourceanalyzer will be invoked 
* during build process.  This is generic code that should work
* on both Windows and *nix but it's only been tested on Windows.
*
* Instructions:
*	a) select compiler or linker to replace, e.g. cl.exe
*	b) compile wrapper.c into wrapper.exe
*	c) rename cl.exe to real_cl.exe
*	d) rename the wrapper.exe to cl.exe
*	e) move the new cl.exe to the same directory as real_cl.exe
*	f) set the %FORTIFY_BUILDID% env var with appropriate options
*		e.g. foo_version_1
*	g) add "real_cl.exe" to fortify-sca.properties
*	h) build the application in question invoking cl.exe as usual
*	i) when the build is complete run
*		sourceanalyzer.exe -b <buildid> -scan -f ...
*
* Copyright 2005 Fortify Software
*
* Changes:
*	03/25/05 [0.5] Initial prototype - Chris Prevost
*	06/27/14 [1.0] Included INI file for configuration vs always having to recompile, added logging (a.earle)
*	08/26/16 [1.1] Added debug flag, version parameter, other minor mods (a.earle)
*	11/03/16 [1.2] see word doc, added c flags (a.earle)
*
*
*
*
* To Do:
* 	What if elements are missing? Is the default structure zero'd out?
*	Test finding INI, on Linux too
*	Test/fix flags with spaces in them. uses of quotes
*	C compiler flags
*	Ensure command line doesn't overflow buffer
*
*
****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ini.h"

#define VERSION "1.2 (3 NOV 2016)"
#define _LOGGING 1
#define _USE_QUOTES 0

short debug = 1;

char *basename (filename)
	const char *filename;
{
	char *p = strrchr (filename, '\\');
	if(!p)
		p = strrchr (filename, '/');
	return p ? p + 1 : (char *) filename;
}

int logme(const char* msg)
{
	if (debug) {
		printf(msg);
	}
	return 0;
}

// ************** INIH stuff ****************

typedef struct
{
    int version;
    const char* compiler;
    const char* sourceanalyzer;
    const char* cflags;
    const char* scaflags;
    const char* buildid;
} configuration;

static int handler(void* user, const char* section, const char* name,
                   const char* value)
{
	configuration* pconfig = (configuration*)user;

	#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
	if (MATCH("INI", "version")) {
		pconfig->version = atoi(value);
		logme("matched version\n  ");
		logme(value);
		logme("\n");
	} else if (MATCH("INI", "debug")) {
		debug = atoi(value);
		logme("matched debug\n  ");
		logme(value);
		logme("\n");
	} else if (MATCH("compiler", "name")) {
		pconfig->compiler = strdup(value);
		logme("matched compiler name\n  ");
		logme(pconfig->compiler);
		logme("\n");
	} else if (MATCH("compiler", "flags")) {
		pconfig->cflags = strdup(value);
		logme("matched compiler flags\n  ");
		logme(pconfig->cflags);
		logme("\n");
	} else if (MATCH("sca", "prog")) {
		pconfig->sourceanalyzer = strdup(value);
		logme("matched prog\n  ");
		logme(pconfig->sourceanalyzer);
		logme("\n");
	} else if (MATCH("sca", "flags")) {
		pconfig->scaflags = strdup(value);
		logme("matched sca flags\n  ");
		logme(pconfig->scaflags);
		logme("\n");
	} else if (MATCH("sca", "buildid")) {
		pconfig->buildid = strdup(value);
		logme("matched buildid\n  ");
		logme(pconfig->buildid);
		logme("\n");
	} else {
		logme("unrecognized section in INI file\n[");
		logme(section);
		logme("]\n");
		logme(name);
		logme("=");
		logme(value);
		logme("\n");
		return 0;  /* unknown section/name, error */
	}
	return 1;
}

int initest(char *exe) {
	configuration config;

	if (ini_parse("fortify.ini", handler, &config) < 0) {
		printf("Can't load 'fortify.ini'\n");
		return 1;
	}
/*
	printf("Config loaded from 'fortify.ini': version=%d, call=%s, match=%s, path=%s, path2=%s\n",
		config.version, exe, config.compiler, config.path, config.sourceanalyzer);
*/
	return 0;
}

int loadini(configuration *config) {
	int i;

	i = ini_parse("fortify.ini", handler, config);
	if (i = -1) {
		// try again with another file location
		i = ini_parse("//fortify//fortify.ini", handler, config);
	}
		if (i = -1) {
		// try again with yet another file location
		i = ini_parse("//usr//fortify.ini", handler, config);
	}

	if (i < 0) {
		printf("Can't load 'fortify.ini'\n");
		return 1;
	}

	printf("Configuration loaded from 'fortify.ini'\n");
/*	printf(": version=%d, call=%s, match=%s, path=%s, path2=%s\n",
		config.version, exe, config.compiler, config.path, config.sourceanalyzer);
*/
	return 0;
}

// ************** end INIH stuff ****************

int main(int argc, char **argv) { 

	configuration config;
	char line[100000];
	int i;

#ifdef REDIRECT
	freopen("output.txt", "w", stdout);
#endif

	if((argc == 2) && (strcmp(argv[1],"-v")==0)) {
		printf("SCAWrapper version ");
		printf(VERSION);
		i = 0;
	}
	else
	{
		i = loadini(&config);

//		if (!strcmp(argv[0], "cl.exe") || !strcmp(argv[0], "link.exe"))
//		{
		if(i==0)
		{
			// Build source analyzer prepend
			if(_USE_QUOTES) {
				strcpy(line,"\"");
			}

			// sourceanalyzer command
			strcat(line,config.sourceanalyzer);
			if(_USE_QUOTES) {
				strcpy(line,"\"");
			}
			// build id
			strcat(line," -b ");
			strcat(line,config.buildid);
			strcat(line," ");
			// SCA flags
			strcat(line,config.scaflags);
			strcat(line," ");

			if(_USE_QUOTES) {
				strcpy(line,"\"");
			}
//		}
			// compiler command
			strcat(line,config.compiler);
			if(_USE_QUOTES) {
				strcpy(line,"\"");
			}
			strcat(line," ");

			// user configured compiler parameters
			if(config.cflags) {
				if(strlen(config.cflags) > 0) {
					strcat(line,config.cflags);
					strcat(line," ");
				}
			}

			// compiler parameters passed into call
			/* printf("line is %s\n", line); */
			for (i=1;i<argc;++i){
				//printf("argv[%d]: %s\n",i, argv[i]);
				if(_USE_QUOTES) {
					strcpy(line,"\"");
				}
				strcat(line,argv[i]);
				if(_USE_QUOTES) {
					strcpy(line,"\"");
				}
				strcat(line," ");
			}

			logme("line is:\n");
			logme(line);
			logme("\n");
			i = 0;

			system(line);
		}

#ifdef REDIRECT
		fclose(stdout);
		printf("Done\n");
#endif

		return i;
	}
}

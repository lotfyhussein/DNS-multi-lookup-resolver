#ifndef MULTILOOKUP_H
#define MULTILOOKUP_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include "queue.h"
#include "util.h"

void* Requester(char* file);

void* Resolver(void* p);

#endif
/*******************************************************************************
* 
* The content of this file or document is CONFIDENTIAL and PROPRIETARY
* to GEO Semiconductor.  It is subject to the terms of a License Agreement 
* between Licensee and GEO Semiconductor, restricting among other things,
* the use, reproduction, distribution and transfer.  Each of the embodiments,
* including this information and any derivative work shall retain this 
* copyright notice.
* 
* Copyright 2013-2016 GEO Semiconductor, Inc.
* All rights reserved.
*
* 
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include <pthread.h>
#include <assert.h>


#include <sys/time.h>

static pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;

struct queue {
	int maxsize;
	int item_size;
	int first;
	int last;
	int prefill;
	int prefilled;
	struct timeval tv_stash;
	struct timeval tv_pop;
	char *name;
	void *items;
};

int queue_init(struct queue **q, char *name, int maxsize, int item_size, int prefill)
{
	CHECK_ERROR (prefill > maxsize, -1,
		"Cannot initialize %s queue: prefill (%i) is greater "
		"than the maxsize (%i)", name, prefill, maxsize);

	pthread_mutex_lock(&queue_mutex);
	*q = malloc(sizeof(struct queue));
	if(*q == NULL) {
		ERROR_NORET("Out of memory");
		pthread_mutex_unlock(&queue_mutex);
		return -1;
	}

	(*q)->maxsize = maxsize;
	(*q)->item_size = item_size;
	(*q)->items = malloc(maxsize*item_size);
	if ((*q)->items == NULL) {
		ERROR_NORET("Cannot initialize %s queue: out of memory", name);
		pthread_mutex_unlock(&queue_mutex);
		return -1;
	}

	(*q)->name = name;
	(*q)->first = 0;
	(*q)->last  = 0;
	(*q)->prefill = prefill - 1;
	(*q)->prefilled = 0;
	(*q)->tv_pop.tv_sec = 0;
	(*q)->tv_pop.tv_usec = 0;
	(*q)->tv_stash.tv_sec = 0;
	(*q)->tv_stash.tv_usec = 0;

	pthread_mutex_unlock(&queue_mutex);
	return 0;
}

int queue_free(struct queue *q)
{
	CHECK_ERROR(q == NULL, -1, "Queue does not exist");

	pthread_mutex_lock(&queue_mutex);
	if(q->items)
		free(q->items);
	free(q);

	pthread_mutex_unlock(&queue_mutex);
	return 0;
}

int queue_reset(struct queue *q)
{
	CHECK_ERROR(q == NULL, -1, "Queue does not exist");

	pthread_mutex_lock(&queue_mutex);
	q->first = q->last = 0;
	q->prefilled = 0;
	pthread_mutex_unlock(&queue_mutex);
	return 0;
}

int queue_is_prefilled(struct queue *q)
{
	CHECK_ERROR(q == NULL, -1, "Queue does not exist");

	return q->prefilled;
}

int queue_stash(struct queue *q, void *p)
{
	CHECK_ERROR(q == NULL, -1, "Queue does not exist");

	int count;

#if DEBUG_AUDIO_QUEUE > 0
	struct timeval tv;
	struct timezone tz;
	int delta_time;

	gettimeofday(&tv, &tz);
	if (q->tv_stash.tv_sec == 0 && q->tv_stash.tv_usec == 0) {
		q->tv_stash = tv;
	}

	delta_time = (1000000*tv.tv_sec + tv.tv_usec) -
		(1000000*q->tv_stash.tv_sec + q->tv_stash.tv_usec);

	q->tv_stash = tv;
	TRACE("%s %-20s = %i ms\n",q->name, "queue: stash delta",
			(delta_time + 500)/1000);

#endif

	pthread_mutex_lock(&queue_mutex);

	/* Add the new item */
	assert(q->last + 1 <= q->maxsize);
	memcpy(q->items + q->last*q->item_size , p, q->item_size);

	count = ((q->last - q->first) % q->maxsize);
	if (count < 0)
		count += q->maxsize;
	count++;
	if (q->prefilled == 0 && count > q->prefill)
		q->prefilled = 1;

	/* Drop items and only keeps 'prefill' items if the queue is full */
	if (count == q->maxsize) {
		WARNING("%s queue is full. Dropping %i items.",
				q->name, q->maxsize - q->prefill - 1);
		q->first = (q->last - q->prefill) % q->maxsize;
		if (q->first < 0)
			q->first += q->maxsize;
	}

	q->last = (q->last + 1) % q->maxsize;

	pthread_mutex_unlock(&queue_mutex);
	return 0;
}

int queue_pop(struct queue *q, void **p)
{
	CHECK_ERROR(q == NULL, -1, "Queue does not exist");

#if DEBUG_AUDIO_QUEUE >= 2
	struct timeval tv;
	struct timezone tz;
	int delta_time;

	gettimeofday(&tv, &tz);
	if (q->tv_pop.tv_sec == 0 && q->tv_pop.tv_usec == 0) {
		q->tv_pop = tv;
	}

	delta_time = (1000000*tv.tv_sec + tv.tv_usec) -
		(1000000*q->tv_pop.tv_sec + q->tv_pop.tv_usec);

	TRACE("%s %-20s = %i ms\n", q->name, "queue: pop delta", (delta_time + 500)/1000);

	q->tv_pop = tv;
#endif

	pthread_mutex_lock(&queue_mutex);
	if (q->last == q->first) {/* Queue empty */
#if DEBUG_AUDIO_QUEUE > 0
		WARNING("%s queue is empty.", q->name);
#endif
		pthread_mutex_unlock(&queue_mutex);
		return -1;
	}

	*p = (q->items + q->first*q->item_size);
	q->first = (q->first + 1) % q->maxsize;

	pthread_mutex_unlock(&queue_mutex);
	return 0;
}

int queue_getsize(struct queue *q)
{
	CHECK_ERROR(q == NULL, -1, "Queue does not exist");

	int count = ((q->last - q->first) % q->maxsize);
	if (count < 0)
		count += q->maxsize;

	return count;
}

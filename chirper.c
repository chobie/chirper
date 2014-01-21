/*
 * PHP chirper Extension
 *
 * https://github.com/chobie/chirper
 *
 * Copyright 2014 Shuhei Tanuma.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include "php_chirper.h"
#include "ngx-queue.h"
#include <pthread.h>
#include <unistd.h>
#include <curl/curl.h>

static pthread_t thread;
static pthread_mutex_t lock;
static pthread_cond_t cond;

typedef struct chirper_chunk{
	ngx_queue_t queue;
	char *data;
} chirper_chunk;

chirper_chunk* chirper_chunk_init(const char *data, size_t len)
{
	chirper_chunk *chunk = malloc(sizeof(chirper_chunk));
	ngx_queue_init(&chunk->queue);
	chunk->data = malloc(len + 1);
	memcpy(chunk->data, data, len);
	chunk->data[len] = '\0';

	return chunk;
}

void chirper_chunk_delete(chirper_chunk *chunk)
{
	ngx_queue_remove(&chunk->queue);
	free(chunk->data);
	free(chunk);
}

ngx_queue_t queue;

static char buf[255] = {0};

ZEND_DECLARE_MODULE_GLOBALS(chirper);

PHP_FUNCTION(chirper_emit)
{
	char *name;
	int name_len;
	chirper_chunk *chunk;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
		"s", &name, &name_len) == FAILURE) {
		return;
	}

	chunk = chirper_chunk_init(name, name_len);

	pthread_mutex_lock(&lock);
	ngx_queue_insert_tail(&queue, &chunk->queue);
	pthread_mutex_unlock(&lock);

	pthread_cond_signal(&cond);
}

static zend_function_entry php_chirper_functions[] = {
	PHP_FE(chirper_emit, NULL)
	PHP_FE_END
};

PHP_MINFO_FUNCTION(chirper)
{
}

PHP_INI_BEGIN()
	STD_PHP_INI_BOOLEAN("chirper.dummy", "1", PHP_INI_ALL, OnUpdateLong, dummy, zend_chirper_globals, chirper_globals)
PHP_INI_END()

static PHP_GINIT_FUNCTION(chirper)
{
}

static PHP_GSHUTDOWN_FUNCTION(chirper)
{
}

static void *worker(void * arg)
{
	int error = 0;
	CURL *curl;
	CURLcode res;
	long code;

	curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:9999/");
	curl_easy_setopt(curl, CURLOPT_HEADER, 0);

	while(1) {
		fprintf(stderr, "============================\n");
		while(!ngx_queue_empty(&queue)) {
			pthread_mutex_lock(&lock);
			ngx_queue_t *q = ngx_queue_head(&queue);
			chirper_chunk *chunk = ngx_queue_data(q, chirper_chunk, queue);
			pthread_mutex_unlock(&lock);

			res = curl_easy_perform(curl);
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
			fprintf(stderr, "buf:%s, status:%d\n", chunk->data, code);
			chirper_chunk_delete(chunk);
		}
		sleep(3);
		error = pthread_cond_wait(&cond, &lock);
		pthread_mutex_unlock(&lock);
	}
	curl_easy_cleanup(curl);
}

PHP_MINIT_FUNCTION(chirper)
{
	REGISTER_INI_ENTRIES();
	ngx_queue_init(&queue);
	pthread_mutex_init(&lock, NULL);
	pthread_cond_init(&cond, NULL);
	pthread_create(&thread, NULL, &worker, NULL);

	return SUCCESS;
}

PHP_RINIT_FUNCTION(chirper)
{
	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(chirper)
{
	UNREGISTER_INI_ENTRIES();

	pthread_mutex_destroy(&lock);
	return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(chirper)
{
	return SUCCESS;
}

zend_module_entry chirper_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	PHP_CHIRPER_EXTNAME,
	php_chirper_functions,					/* Functions */
	PHP_MINIT(chirper),	/* MINIT */
	PHP_MSHUTDOWN(chirper),	/* MSHUTDOWN */
	PHP_RINIT(chirper),	/* RINIT */
	PHP_RSHUTDOWN(chirper),		/* RSHUTDOWN */
	PHP_MINFO(chirper),	/* MINFO */
#if ZEND_MODULE_API_NO >= 20010901
	PHP_CHIRPER_EXTVER,
#endif
	PHP_MODULE_GLOBALS(chirper),
	PHP_GINIT(chirper),
	PHP_GSHUTDOWN(chirper),
	NULL,
	STANDARD_MODULE_PROPERTIES_EX
};

#ifdef COMPILE_DL_CHIRPER
ZEND_GET_MODULE(chirper)
#endif
/*
 * Copyright (C) 2012 iZephyr
 *
 * Based on nginx source (C) Igor Sysoev
 */

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

/* Constatns*/
static ngx_str_t  arg_upstream = ngx_string("arg_upstream");
static ngx_str_t  arg_server = ngx_string("arg_server");
static ngx_str_t  arg_weight = ngx_string("arg_weight");
static ngx_str_t  arg_max_fails = ngx_string("arg_max_fails");
static ngx_str_t  arg_fail_timeout = ngx_string("arg_fail_timeout");
static ngx_str_t  arg_backup = ngx_string("arg_backup");
static ngx_str_t  arg_down = ngx_string("arg_down");


static char * ngx_http_touch(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

static ngx_int_t ngx_http_touch_handler(ngx_http_request_t *r);

static ngx_int_t ngx_http_touch_set_upstream_server(ngx_http_request_t *r, ngx_http_upstream_server_t *us, ngx_http_upstream_srv_conf_t *uscf);

//commands
static ngx_command_t  ngx_http_touch_commands[] =
{
    {
        ngx_string("touch"),
        NGX_HTTP_LOC_CONF|NGX_CONF_NOARGS,
        ngx_http_touch,
        0,
        0,
        NULL
    },

    ngx_null_command
};

//context
static ngx_http_module_t  ngx_http_touch_module_ctx =
{
    NULL,                                  /* preconfiguration */
    NULL,                                  /* postconfiguration */

    NULL,                                  /* create main configuration */
    NULL,                                  /* init main configuration */

    NULL,                                  /* create server configuration */
    NULL,                                  /* merge server configuration */

    NULL,                                  /* create location configuration */
    NULL,                                  /* merge location configuration */
};

//module
ngx_module_t  ngx_http_touch_module =
{
    NGX_MODULE_V1,
    &ngx_http_touch_module_ctx, /* module context */
    ngx_http_touch_commands,    /* module directives */
    NGX_HTTP_MODULE,                       /* module type */
    NULL,                                  /* init master */
    NULL,    /* init module */
    NULL,                                  /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};


static char *
ngx_http_touch(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{

    ngx_http_core_loc_conf_t  *clcf;
    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    //ÉèÖÃlocationÀïÃæµÄhandlerº¯Êý£¬´¦ÀíÇëÇóÊ±»áµ÷Ó
    clcf->handler = ngx_http_touch_handler;

    return NGX_CONF_OK;

}

static ngx_int_t
ngx_http_touch_handler(ngx_http_request_t *r)
{
    //main config
    ngx_http_upstream_main_conf_t * umcf = ngx_http_get_module_main_conf(r, ngx_http_upstream_module);
    //server config
    //ngx_http_upstream_srv_conf_t * umsf = r->srv_conf[ngx_http_upstream_module.ctx_index];
    //location config
    //ngx_http_core_loc_conf_t * hclf = (*(r->loc_conf));

    ngx_http_upstream_srv_conf_t **uscfp, *uscf;

    ngx_uint_t i, j, len;

    u_char *p, *b;

    ngx_chain_t *cl;
    ngx_http_upstream_server_t *us;

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "ngx_http_touch_handler");

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "umcfaddress=%d", umcf);

    if (umcf == NULL || umcf->upstreams.nelts <= 0)
    {
        ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "NGX_HTTP_NO_CONTENT");
        return NGX_HTTP_NO_CONTENT;
    }

    //response content buffer length
    len = 1024 * 16;

    p = b = ngx_palloc(r->pool, len);

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "umcf->upstreams.nelts=%ui\n", umcf->upstreams.nelts);


    ngx_http_variable_value_t  *upstreamname, *servername;
    ngx_uint_t                   hash;

    hash = ngx_hash_key(arg_upstream.data, arg_upstream.len);
    upstreamname = ngx_http_get_variable(r, &arg_upstream, hash);

    hash = ngx_hash_key(arg_server.data, arg_server.len);
    servername = ngx_http_get_variable(r, &arg_server, hash);

    p = ngx_slprintf(p, b + len, "Worker id: %P\n", ngx_pid);

    uscfp = umcf->upstreams.elts;

    for (i = 0; i < umcf->upstreams.nelts; i++)
    {
        ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "i=%d", i);
        uscf = uscfp[i];

        // ngx_slprintf(start, last, fmt, args)

        p = ngx_slprintf(p, b + len, "upstream name: %V\n", &uscf->host);

        ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "upstream name:%V", &uscf->host);

        if(uscf->servers != NULL && uscf->servers->nelts > 0)
        {
            ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "uscf->servers->nelts = %ui", uscf->servers->nelts);
            ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "uscf->servers->size = %ui", uscf->servers->size);

            for (j = 0; j < uscf->servers->nelts; j++)
            {
                ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "j=%d", j);
                //us = (ngx_http_upstream_server_t *)(uscf->servers->elts + j * uscf->servers->size);
                us = (ngx_http_upstream_server_t *)uscf->servers->elts +  j;
                ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "us=%d", us);

                if (us != NULL)
                {

                    if (upstreamname && upstreamname->not_found == 0
                        && servername && servername->not_found == 0
                        && ngx_strncmp(upstreamname->data, uscf->host.data, upstreamname->len) == 0
                        && ngx_strncmp(servername->data, us->addrs->name.data, servername->len) == 0)
                    {
                        ngx_http_touch_set_upstream_server(r, us, uscf);
                    }

                    ngx_log_debug6(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "usaddress=%d, weight=%d, max_fails=%d, fail_timeout=%d, down=%d, backup=%d", us, us->weight, us->max_fails, us->fail_timeout, us->down, us->backup);
                    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "server name=%V", &us->addrs->name);
                    if (us->addrs != NULL)
                    {
                        // socket to string
                        // parameters :sockaddr,  start,  max_length, port print?
                        p += ngx_sock_ntop(us->addrs->sockaddr, p, b - p + len, 1);
                    }

                    p = ngx_slprintf(p, b + len, " weight=%d, max_fails=%d, fail_timeout=%d, down=%d, backup=%d\n", us->weight, us->max_fails, us->fail_timeout, us->down, us->backup);
                }
            }
        }
    }

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 1, "ngx_alloc_chain_link");

    cl = ngx_alloc_chain_link(r->pool);
    if (cl == NULL)
    {
        ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 1, "ngx_alloc_chain_link error");
        return NGX_ERROR;
    }

    cl->next = NULL;
    cl->buf = ngx_calloc_buf(r->pool);

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 1, "ngx_calloc_buf");

    if (cl->buf == NULL)
    {
        ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 1, "ngx_calloc_buf error");
        return NGX_ERROR;
    }

    cl->buf->pos = b;
    cl->buf->last = p;
    cl->buf->last_buf = 1;/* this is last , and there will be no more buffers in the request */
    cl->buf->memory = 1; /* content is in read-only memory */ /* (i.e., filters should copy it rather than rewrite in place) */

    r->headers_out.content_length_n = p - b;
    r->headers_out.status = NGX_HTTP_OK;

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 1, "ngx_http_send_header(r)");

    if (ngx_http_send_header(r) != NGX_OK)
    {
        ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 1, "ngx_http_send_header(r) error");
        return NGX_ERROR;
    }

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 1, "ngx_http_output_filter");

    return ngx_http_output_filter(r, cl);

}

static ngx_int_t
ngx_http_touch_set_upstream_server(ngx_http_request_t *r, ngx_http_upstream_server_t *us, ngx_http_upstream_srv_conf_t *uscf)
{
    ngx_int_t weight = us->weight;
    ngx_int_t max_fails = us->max_fails;
    ngx_int_t fail_timeout = us->fail_timeout;
    ngx_int_t down = us->down;
    ngx_int_t backup = us->backup;

    ngx_http_variable_value_t  *value;
    ngx_uint_t                   hash, i;
    ngx_str_t s;

    ngx_http_upstream_rr_peers_t  *peers, *backup_peers;

    hash = ngx_hash_key(arg_weight.data, arg_weight.len);
    value = ngx_http_get_variable(r, &arg_weight, hash);

    ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 1, "value->len=%d, value->not_found=%d\n", value->len, value->not_found);

    if (value && value->not_found == 0)
    {
        weight = ngx_atoi(value->data, value->len);

        ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 1, "new weight=%d\n", weight);

        if (weight > 0)
        {
            us->weight = weight;
        }
    }

    hash = ngx_hash_key(arg_max_fails.data, arg_max_fails.len);
    value = ngx_http_get_variable(r, &arg_max_fails, hash);

    if (value && value->not_found == 0)
    {
        max_fails = ngx_atoi(value->data, value->len);

        ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 1, "new max_fails=%d\n", max_fails);

        if (max_fails != NGX_ERROR)
        {
            us->max_fails = max_fails;
        }
    }

    hash = ngx_hash_key(arg_down.data, arg_down.len);
    value = ngx_http_get_variable(r, &arg_down, hash);

    if (value && value->not_found == 0)
    {
        down = ngx_atoi(value->data, value->len);

        ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 1, "new down=%d\n", down);

        if (down != NGX_ERROR)
        {
            us->down = down;
        }
    }

    hash = ngx_hash_key(arg_backup.data, arg_backup.len);
    value = ngx_http_get_variable(r, &arg_backup, hash);

    if (value && value->not_found == 0)
    {
        backup = ngx_atoi(value->data, value->len);

        ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 1, "new backup=%d\n", backup);

        if (backup != NGX_ERROR)
        {
            us->backup = backup;
        }
    }

    hash = ngx_hash_key(arg_fail_timeout.data, arg_fail_timeout.len);
    value = ngx_http_get_variable(r, &arg_fail_timeout, hash);

    s.len = value->len;
    s.data = value->data;
    ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 1, "value=%V, value->not_found=%d", &s, value->not_found);

    if (value && value->not_found == 0)
    {
        s.len = value->len;
        s.data = value->data;

        fail_timeout = ngx_parse_time(&s, 1);

        ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 1, "new fail_timeout=%d\n", fail_timeout);

        if (fail_timeout != NGX_ERROR)
        {
            us->fail_timeout = fail_timeout;
        }
    }

    peers = uscf->peer.data;

    ngx_uint_t found = 0;

    if(peers != NULL)
    {
        backup_peers = peers->next;


        for(i = 0; i < peers->number; i ++)
        {

            ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 1, "=== peers->peer[i].name=%V\n", &peers->peer[i].name);

            if (backup_peers->peer[i].name.len == us->addrs->name.len
                && ngx_strncmp(us->addrs->name.data, peers->peer[i].name.data, us->addrs->name.len) == 0)
            {
                peers->peer[i].max_fails = us->max_fails;
                peers->peer[i].fail_timeout = us->fail_timeout;
                peers->peer[i].down = us->down;
                peers->peer[i].weight = us->weight;

                found = 1;

				break;
            }
        }

        if (backup_peers !=NULL && !found)
        {
            for(i = 0; i < backup_peers->number; i ++)
            {

                ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 1, "=== backup_peers->peer[i].name=%V\n", &backup_peers->peer[i].name);

                if (backup_peers->peer[i].name.len == us->addrs->name.len
                    && ngx_strncmp(us->addrs->name.data, backup_peers->peer[i].name.data, us->addrs->name.len) == 0)
                {
                    backup_peers->peer[i].max_fails = us->max_fails;
                    backup_peers->peer[i].fail_timeout = us->fail_timeout;
                    backup_peers->peer[i].down = us->down;
                    backup_peers->peer[i].weight = us->weight;

                    found = 1;

					break;
                }

            }
        }

    }

    return NGX_OK;

}




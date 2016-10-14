
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_HTTP_CORE_H_INCLUDED_
#define _NGX_HTTP_CORE_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#if (NGX_THREADS)
#include <ngx_thread_pool.h>
#endif


#define NGX_HTTP_GZIP_PROXIED_OFF       0x0002
#define NGX_HTTP_GZIP_PROXIED_EXPIRED   0x0004
#define NGX_HTTP_GZIP_PROXIED_NO_CACHE  0x0008
#define NGX_HTTP_GZIP_PROXIED_NO_STORE  0x0010
#define NGX_HTTP_GZIP_PROXIED_PRIVATE   0x0020
#define NGX_HTTP_GZIP_PROXIED_NO_LM     0x0040
#define NGX_HTTP_GZIP_PROXIED_NO_ETAG   0x0080
#define NGX_HTTP_GZIP_PROXIED_AUTH      0x0100
#define NGX_HTTP_GZIP_PROXIED_ANY       0x0200


#define NGX_HTTP_AIO_OFF                0
#define NGX_HTTP_AIO_ON                 1
#define NGX_HTTP_AIO_THREADS            2


#define NGX_HTTP_SATISFY_ALL            0
#define NGX_HTTP_SATISFY_ANY            1


#define NGX_HTTP_LINGERING_OFF          0
#define NGX_HTTP_LINGERING_ON           1
#define NGX_HTTP_LINGERING_ALWAYS       2


#define NGX_HTTP_IMS_OFF                0
#define NGX_HTTP_IMS_EXACT              1
#define NGX_HTTP_IMS_BEFORE             2


#define NGX_HTTP_KEEPALIVE_DISABLE_NONE    0x0002
#define NGX_HTTP_KEEPALIVE_DISABLE_MSIE6   0x0004
#define NGX_HTTP_KEEPALIVE_DISABLE_SAFARI  0x0008


typedef struct ngx_http_location_tree_node_s  ngx_http_location_tree_node_t;
typedef struct ngx_http_core_loc_conf_s  ngx_http_core_loc_conf_t;

/* 对应配置server{listen xxxx; } */
typedef struct {
    ngx_sockaddr_t             sockaddr;           /* 网络字节序ip地址 */
    socklen_t                  socklen;            /* sockaddr长度，区分v4/v6 */

    unsigned                   set:1;              /* 是否设置了backlog/rcvbuf/...等特性 */
    unsigned                   default_server:1;   /* 对应配置default_server/default */
    unsigned                   bind:1;             /* 配置bind */
    unsigned                   wildcard:1;         /* 监听地址是否为ANY */
#if (NGX_HTTP_SSL)
    unsigned                   ssl:1;              /* 配置ssl */
#endif
#if (NGX_HTTP_V2)
    unsigned                   http2:1;            /* 配置http2 */
#endif
#if (NGX_HAVE_INET6 && defined IPV6_V6ONLY)
    unsigned                   ipv6only:1;          /* 配置ipv6=on/off */
#endif
#if (NGX_HAVE_REUSEPORT)
    unsigned                   reuseport:1;         /* 配置reuseport */
#endif
    unsigned                   so_keepalive:2;      /* 配置so_keepalive=on/off */
    unsigned                   proxy_protocol:1;    /* 配置proxy_protocol */

    int                        backlog;             /* 配置backlog=, listen()的参数，linux为511 */
    int                        rcvbuf;              /* 配置rcvbuf= */
    int                        sndbuf;              /* 配置sndbuf= */
#if (NGX_HAVE_SETFIB)
    int                        setfib;              /* 配置setfib= */
#endif
#if (NGX_HAVE_TCP_FASTOPEN)
    int                        fastopen;            /* 配置fastopen= */
#endif
#if (NGX_HAVE_KEEPALIVE_TUNABLE)
    int                        tcp_keepidle;        /* 对应so_keepalive= */
    int                        tcp_keepintvl;
    int                        tcp_keepcnt;
#endif

#if (NGX_HAVE_DEFERRED_ACCEPT && defined SO_ACCEPTFILTER)
    char                      *accept_filter;      /* 配置accept_filter= */
#endif
#if (NGX_HAVE_DEFERRED_ACCEPT && defined TCP_DEFER_ACCEPT)
    ngx_uint_t                 deferred_accept;    /* 配置deferred= */
#endif

    u_char                     addr[NGX_SOCKADDR_STRLEN + 1];   /* ip地址的可读格式 */
} ngx_http_listen_opt_t;

/*读取完请求头后，nginx进入请求的处理阶段；简单的情况下，客户端发送过的统一资
  源定位符(url)对应服务器上某一路径上的资源，web服务器需要做的仅仅是将url映射
  到本地文件系统的路径，然后读取相应文件并返回给客户端。但这仅仅是最初的互联
  网的需求，而如今互联网出现了各种各样复杂的需求，要求web服务器能够处理诸如安
  全及权限控制，多媒体内容和动态网页等等问题。这些复杂的需求导致web服务器不再
  是一个短小的程序，而变成了一个必须经过仔细设计，模块化的系统。nginx良好的模
  块化特性体现在其对请求处理流程的多阶段划分当中，多阶段处理流程就好像一条流
  水线，一个nginx进程可以并发的处理处于不同阶段的多个请求。nginx允许开发者在
  处理流程的任意阶段注册模块，在启动阶段，nginx会把各个阶段注册的所有模块处理
  函数按序的组织成一条执行链。*/
typedef enum {
    NGX_HTTP_POST_READ_PHASE = 0,    /* 请求头读取完之后；nginx读取并解析完
                                        请求头以后执行回调函数 */

    NGX_HTTP_SERVER_REWRITE_PHASE,   /* server内请求地址重写；定位到server以后
                                        执行回调 */

    NGX_HTTP_FIND_CONFIG_PHASE,      /* 配置查找阶段；定位location；
                                        <NOTE>不能挂接回调函数 */
    NGX_HTTP_REWRITE_PHASE,          /* location内请求地址重写 */
    NGX_HTTP_POST_REWRITE_PHASE,     /* 请求地址重写完成之后；
                                        <NOTE>不能挂接回调函数 */

    NGX_HTTP_PREACCESS_PHASE,        /* 访问权限检查准备阶段 */

    NGX_HTTP_ACCESS_PHASE,           /* 访问权限检查阶段 */
    NGX_HTTP_POST_ACCESS_PHASE,      /* 访问权限检查完成之后的阶段 */

    NGX_HTTP_TRY_FILES_PHASE,        /* 配置项try_files处理阶段 */
    NGX_HTTP_CONTENT_PHASE,          /* 内容生成阶段 */

    NGX_HTTP_LOG_PHASE               /* 日志模块儿处理阶段 */
} ngx_http_phases;

typedef struct ngx_http_phase_handler_s  ngx_http_phase_handler_t;

typedef ngx_int_t (*ngx_http_phase_handler_pt)(ngx_http_request_t *r,
    ngx_http_phase_handler_t *ph);

struct ngx_http_phase_handler_s {
    ngx_http_phase_handler_pt  checker;    /* 辅助函数，判断是否执行handler */
    ngx_http_handler_pt        handler;    /* 回调函数，有多种返回值
                                                NGX_OK: 当前阶段处理OK，进入下一阶段
                                                NGX_DECLINED: 当前回调不处理此情况，进入同阶段下一个回调
                                                NGX_AGAIN: 当前处理所需资源不足，需等待依赖的事件发生
                                                NGX_DONE: 当前处理结束，仍需等待进一步事件发生后再做处理
                                                NGX_ERROR/...: 各种错误，需要进入异常处理
                                            */
    ngx_uint_t                 next;       /* 类似于静态链表，实现阶段间跳转；
                                              指向下一阶段的起始索引 */
};


typedef struct {
    ngx_http_phase_handler_t  *handlers;                 /* 一维的指针数组 */
    ngx_uint_t                 server_rewrite_index;
    ngx_uint_t                 location_rewrite_index;
} ngx_http_phase_engine_t;


typedef struct {
    ngx_array_t                handlers;
} ngx_http_phase_t;

/* ngx_http_core_module的http{}环境的main_conf配置 */
typedef struct {
    ngx_array_t                servers;         /* server{}配置数组，ngx_http_core_srv_conf_t */
    
    /* 根据->phases[]生成的最终函数指针列表；
          1) 从二维变一维，以加速； 
          2) 添加判断是否执行回调的checker函数； */
    ngx_http_phase_engine_t    phase_engine;
    
    /*各模块儿对http特定头的处理回调
                                    如, http模块的
                                        ngx_http_headers_in[]
                                    upstream模块儿的
                                        ngx_http_upstream_headers_in[]*/
    ngx_hash_t                 headers_in_hash;

    /* 由->variables_keys构造的hash数组
                                    ngx_http_variable_t->index =
                                    对应其在(variables[])中的数组索引*/
    ngx_hash_t                 variables_hash;

    /* 包括: 解析配置文件时，被使用到的变量; 
                                    及内置在代码中的变量; 
                                    类型ngx_http_variable_t */
    ngx_array_t                variables;       /* ngx_http_variable_t */
    ngx_uint_t                 ncaptures;

    ngx_uint_t                 server_names_hash_max_size;
    ngx_uint_t                 server_names_hash_bucket_size;

    ngx_uint_t                 variables_hash_max_size;
    ngx_uint_t                 variables_hash_bucket_size;

    /* 可配置变量的hash数组, 包括
                                    ngx_http_core_variables[], 其他模块
                                    儿支持的变量在初始化时也将注册进来;
                                    解析完毕后, 此内存会被释放, = NULL*/
    ngx_hash_keys_arrays_t    *variables_keys;

    ngx_array_t               *ports;           /* listen监听端口数组，ngx_http_conf_port_t */

    ngx_uint_t                 try_files;       /* unsigned  try_files:1 */

    /* 阶段处理函数数组, 由各模块儿ngx_module_t->ctx->postconfiguration()注册；
       注册的回调句柄在函数ngx_http_core_run_phases()被执行时, 依照返回值不同, 
       影响同一阶段的其他句柄执行; 由于先注册的模块其回调反而后执行, 即后进先出,
       因此模块儿注册的顺序非常重要; 逻辑上执行顺序靠前的模块儿需要后注册 */
    ngx_http_phase_t           phases[NGX_HTTP_LOG_PHASE + 1];
} ngx_http_core_main_conf_t;


typedef struct {
    ngx_array_t                 server_names;        /* 配置server_name, 虚拟主机数组
                                                        ngx_http_server_name_t; */
    /* server ctx */
    ngx_http_conf_ctx_t        *ctx;                 /* server{}环境上下文 */

    ngx_str_t                   server_name;         /* */

    size_t                      connection_pool_size;
    size_t                      request_pool_size;
    size_t                      client_header_buffer_size;

    ngx_bufs_t                  large_client_header_buffers;

    ngx_msec_t                  client_header_timeout;

    ngx_flag_t                  ignore_invalid_headers;
    ngx_flag_t                  merge_slashes;
    ngx_flag_t                  underscores_in_headers;

    unsigned                    listen:1;          /* 是否配置了listen指令 */
#if (NGX_PCRE)
    unsigned                    captures:1;        /* 是否有变量捕捉??? */
#endif

    ngx_http_core_loc_conf_t  **named_locations;   /**/
} ngx_http_core_srv_conf_t;


/* list of structures to find core_srv_conf quickly at run time */

/* 虚拟主机名，对应server_name配置指令 */
typedef struct {
#if (NGX_PCRE)
    ngx_http_regex_t          *regex;    /* 主机名支持正则表达式 */
#endif
    ngx_http_core_srv_conf_t  *server;   /* 对应server{}环境的srv_conf配置 */
    ngx_str_t                  name;     /* 主机名 */
} ngx_http_server_name_t;


typedef struct {
    ngx_hash_combined_t        names;

    ngx_uint_t                 nregex;
    ngx_http_server_name_t    *regex;
} ngx_http_virtual_names_t;


struct ngx_http_addr_conf_s {
    /* the default server configuration for this address:port */
    ngx_http_core_srv_conf_t  *default_server;

    ngx_http_virtual_names_t  *virtual_names;

#if (NGX_HTTP_SSL)
    unsigned                   ssl:1;
#endif
#if (NGX_HTTP_V2)
    unsigned                   http2:1;
#endif
    unsigned                   proxy_protocol:1;
};


typedef struct {
    in_addr_t                  addr;
    ngx_http_addr_conf_t       conf;
} ngx_http_in_addr_t;


#if (NGX_HAVE_INET6)

typedef struct {
    struct in6_addr            addr6;
    ngx_http_addr_conf_t       conf;
} ngx_http_in6_addr_t;

#endif


typedef struct {
    /* ngx_http_in_addr_t or ngx_http_in6_addr_t */
    void                      *addrs;
    ngx_uint_t                 naddrs;
} ngx_http_port_t;

/* listen端口的配置结构 */
typedef struct {
    ngx_int_t                  family;    /* 地址类型，struct sockaddr->sa_family */
    in_port_t                  port;      /* 端口 */
    ngx_array_t                addrs;     /* 监听地址数组，ngx_http_conf_addr_t */
} ngx_http_conf_port_t;

/* 端口结构中，存储对应监听地址的信息结构 */
typedef struct {
    ngx_http_listen_opt_t      opt;       /* 配置http{ server{listen xxx}}的解析结果 */

    ngx_hash_t                 hash;
    ngx_hash_wildcard_t       *wc_head;
    ngx_hash_wildcard_t       *wc_tail;

#if (NGX_PCRE)
    ngx_uint_t                 nregex;
    ngx_http_server_name_t    *regex;
#endif

    /* the default server configuration for this address:port */
    ngx_http_core_srv_conf_t  *default_server;    /* 默认服务器 */
    ngx_array_t                servers;           /* server{}配置信息指针数组，
                                                     ngx_http_core_srv_conf_t */
} ngx_http_conf_addr_t;


typedef struct {
    ngx_int_t                  status;
    ngx_int_t                  overwrite;
    ngx_http_complex_value_t   value;
    ngx_str_t                  args;
} ngx_http_err_page_t;


typedef struct {
    ngx_array_t               *lengths;
    ngx_array_t               *values;
    ngx_str_t                  name;

    unsigned                   code:10;
    unsigned                   test_dir:1;
} ngx_http_try_file_t;


struct ngx_http_core_loc_conf_s {
    ngx_str_t     name;          /* location name */

#if (NGX_PCRE)
    ngx_http_regex_t  *regex;
#endif

    unsigned      noname:1;   /* "if () {}" block or limit_except */
    unsigned      lmt_excpt:1;
    unsigned      named:1;

    unsigned      exact_match:1;
    unsigned      noregex:1;

    unsigned      auto_redirect:1;
#if (NGX_HTTP_GZIP)
    unsigned      gzip_disable_msie6:2;
#if (NGX_HTTP_DEGRADATION)
    unsigned      gzip_disable_degradation:2;
#endif
#endif

    ngx_http_location_tree_node_t   *static_locations;
#if (NGX_PCRE)
    ngx_http_core_loc_conf_t       **regex_locations;
#endif

    /* pointer to the modules' loc_conf */
    void        **loc_conf;

    uint32_t      limit_except;
    void        **limit_except_loc_conf;

    ngx_http_handler_pt  handler;

    /* location name length for inclusive location with inherited alias */
    size_t        alias;
    ngx_str_t     root;                    /* root, alias */
    ngx_str_t     post_action;

    ngx_array_t  *root_lengths;
    ngx_array_t  *root_values;

    ngx_array_t  *types;
    ngx_hash_t    types_hash;
    ngx_str_t     default_type;

    off_t         client_max_body_size;    /* client_max_body_size */
    off_t         directio;                /* directio */
    off_t         directio_alignment;      /* directio_alignment */

    size_t        client_body_buffer_size; /* client_body_buffer_size */
    size_t        send_lowat;              /* send_lowat */
    size_t        postpone_output;         /* postpone_output */
    size_t        limit_rate;              /* limit_rate */
    size_t        limit_rate_after;        /* limit_rate_after */
    size_t        sendfile_max_chunk;      /* sendfile_max_chunk */
    size_t        read_ahead;              /* read_ahead */

    ngx_msec_t    client_body_timeout;     /* client_body_timeout */
    ngx_msec_t    send_timeout;            /* send_timeout */
    ngx_msec_t    keepalive_timeout;       /* keepalive_timeout */
    ngx_msec_t    lingering_time;          /* lingering_time */
    ngx_msec_t    lingering_timeout;       /* lingering_timeout */
    ngx_msec_t    resolver_timeout;        /* resolver_timeout */

    ngx_resolver_t  *resolver;             /* resolver */

    time_t        keepalive_header;        /* keepalive_timeout */

    ngx_uint_t    keepalive_requests;      /* keepalive_requests */
    ngx_uint_t    keepalive_disable;       /* keepalive_disable */
    ngx_uint_t    satisfy;                 /* satisfy */
    ngx_uint_t    lingering_close;         /* lingering_close */
    ngx_uint_t    if_modified_since;       /* if_modified_since */
    ngx_uint_t    max_ranges;              /* max_ranges */
    ngx_uint_t    client_body_in_file_only; /* client_body_in_file_only */

    ngx_flag_t    client_body_in_single_buffer;
                                           /* client_body_in_singe_buffer */
    ngx_flag_t    internal;                /* internal */
    ngx_flag_t    sendfile;                /* sendfile */
    ngx_flag_t    aio;                     /* aio */
    ngx_flag_t    aio_write;               /* aio_write */
    ngx_flag_t    tcp_nopush;              /* tcp_nopush */
    ngx_flag_t    tcp_nodelay;             /* tcp_nodelay */
    ngx_flag_t    reset_timedout_connection; /* reset_timedout_connection */
    ngx_flag_t    server_name_in_redirect; /* server_name_in_redirect */
    ngx_flag_t    port_in_redirect;        /* port_in_redirect */
    ngx_flag_t    msie_padding;            /* msie_padding */
    ngx_flag_t    msie_refresh;            /* msie_refresh */
    ngx_flag_t    log_not_found;           /* log_not_found */
    ngx_flag_t    log_subrequest;          /* log_subrequest */
    ngx_flag_t    recursive_error_pages;   /* recursive_error_pages */
    ngx_flag_t    server_tokens;           /* server_tokens */
    ngx_flag_t    chunked_transfer_encoding; /* chunked_transfer_encoding */
    ngx_flag_t    etag;                    /* etag */

#if (NGX_HTTP_GZIP)
    ngx_flag_t    gzip_vary;               /* gzip_vary */

    ngx_uint_t    gzip_http_version;       /* gzip_http_version */
    ngx_uint_t    gzip_proxied;            /* gzip_proxied */

#if (NGX_PCRE)
    ngx_array_t  *gzip_disable;            /* gzip_disable */
#endif
#endif

#if (NGX_THREADS)
    ngx_thread_pool_t         *thread_pool;
    ngx_http_complex_value_t  *thread_pool_value;
#endif

#if (NGX_HAVE_OPENAT)
    ngx_uint_t    disable_symlinks;        /* disable_symlinks */
    ngx_http_complex_value_t  *disable_symlinks_from;
#endif

    ngx_array_t  *error_pages;             /* error_page */
    ngx_http_try_file_t    *try_files;     /* try_files */

    ngx_path_t   *client_body_temp_path;   /* client_body_temp_path */

    ngx_open_file_cache_t  *open_file_cache;
    time_t        open_file_cache_valid;
    ngx_uint_t    open_file_cache_min_uses;
    ngx_flag_t    open_file_cache_errors;
    ngx_flag_t    open_file_cache_events;

    ngx_log_t    *error_log;

    ngx_uint_t    types_hash_max_size;
    ngx_uint_t    types_hash_bucket_size;

    ngx_queue_t  *locations;               /* 对应http{server{}}中出现的
                                                所有location配置队列, 或
                                                者嵌套的location配置*/

#if 0
    ngx_http_core_loc_conf_t  *prev_location;
#endif
};


typedef struct {
    ngx_queue_t                      queue;
    ngx_http_core_loc_conf_t        *exact;
    ngx_http_core_loc_conf_t        *inclusive;
    ngx_str_t                       *name;
    u_char                          *file_name;
    ngx_uint_t                       line;
    ngx_queue_t                      list;
} ngx_http_location_queue_t;


struct ngx_http_location_tree_node_s {
    ngx_http_location_tree_node_t   *left;
    ngx_http_location_tree_node_t   *right;
    ngx_http_location_tree_node_t   *tree;

    ngx_http_core_loc_conf_t        *exact;
    ngx_http_core_loc_conf_t        *inclusive;

    u_char                           auto_redirect;
    u_char                           len;
    u_char                           name[1];
};


void ngx_http_core_run_phases(ngx_http_request_t *r);
ngx_int_t ngx_http_core_generic_phase(ngx_http_request_t *r,
    ngx_http_phase_handler_t *ph);
ngx_int_t ngx_http_core_rewrite_phase(ngx_http_request_t *r,
    ngx_http_phase_handler_t *ph);
ngx_int_t ngx_http_core_find_config_phase(ngx_http_request_t *r,
    ngx_http_phase_handler_t *ph);
ngx_int_t ngx_http_core_post_rewrite_phase(ngx_http_request_t *r,
    ngx_http_phase_handler_t *ph);
ngx_int_t ngx_http_core_access_phase(ngx_http_request_t *r,
    ngx_http_phase_handler_t *ph);
ngx_int_t ngx_http_core_post_access_phase(ngx_http_request_t *r,
    ngx_http_phase_handler_t *ph);
ngx_int_t ngx_http_core_try_files_phase(ngx_http_request_t *r,
    ngx_http_phase_handler_t *ph);
ngx_int_t ngx_http_core_content_phase(ngx_http_request_t *r,
    ngx_http_phase_handler_t *ph);


void *ngx_http_test_content_type(ngx_http_request_t *r, ngx_hash_t *types_hash);
ngx_int_t ngx_http_set_content_type(ngx_http_request_t *r);
void ngx_http_set_exten(ngx_http_request_t *r);
ngx_int_t ngx_http_set_etag(ngx_http_request_t *r);
void ngx_http_weak_etag(ngx_http_request_t *r);
ngx_int_t ngx_http_send_response(ngx_http_request_t *r, ngx_uint_t status,
    ngx_str_t *ct, ngx_http_complex_value_t *cv);
u_char *ngx_http_map_uri_to_path(ngx_http_request_t *r, ngx_str_t *name,
    size_t *root_length, size_t reserved);
ngx_int_t ngx_http_auth_basic_user(ngx_http_request_t *r);
#if (NGX_HTTP_GZIP)
ngx_int_t ngx_http_gzip_ok(ngx_http_request_t *r);
#endif


ngx_int_t ngx_http_subrequest(ngx_http_request_t *r,
    ngx_str_t *uri, ngx_str_t *args, ngx_http_request_t **sr,
    ngx_http_post_subrequest_t *psr, ngx_uint_t flags);
ngx_int_t ngx_http_internal_redirect(ngx_http_request_t *r,
    ngx_str_t *uri, ngx_str_t *args);
ngx_int_t ngx_http_named_location(ngx_http_request_t *r, ngx_str_t *name);


ngx_http_cleanup_t *ngx_http_cleanup_add(ngx_http_request_t *r, size_t size);


typedef ngx_int_t (*ngx_http_output_header_filter_pt)(ngx_http_request_t *r);
typedef ngx_int_t (*ngx_http_output_body_filter_pt)
    (ngx_http_request_t *r, ngx_chain_t *chain);
typedef ngx_int_t (*ngx_http_request_body_filter_pt)
    (ngx_http_request_t *r, ngx_chain_t *chain);


ngx_int_t ngx_http_output_filter(ngx_http_request_t *r, ngx_chain_t *chain);
ngx_int_t ngx_http_write_filter(ngx_http_request_t *r, ngx_chain_t *chain);
ngx_int_t ngx_http_request_body_save_filter(ngx_http_request_t *r,
    ngx_chain_t *chain);


ngx_int_t ngx_http_set_disable_symlinks(ngx_http_request_t *r,
    ngx_http_core_loc_conf_t *clcf, ngx_str_t *path, ngx_open_file_info_t *of);

ngx_int_t ngx_http_get_forwarded_addr(ngx_http_request_t *r, ngx_addr_t *addr,
    ngx_array_t *headers, ngx_str_t *value, ngx_array_t *proxies,
    int recursive);


extern ngx_module_t  ngx_http_core_module;

extern ngx_uint_t ngx_http_max_module;

extern ngx_str_t  ngx_http_core_get_method;


#define ngx_http_clear_content_length(r)                                      \
                                                                              \
    r->headers_out.content_length_n = -1;                                     \
    if (r->headers_out.content_length) {                                      \
        r->headers_out.content_length->hash = 0;                              \
        r->headers_out.content_length = NULL;                                 \
    }

#define ngx_http_clear_accept_ranges(r)                                       \
                                                                              \
    r->allow_ranges = 0;                                                      \
    if (r->headers_out.accept_ranges) {                                       \
        r->headers_out.accept_ranges->hash = 0;                               \
        r->headers_out.accept_ranges = NULL;                                  \
    }

#define ngx_http_clear_last_modified(r)                                       \
                                                                              \
    r->headers_out.last_modified_time = -1;                                   \
    if (r->headers_out.last_modified) {                                       \
        r->headers_out.last_modified->hash = 0;                               \
        r->headers_out.last_modified = NULL;                                  \
    }

#define ngx_http_clear_location(r)                                            \
                                                                              \
    if (r->headers_out.location) {                                            \
        r->headers_out.location->hash = 0;                                    \
        r->headers_out.location = NULL;                                       \
    }

#define ngx_http_clear_etag(r)                                                \
                                                                              \
    if (r->headers_out.etag) {                                                \
        r->headers_out.etag->hash = 0;                                        \
        r->headers_out.etag = NULL;                                           \
    }


#endif /* _NGX_HTTP_CORE_H_INCLUDED_ */

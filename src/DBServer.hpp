#ifndef DB_SERVER_HPP
#define DB_SERVER_HPP

#include <xmlrpc-c/registry.hpp>
#include <xmlrpc-c/server_abyss.hpp>

#include "HTTPServer.hpp"
#include "Thread.hpp"
#include "Mutex.hpp"
#include "ImageDB.hpp"


// The DBServer class runs the XML-RPC server that
// implements the LaneHawk remote API.

class DBServer
{
public:
    public: // Structors
    DBServer(ImageDB *db, int port);
    ~DBServer();
        
public:
    bool start();
    bool stop();
    
protected:
    bool register_methods();
    void register_url_handler(handleReq2Fn handler_fn);
    bool register_url_handlers();
    HTTPServer *get_http_server();
    
protected:
    static ThreadProcResult thread_proc(ThreadProcParam param);
    static xmlrpc_bool default_url_handler(TSession* r);


protected:
    static void handle_list_request(URIHandler2 *h, HTTPServer::Request *r, abyss_bool *handled);
    static void handle_match_request(URIHandler2 *h, HTTPServer::Request *r, abyss_bool *handled);
    static void handle_match_get(DBServer *s, HTTPServer::Request *r);
    static void handle_match_post(DBServer *s, HTTPServer::Request *r);
    static void handle_add_request(URIHandler2 *h, HTTPServer::Request *r, abyss_bool *handled);
    static void handle_add_get(DBServer *s, HTTPServer::Request *r);
    static void handle_add_post(DBServer *s, HTTPServer::Request *r);
    static void handle_del_request(URIHandler2 *h, HTTPServer::Request *r, abyss_bool *handled);
    static void handle_del_get(DBServer *s, HTTPServer::Request *r);
    static void handle_del_post(DBServer *s, HTTPServer::Request *r);

protected:
    ImageDB *m_db;
    xmlrpc_c::registry m_registry;
    HTTPServer m_server;
    Thread *m_thread;
    Mutex m_mutex;
};

#endif

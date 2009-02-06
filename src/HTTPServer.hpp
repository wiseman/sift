#ifndef HTTP_SERVER_HPP
#define HTTP_SERVER_HPP

#include <xmlrpc-c/base.hpp>
#include <xmlrpc-c/registry.hpp>
#include <xmlrpc-c/abyss.h>

class HTTPServer
{
public:
    typedef TSession Request;

public:
    HTTPServer(xmlrpc_c::registry const& registry,
                unsigned port_number = 8080,
                unsigned keepaliveTimeout = 0,
                unsigned keepaliveMaxConn = 0,
                unsigned timeout = 0,
                bool dontAdvertise = false);
    ~HTTPServer();
    
    void run();

public:
    bool set_default_url_handler(URIHandler handler);
    bool add_url_handler(URIHandler2 *handler);

public:
    void send_error(Request *request, int status);
    void set_status(Request *request, int status);
    void set_parameter (Request *request, const char *name, const char* value);
    void set_content_type (Request *request, const char *type);
    void set_content_length (Request *request, size_t length);



public:
    static const char *get_header_value(Request *request, char *name);
    static bool read_line(Request *request, std::string *line);
    static void get_buffer_data(Request *request, int max, char **out_start, int *out_len);
    static bool refill_buffer_from_connection(Request *request);
    

public:
    void ServerFileHandler (HTTPServer::Request *request, const std::string& path);
    void ServerDirectoryHandler (HTTPServer::Request *request, const std::string& path);

public:
    static void ResponseError (HTTPServer::Request *request);
    static std::string get_server_info ();

protected:
    bool set_url_handlers();
    void set_additional_server_parms();

protected:
    static void setup_signal_handlers ();

protected:
    // We rely on the creator to keep the registry object around as
    // long as the server object is, so that this pointer is valid.
    // We need to use some kind of automatic handle instead.

    const xmlrpc_c::registry& m_registry;
    
    std::string m_config_file_name;
    unsigned m_port_number;
    unsigned m_keepalive_timeout;
    unsigned m_keepalive_max_conn;
    unsigned m_timeout;
    bool m_dont_advertise;

    TServer *m_abyss_server;
};


#if 0
// Bah, Abyss' ConnWrite is defined to take a void* instead of const
// void* even though ConnWrite does not modify the buffer.
abyss_bool ConnWrite (TConn *c, const void *buffer, uint32_t size);

// Similarly, FileFindFirst doesn't need a char* path.
abyss_bool FileFindFirst(TFileFind *filefind, const char *path, TFileInfo *fileinfo);

// And MIMETypeGuessFromFile!
char *MIMETypeGuessFromFile(const char *filename);
#endif


#endif

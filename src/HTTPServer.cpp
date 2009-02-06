#include "HTTPServer.hpp"

#include <xmlrpc-c/server_abyss.h>
#include <xmlrpc-c/abyss.h>

#include "abyss_conn.h"
#include "abyss_session.h"
#include "abyss_server.h"

#include <signal.h>
#include <sstream>
#include <iostream>

#include <assert.h>


#define BOUNDARY    "##123456789###BOUNDARY"


HTTPServer::HTTPServer (const xmlrpc_c::registry& registry,
                          unsigned  portNumber,
                          unsigned  keepaliveTimeout,
                          unsigned  keepaliveMaxConn,
                          unsigned  timeout,
                          bool dontAdvertise)
    : m_registry(registry),
      m_keepalive_timeout(keepaliveTimeout),
      m_keepalive_max_conn(keepaliveMaxConn),
      m_timeout(timeout),
      m_dont_advertise(dontAdvertise)
{
    if (portNumber <= 0xffff)
    {
        m_port_number = portNumber;
    }

    xmlrpc_env env;
    xmlrpc_env_init(&env);
    
    DateInit();
    MIMETypeInit();
    
    m_abyss_server = new TServer;
    //ServerSetName(m_abyss_server, "hi");

    // FIXME jjw: what happens if we can't get the port we want?
    // Apparently the underlying Abyss server prints "Can't bind" and
    // calls exit!  I emailed Bryan Henderson of the xmlrpc_c project
    // about this.
    ServerCreate(m_abyss_server, "XmlRpcServer", m_port_number, "/dev/null", NULL);
    
    set_additional_server_parms();
    set_url_handlers();
}


HTTPServer::~HTTPServer()
{
    delete m_abyss_server;
}


void
HTTPServer::setup_signal_handlers ()
{
    struct sigaction mysigaction;
    
    sigemptyset(&mysigaction.sa_mask);
    mysigaction.sa_flags = 0;
    
    /* This signal indicates connection closed in the middle */
    mysigaction.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &mysigaction, NULL);
}


bool
HTTPServer::set_url_handlers ()
{
    xmlrpc_env env;
    xmlrpc_env_init(&env);
    xmlrpc_server_abyss_set_handler(&env, m_abyss_server, "/RPC2", m_registry.c_registry());
    if (env.fault_occurred)
    {
        return false;
    }
    // ServerDefaultHandler
    return true;
}




void
HTTPServer::set_additional_server_parms ()
{
    if (m_keepalive_timeout > 0)
    {
        ServerSetKeepaliveTimeout(m_abyss_server, m_keepalive_timeout);
    }
    if (m_keepalive_max_conn > 0)
    {
        ServerSetKeepaliveMaxConn(m_abyss_server, m_keepalive_max_conn);
    }
    if (m_timeout > 0)
    {
        ServerSetTimeout(m_abyss_server, m_timeout);
    }
    ServerSetAdvertise(m_abyss_server, !m_dont_advertise);
}


void
HTTPServer::run ()
{
    ServerInit(m_abyss_server);
    
    setup_signal_handlers();
       
    while (true)
    {
        ServerRunOnce2(m_abyss_server, ABYSS_FOREGROUND);
    }
    // ServerRun(m_abyss_server);

    /* We can't exist here because ServerRun doesn't return */
    assert(false);

}

bool
HTTPServer::set_default_url_handler (URIHandler handler)
{
    ServerDefaultHandler(m_abyss_server, handler);
    return true;
}

bool
HTTPServer::add_url_handler (URIHandler2 *handler)
{
    abyss_bool success;
    ServerAddHandler2(m_abyss_server, handler, &success);

    return success == TRUE;
}


const char *
HTTPServer::get_header_value (Request *request, char *name)
{
    const char *value = RequestHeaderValue(request, name);
    return value;
}


#if 0
bool
HTTPServer::read_line (Request *request, std::string *line)
{
    char *raw_line;
    if (ConnReadLine(request->conn, &raw_line, request->server->timeout))
    {
        *line = raw_line;
        return true;
    }
    else
    {
        return false;
    }
}
#endif

void
HTTPServer::get_buffer_data(HTTPServer::Request *r, 
                            int max, 
                            char **out_start, 
                            int *out_len)
{
    /* Point to the start of our data. */
    *out_start = &r->conn->buffer[r->conn->bufferpos];
    
    /* Decide how much data to retrieve. */
    *out_len = r->conn->buffersize - r->conn->bufferpos;
    if (*out_len > max)
        *out_len = max;
    
    /* Update our buffer position. */
    r->conn->bufferpos += *out_len;
}

bool
HTTPServer::refill_buffer_from_connection (HTTPServer::Request *r)
{
/*----------------------------------------------------------------------------
  Get the next chunk of data from the connection into the buffer.
  -----------------------------------------------------------------------------*/
    abyss_bool succeeded;
    
    /* Reset our read buffer & flush data from previous reads. */
    ConnReadInit(r->conn);
    
    /* Read more network data into our buffer.  If we encounter a
       timeout, exit immediately.  We're very forgiving about the
       timeout here. We allow a full timeout per network read, which
       would allow somebody to keep a connection alive nearly
       indefinitely.  But it's hard to do anything intelligent here
       without very complicated code.
    */
    succeeded = ConnRead(r->conn,
                         r->conn->server->srvP->timeout);
    return succeeded;
}

void
HTTPServer::send_error (HTTPServer::Request *request, int status)
{
    ResponseStatus(request, status);
    ResponseError(request);
}

void
HTTPServer::set_status (HTTPServer::Request *request, int status)
{
    ResponseStatus(request, status);
}

void
HTTPServer::set_content_type (HTTPServer::Request *request, const char *type)
{
    ResponseContentType(request, (char *) type);
}

void
HTTPServer::set_content_length (HTTPServer::Request *request, size_t length)
{
    ResponseContentLength(request, length);
}

void
HTTPServer::set_parameter (HTTPServer::Request *request, const char* name, const char* value)
{
    ResponseAddField(request, (char*) name, (char*) value);
}

// Fix the fact that Abyss' ConnWrite is declared annoyingly
// incorrectly, in that it is supposed to accept a writeable void* but
// doesn't actually write to the buffer.

#if 0
abyss_bool
ConnWrite (TConn *c, const void *buffer, uint32_t size)
{
    return ConnWrite(c, (void*) buffer, size);
}


// Similarly for FileFindFirst's path argument.

abyss_bool
FileFindFirst (TFileFind *filefind, const char *path, TFileInfo *fileinfo)
{
    return FileFindFirst(filefind, (char*) path, fileinfo);
}


// And for MIMETypeGuessFromFile.

char *
MIMETypeGuessFromFile (const char *filename)
{
    return MIMETypeGuessFromFile((char *) filename);
}

#endif



#if 0
bool
cmpfilenames (TFileInfo *f1, TFileInfo *f2)
{
    return strcmp(f1->name, f2->name) < 0;
}

void
HTTPServer::ServerDirectoryHandler (HTTPServer::Request *request, const std::string& path)
{
    TFileInfo fileinfo,*fi;
    TFileFind findhandle;
    char z1[4096],z2[20],z3[9],u,*z4;
    uint32_t k;
    struct tm ftm;
    TPool pool;

    if (!FileFindFirst(&findhandle, path.c_str(), &fileinfo))
    {
        ResponseStatusErrno(request);
        ResponseError(request);
        return;
    };

    std::vector<TFileInfo*> filelist;

    if (!PoolCreate(&pool, 1024))
    {
        ResponseStatus(request, 500);
        return;
    };

    do
    {
        /* Files which names start with a dot are ignored */
        /* This includes implicitly the ./ and ../ */
        if (*fileinfo.name=='.')
        {
            if (strcmp(fileinfo.name,"..") == 0)
            {
                if (strcmp(request->uri,"/") == 0)
                {
                    continue;
                }
            }
            else
            {
                continue;
            }
        }
        fi = (TFileInfo *) PoolAlloc(&pool, sizeof(fileinfo));
        if (fi)
        {
            memcpy(fi, &fileinfo, sizeof(fileinfo));
            filelist.push_back(fi);
        }

    } while (FileFindNext(&findhandle, &fileinfo));

    FileFindClose(&findhandle);

    /* Send something to the user to show that we are still alive */
    ResponseStatus(request, 200);
    ResponseContentType(request, "text/html");
    ResponseChunked(request);
    ResponseWrite(request);

    char z[8192];

    if (request->method != m_head)
    {
        sprintf(z, "<HTML><HEAD><TITLE>Index of %s</TITLE>" CRLF
                "<style type=\"text/css\">" CRLF
                "  td { font-family: monospace; padding-right: 20px; }" CRLF
                "</style></HEAD><BODY>" CRLF
                "<H1>Index of %s</H1>",request->uri,request->uri);
        strcat(z, CRLF CRLF
               "<table>" CRLF
               "<tr><th>Name</th><th>Size</th><th>Modified</th><th>Type</th></tr>" CRLF
               "<tr><td colspan=\"4\"><hr></td></tr>" CRLF);
        
        HTTPWrite(request, z, strlen(z));

        /* Sort the files */
        std::sort(filelist.begin(), filelist.end(), cmpfilenames);

        /* Write the listing */
        for (unsigned i = 0; i < filelist.size(); i++)
        {
            fi = filelist[i];
            strcpy(z, fi->name);

            k = strlen(z);

            if (fi->attrib & A_SUBDIR)
            {
                z[k++] = '/';
                z[k] = '\0';
            };


            strcpy(z1, z);

            ftm = *(gmtime(&fi->time_write));
            sprintf(z2, "%02u/%02u/%04u %02u:%02u:%02u", ftm.tm_mday, ftm.tm_mon,
                    ftm.tm_year+1900, ftm.tm_hour, ftm.tm_min, ftm.tm_sec);

            if (fi->attrib & A_SUBDIR)
            {
                strcpy(z3,"   --  ");
                z4 = "Directory";
            }
            else
            {
                if (fi->size<9999)
                {
                    u = 'b';
                }
                else 
                {
                    fi->size /= 1024;
                    if (fi->size < 9999)
                    {
                        u = 'K';
                    }
                    else
                    {
                        fi->size /= 1024;
                        if (fi->size < 9999)
                        {
                            u = 'M';
                        }
                        else
                        {
                            u = 'G';
                        }
                    }
                }
                
                sprintf(z3, "%5llu %c", fi->size, u);
                
                if (strcmp(fi->name, "..") == 0)
                {
                    z4 = "";
                }
                else
                {
                    z4 = MIMETypeFromFileName(fi->name);
                }

                if (!z4)
                {
                    z4 = "Unknown";
                }
            }
            
            sprintf(z,"<tr><td><A HREF=\"%s%s\">%s</A></td><td align=\"right\">%s</td><td>%s</td><td>%s</td></tr>"CRLF,
                    fi->name, (fi->attrib & A_SUBDIR?"/":""), z1, z3, z2, z4);
            
            HTTPWrite(request, z, strlen(z));
        }
        
        /* Write the tail of the file */
        strcpy(z, "</table>");
        strcat(z, get_server_info().c_str());
        strcat(z, "</BODY></HTML>" CRLF CRLF);
        
        HTTPWrite(request, z, strlen(z));
    };
    
    HTTPWriteEnd(request);
    /* Free memory and exit */
    PoolFree(&pool);
}
#endif

// Returns the HTML footer containing our standard server info text.

std::string
HTTPServer::get_server_info ()
{
    std::stringstream si;
    si << "<p><hr>";
    si << "<em><b>DBServer web server</b></em><br />";
    si << "</p>";
    return si.str();
}


#if 0
void
HTTPServer::ServerFileHandler(HTTPServer::Request *request, const std::string& path)
{
    char *mediatype;
    TFile file;
    uint64_t filesize, start, end;
    uint16_t i;
    TDate date;
    char *p;

    mediatype = MIMETypeGuessFromFile(path.c_str());

    if (!FileOpen(&file, path.c_str(), O_BINARY | O_RDONLY))
    {
        ResponseStatusErrno(request);
        ResponseError(request);
        return;
    };

    TDate *filedate=&request->date;

    p=RequestHeaderValue(request, "if-modified-since");
    if (p)
    {
        if (DateDecode(p, &date))
        {
            if (DateCompare(filedate, &date) <= 0)
            {
                ResponseStatus(request, 304);
                ResponseWrite(request);
                return;
            }
            else
            {
                request->ranges.size=0;
            }
        }
    }
    filesize=FileSize(&file);

    char z[4096];
    switch (request->ranges.size)
    {
    case 0:
        ResponseStatus(request, 200);
        break;

    case 1:
        if (!RangeDecode((char *)(request->ranges.item[0]), filesize, &start, &end))
        {
            ListFree(&(request->ranges));
            ResponseStatus(request, 200);
            break;
        }
        
        sprintf(z, "bytes %llu-%llu/%llu", start, end, filesize);

        ResponseAddField(request, "Content-range", z);
        ResponseContentLength(request, end-start+1);
        ResponseStatus(request, 206);
        break;

    default:
        ResponseContentType(request, "multipart/ranges; boundary=" BOUNDARY);
        ResponseStatus(request, 206);
        break;
    };
    
    if (request->ranges.size==0)
    {
        ResponseContentLength(request, filesize);
        ResponseContentType(request, mediatype);
    };
    
    if (DateToString(filedate, z))
    {
        ResponseAddField(request, "Last-Modified", z);
    }

    ResponseWrite(request);

    if (request->method!=m_head)
    {
        if (request->ranges.size==0)
        {
            ConnWriteFromFile(request->conn,&file,0,filesize-1,z,4096,0);
        }
        else if (request->ranges.size==1)
            ConnWriteFromFile(request->conn,&file,start,end,z,4096,0);
        else
            for (i=0;i<=request->ranges.size;i++)
            {
                ConnWrite(request->conn, "--",2);
                ConnWrite(request->conn, BOUNDARY,strlen(BOUNDARY));
                ConnWrite(request->conn,CRLF,2);

                if (i<request->ranges.size)
                    if (RangeDecode((char *)(request->ranges.item[i]),
                                    filesize,
                                    &start,&end))
                    {
                        /* Entity header, not response header */
                        sprintf(z,"Content-type: %s" CRLF
                                "Content-range: bytes %llu-%llu/%llu" CRLF
                                "Content-length: %llu" CRLF
                                CRLF, mediatype, start, end,
                                filesize, end-start+1);

                        ConnWrite(request->conn,z,strlen(z));

                        ConnWriteFromFile(request->conn,&file,start,end,z,4096,0);
                    };
            };
    };

    FileClose(&file);
}
#endif


// Copied from Abyss' ResponseError, more or less, but using our own
// get_server_info.
void
HTTPServer::ResponseError (HTTPServer::Request *request)
{
    ::ResponseError(request);
}



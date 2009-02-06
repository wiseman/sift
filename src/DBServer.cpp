/*
  
  /match	POST
    image

  /add		POST
    image
    label

  /del		POST
    label

  /list		GET


 */



#include "DBServer.hpp"
#include "MultipartParser.hpp"
#include "ImageDB.hpp"

#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>

#include <errno.h>

#include <xmlrpc-c/abyss.h>
#include "abyss_data.h"
#include "abyss_session.h"
#include "abyss_conn.h"
#include "sift.h"


std::string filename_extension(const std::string& filename);

typedef std::vector<MatchResult> ResultVector;

unsigned long get_millisecond_time ();
bool result_score_cmp(const MatchResult& r1, const MatchResult& r2);
bool do_match(ImageDB *db, const std::string& image_path, int max_nn_chks, bool exhaustive_match, ResultVector *results_p);


class ImagePartReceiver : public MultipartParser::PartReceiver
{
public:
  ImagePartReceiver()
    : m_image_file(NULL), m_image_filename(""), m_image_label("")
  {
  }

  bool start(const MultipartParser::PartInfo& part_info)
  {
    std::stringstream fs;
    fs << "/var/tmp/image." << getpid() << "." << filename_extension(part_info.filename);
    
    m_image_filename = fs.str();
    return true;
  }

  bool finish()
  {
    if (m_image_file) {
      fclose(m_image_file);
    }
    return true;
  }

  bool operator()(const MultipartParser::PartInfo& part_info, char *data, size_t data_len)
  {
    if (m_image_file == NULL) {
      part_info.dump();
    }
    if (part_info.name == std::string("image")) {
      if (!m_image_file) {
	m_image_file = fopen(m_image_filename.c_str(), "wb");
	if (!m_image_file) {
	  perror("Unable to open image file for writing");
	  return false;
	}
      }
      fwrite(data, data_len, 1, m_image_file);
    }
    else if (part_info.name == std::string("label")) {
      m_image_label = std::string(data, data_len);
    }
    
    return true;
  }

  std::string image_filename() const { return m_image_filename; }
  std::string image_label() const { return m_image_label; }

protected:
  FILE* m_image_file;
  std::string m_image_filename;
  std::string m_image_label;
};

class LabelPartReceiver : public MultipartParser::PartReceiver
{
public:
  LabelPartReceiver()
    : m_image_label("")
  {
  }

  bool operator()(const MultipartParser::PartInfo& part_info, char *data, size_t data_len)
  {
    if (m_image_file == NULL) {
      part_info.dump();
    }
    if (part_info.name == std::string("image")) {
      if (!m_image_file) {
	m_image_file = fopen(m_image_filename.c_str(), "wb");
	if (!m_image_file) {
	  perror("Unable to open image file for writing");
	  return false;
	}
      }
      fwrite(data, data_len, 1, m_image_file);
    }
    else if (part_info.name == std::string("label")) {
      m_image_label = std::string(data, data_len);
    }
    
    return true;
  }

  std::string image_filename() const { return m_image_filename; }
  std::string image_label() const { return m_image_label; }

protected:
  FILE* m_image_file;
  std::string m_image_filename;
  std::string m_image_label;
};



DBServer::DBServer (ImageDB *db, int port)
    : m_db(db), m_server(m_registry, port)
{
    register_methods();
}

DBServer::~DBServer ()
{
}

// Register the handlers for the XML-RPC methods we support.

bool
DBServer::register_methods ()
{
#if 0
    m_registry.addMethod("test.test", new TestMethod(this));
#endif

    return true;
}


// Registers a single URL handler function.

void
DBServer::register_url_handler (handleReq2Fn handler_fn)
{
    URIHandler2 *handler = new URIHandler2;
    handler->handleReq2 = handler_fn;
    handler->handleReq1 = NULL;
    handler->init = NULL;
    handler->term = NULL;
    handler->userdata = this;
    m_server.add_url_handler(handler);
}


// Register the URL handlers.  One handler is the standard xmlrpc_c handler for processing
// XML-RPC requests.  We also install custom handlers.

bool
DBServer::register_url_handlers ()
{
    m_server.set_default_url_handler(default_url_handler);
    register_url_handler(handle_match_request);
    register_url_handler(handle_add_request);
    register_url_handler(handle_del_request);
    register_url_handler(handle_list_request);

    return true;
}


// Prints the headers in the HTTP request.  For debugging purposes.

void dump_headers (HTTPServer::Request *session)
{
    std::cerr << "Headers:\n";
    TTable headers = session->request_headers;
    for (int i = 0; i < headers.size; i++)
    {
        std::cerr << "HDR " << headers.item[i].name << ": " << headers.item[i].value << "\n";
    }
}


// Prints an entire HTTP request.  For debugging purposes.  Can handle
// binary data, too, like if someone is trying to, say, upload a
// JPEG.

void dump_body (HTTPServer::Request *session)
{
    const char *content_length_h = RequestHeaderValue(session, "content-length");
    if (content_length_h != NULL && content_length_h[0] != '\0')
    {
        size_t content_length = strtoul(content_length_h, NULL, 10);
        size_t bytes_read = 0;
        char *chunk_ptr;
        int chunk_len;
        
        while (bytes_read < content_length)
        {
            HTTPServer::get_buffer_data(session, content_length - bytes_read, &chunk_ptr, &chunk_len);
            std::cerr << "Got " << chunk_len << " bytes.\n";
            xsnap("Data:", (unsigned char*) chunk_ptr, chunk_len, 120, bytes_read);
            
            bytes_read += chunk_len;
            
            if (bytes_read < content_length)
            {
                HTTPServer::refill_buffer_from_connection(session);
            }
        }
    }
    else
    {
        std::cerr << "Couldn't determine content length.\n";
    }
}




// Starts the remote API server.

bool
DBServer::start ()
{
    m_thread = new Thread(&thread_proc, this, false);
    m_thread->start();
    return true;
}


// Stops the remote API server.

bool
DBServer::stop ()
{
    // FIXME jjw: what should we do here?
    return true;
}




// This function runs the abyss_server, and is run its own thread.

ThreadProcResult
DBServer::thread_proc (ThreadProcParam param)
{
    DBServer *server = (DBServer*) param;
    server->register_url_handlers();
    server->m_server.set_default_url_handler(default_url_handler);

    server->m_server.run();

    return 0;
}


HTTPServer*
DBServer::get_http_server ()
{
    return &m_server;
}


// The default response, for URLs other than the XML-RPC endpoint and
// the modelset uploading URL, is to send back a 404.

xmlrpc_bool
DBServer::default_url_handler (HTTPServer::Request *r)
{
    ResponseStatus(r, 404);
    HTTPServer::ResponseError(r);
    
    return TRUE;
}


/*

The output of /list is text/plain in YAML format that looks something
like this:

# Database has 33 entries- {label: 108, num_features: 619}
- {label: 106, num_features: 3746}
- {label: 105, num_features: 2533}
- {label: 104, num_features: 2716}
- {label: 141, num_features: 12139}
- {label: public/images/157, num_features: 175}
- {label: public/images/158, num_features: 514}
- {label: public/images/166, num_features: 386}

*/

void
DBServer::handle_list_request (URIHandler2 *handler,
                                HTTPServer::Request *request,
                                abyss_bool *handled)
{
    DBServer *server = (DBServer*) handler->userdata;
    AcquirableLock lock(server->m_mutex);

    typedef std::list<std::string> StringList;

    const TRequestInfo *request_info;
    SessionGetRequestInfo(request, &request_info);
    if (strcmp(request_info->uri, "/list") != 0) {
        *handled = false;
    } else {
        std::cerr << "Handling /list from "
                  << (int) IPB1(request->conn->peerip) << "."
                  << (int) IPB2(request->conn->peerip) << "."
                  << (int) IPB3(request->conn->peerip) << "."
                  << (int) IPB4(request->conn->peerip) << "\n";
        *handled = TRUE;
        ImageDB *db = server->m_db;

        if (request_info->method != m_get) {
            server->m_server.send_error(request, 405);
        } else {
            std::stringstream texts;
            texts << "# Database has " << db->num_images() << " entries\n";
            StringList labels = db->all_labels();
	    std::cerr << "labels: " << labels.size() << "\n";
	    if (labels.size() > 0) {
	      for (StringList::const_iterator i = labels.begin();
		   i != labels.end(); i++) {
	        const FeatureInfo *fi;
		fi = db->get_image_info(*i);
                if (!fi) {
		  std::cerr << "Internal error: unable to get info for '" << *i << "'\n";
                } else {
		  texts << "- {label: " << *i << ", "
			<< "num_features: " << fi->num_features << "}\n";
                }
	      }
	    } else {
	      // Empty DB
	      texts << "[]\n";
	    }
	    
            std::string text = texts.str();
            server->m_server.set_content_type(request, "text/plain");
            server->m_server.set_content_length(request, text.size());
            ResponseStatus(request, 200);
            ResponseWriteStart(request);
            ResponseWriteBody(request, (char*) text.c_str(), text.size());
            ResponseWriteEnd(request);
        }
    }
}


void
DBServer::handle_add_request (URIHandler2 *handler,
                              HTTPServer::Request *request,
                              abyss_bool *handled)
{
    const TRequestInfo *request_info;
    SessionGetRequestInfo(request, &request_info);
    if (strcmp(request_info->uri, "/add") != 0) {
        *handled = false;
    } else {
        std::cerr << "Handling /add from "
                  << (int) IPB1(request->conn->peerip) << "."
                  << (int) IPB2(request->conn->peerip) << "."
                  << (int) IPB3(request->conn->peerip) << "."
                  << (int) IPB4(request->conn->peerip) << "\n";
        *handled = TRUE;
        DBServer *server = (DBServer*) handler->userdata;

        if (request_info->method == m_get) {
            handle_add_get(server, request);
        } else if (request_info->method == m_post) {
            handle_add_post(server, request);
        } else {
            server->m_server.send_error(request, 405);
        }
    }
}


// Handles GET /add, for which we just display a simple form.  This is really just to make
// testing by humans easier--you can point your browser at http://localhost:5555/add and
// then submit an image to be added.

void DBServer::handle_add_get (DBServer *server, HTTPServer::Request *request)
{
    server->m_server.set_status(request, 200);

    std::stringstream htmls;
    htmls << "<html><head><title>Add Image</title></head><body>"
          << "<h1>Add Image</h1>"
          << "<form action=\"/add\" enctype=\"multipart/form-data\" method=\"post\">"
          << "<input type=\"file\" name=\"image\"><br />"
          << "<p>Label <input type=\"text\" name=\"label\"></p>"
          << "<input type=\"submit\"></p></form></body></html>";
    std::string html = htmls.str();
    server->m_server.set_content_type(request, "text/html");
    server->m_server.set_content_length(request, html.size());
    
    ResponseWriteStart(request);
    ResponseWriteBody(request, (char*) html.c_str(), html.size());
    ResponseWriteEnd(request);
}


void DBServer::handle_add_post (DBServer *server, HTTPServer::Request *request)
{
    AcquirableLock lock(server->m_mutex);

    MultipartParser mp_parser(request);
    MultipartParser::PartInfo part_info;

    //dump_headers(request);
    //dump_body(request);

    // ----------
    // Receive the image.
    // ----------
    ImagePartReceiver receiver;
    if (!mp_parser.parse_init() || !mp_parser.process_parts(&receiver)) {
        // There was apparently some problem with the submission.
        std::cerr << "Request was bad.\n";
        ResponseStatus(request, 400);
        HTTPServer::ResponseError(request);
        return;
    }

    // ----------
    // Process the image.
    // ----------
    if (server->m_db->add_image_file(receiver.image_filename(), receiver.image_label()) &&
	server->m_db->save()) {
        std::stringstream htmls;
        htmls << "<html><head><title>Add Image: Succeeded</title></head>"
              << "<body><h1>Add Image: Succeeded</h1>"
              << "<p>Added image with label '" << receiver.image_label() << "'.</p>"
              << "</body></html>";
        std::string html = htmls.str();
        ResponseStatus(request, 200);
        server->m_server.set_content_type(request, "text/html");
        server->m_server.set_content_length(request, html.size());
        ResponseWriteStart(request);
        ResponseWriteBody(request, (char*) html.c_str(), html.size());
        ResponseWriteEnd(request);
    } else {
        std::stringstream htmls;
        htmls << "<html><head><title>Add Image: Failed</title></head>"
              << "<body><h1>Add Image: Failed</h1>"
              << "<p>Unable to add image with label '" << receiver.image_label() << "'.</p>"
              << "</body></html>";
        std::string html = htmls.str();
        ResponseStatus(request, 400);
        server->m_server.set_content_type(request, "text/html");
        server->m_server.set_content_length(request, html.size());
        ResponseWriteStart(request);
        ResponseWriteBody(request, (char*) html.c_str(), html.size());
        ResponseWriteEnd(request);
    }
}

void
DBServer::handle_del_request (URIHandler2 *handler,
                                HTTPServer::Request *request,
                                abyss_bool *handled)
{
    const TRequestInfo *request_info;
    SessionGetRequestInfo(request, &request_info);
    if (strcmp(request_info->uri, "/del") != 0) {
        *handled = false;
    } else {
        long request_start_time = get_millisecond_time();
        std::cerr << "--- Handling /del from "
                  << (int) IPB1(request->conn->peerip) << "."
                  << (int) IPB2(request->conn->peerip) << "."
                  << (int) IPB3(request->conn->peerip) << "."
                  << (int) IPB4(request->conn->peerip) << "\n";
        *handled = TRUE;
        DBServer *server = (DBServer*) handler->userdata;

        if (request_info->method == m_get) {
            handle_del_get(server, request);
        } else if (request_info->method == m_post) {
	    handle_del_post(server, request);
        } else {
            server->m_server.send_error(request, 405);
        }
        long request_end_time = get_millisecond_time();
        std::cerr << "--- Handled /del request in " << request_end_time - request_start_time << " ms.\n";
    }
}

void DBServer::handle_del_post (DBServer *server, HTTPServer::Request *request)
{
    AcquirableLock lock(server->m_mutex);

    MultipartParser mp_parser(request);
    MultipartParser::PartInfo part_info;

    unsigned long start_time = get_millisecond_time();

    LabelPartReceiver receiver;
    if (!mp_parser.parse_init() || !mp_parser.process_parts(&receiver)) {
        // There was apparently some problem with the submission.
        std::cerr << "Request was bad.\n";
	ResponseStatus(request, 400);
	HTTPServer::ResponseError(request);
        return;
    }

    if (server->m_db->remove_image(receiver.image_label()) &&
	server->m_db->save()) {
        unsigned long end_time = get_millisecond_time();
	std::stringstream texts;
	texts << "# Request processed in " << end_time - start_time << " ms.\n";
	std::string text = texts.str();
	server->m_server.set_content_type(request, "text/plain");
	server->m_server.set_content_length(request, text.size());
	ResponseStatus(request, 200);
	ResponseWriteStart(request);
	ResponseWriteBody(request, (char*) text.c_str(), text.size());
	ResponseWriteEnd(request);
    } else {
        // Badness of some sort.
        std::cerr << "Delete failed,\n";
        std::stringstream htmls;
        htmls << "<html><head><title>Delete Error</title></head>"
              << "<body><h1>Delete Failed</h1>"
              << "</body></html>";
            
        std::string html = htmls.str();
        ResponseStatus(request, 500);
        server->m_server.set_content_type(request, "text/html");
        server->m_server.set_content_length(request, html.size());
        ResponseWriteStart(request);
        ResponseWriteBody(request, (char*) html.c_str(), html.size());
        ResponseWriteEnd(request);
    }
}


/*

Handles GET /del, for which we just display a simple form.  This is
really just to make testing by humans easier.

*/

void DBServer::handle_del_get (DBServer *server, HTTPServer::Request *request)
{
    server->m_server.set_status(request, 200);

    std::stringstream htmls;
    htmls << "<html><head><title>Delete Image</title></head><body>"
          << "<h1>Delete Image</h1>"
          << "<form action=\"/del\" enctype=\"multipart/form-data\" method=\"post\">"
          << "<p>Label <input type=\"text\" name=\"label\"><p></p>"
          << "<input type=\"submit\"></p></form></body></html>";
    std::string html = htmls.str();
    server->m_server.set_content_type(request, "text/html");
    server->m_server.set_content_length(request, html.size());
    
    ResponseWriteStart(request);
    ResponseWriteBody(request, (char*) html.c_str(), html.size());
    ResponseWriteEnd(request);
}


void
DBServer::handle_match_request (URIHandler2 *handler,
                                HTTPServer::Request *request,
                                abyss_bool *handled)
{
    const TRequestInfo *request_info;
    SessionGetRequestInfo(request, &request_info);
    if (strcmp(request_info->uri, "/match") != 0) {
        *handled = false;
    } else {
        long request_start_time = get_millisecond_time();
        std::cerr << "--- Handling /match from "
                  << (int) IPB1(request->conn->peerip) << "."
                  << (int) IPB2(request->conn->peerip) << "."
                  << (int) IPB3(request->conn->peerip) << "."
                  << (int) IPB4(request->conn->peerip) << "\n";
        *handled = TRUE;
        DBServer *server = (DBServer*) handler->userdata;

        if (request_info->method == m_get) {
            handle_match_get(server, request);
        } else if (request_info->method == m_post) {
            handle_match_post(server, request);
        } else {
            server->m_server.send_error(request, 405);
        }
        long request_end_time = get_millisecond_time();
        std::cerr << "--- Handled /match request in " << request_end_time - request_start_time << " ms.\n";
    }
}


/*

Handles POST /match.  Accepts an image, matches against it, returns
results.  The output is text/plain in YAML format and looks something
like this:

# Request processed in 2358 ms.
- {label: public/images/48, score: 67.8, percentage: 10.0893}
- {label: 106, score: 23.4, percentage: 3.48214}
- {label: 141, score: 17, percentage: 2.52976}
- {label: public/images/179, score: 14, percentage: 2.08333}
- {label: public/images/197, score: 11.6, percentage: 1.72619}
- {label: public/images/166, score: 10.2, percentage: 1.51786}
- {label: 105, score: 9.8, percentage: 1.45833}
- {label: public/images/226, score: 9.6, percentage: 1.42857}
- {label: public/images/276, score: 7.8, percentage: 1.16071}

*/

void DBServer::handle_match_post (DBServer *server, HTTPServer::Request *request)
{
    AcquirableLock lock(server->m_mutex);

    MultipartParser mp_parser(request);
    MultipartParser::PartInfo part_info;

    unsigned long start_time = get_millisecond_time();

    //dump_headers(request);
    //dump_body(request);

    // ----------
    // Receive the image.
    // ----------

    ImagePartReceiver receiver;
    if (!mp_parser.parse_init() ||
        !mp_parser.process_parts(&receiver)) {
        // There was apparently some problem with the submission.
        std::cerr << "Request was bad.\n";
        ResponseStatus(request, 400);
        HTTPServer::ResponseError(request);
        return;
    }

    // ----------
    // Process the image.
    // ----------
    ResultVector results;
    int max_nn_chks = 200;
    bool exhaustive_match = false;
    std::cerr << "  --- Doing match\n";
    long match_start_time = get_millisecond_time();

    if (!do_match(server->m_db, receiver.image_filename(),
		  max_nn_chks, exhaustive_match, &results)) {
        // Badness of some sort.
        std::cerr << "Match failed,\n";
        std::stringstream htmls;
        htmls << "<html><head><title>Match Error</title></head>"
              << "<body><h1>Match Failed</h1><p>Maybe the image you uploaded is corrupt?</p>"
              << "</body></html>";
            
        std::string html = htmls.str();
        ResponseStatus(request, 500);
        server->m_server.set_content_type(request, "text/html");
        server->m_server.set_content_length(request, html.size());
        ResponseWriteStart(request);
        ResponseWriteBody(request, (char*) html.c_str(), html.size());
        ResponseWriteEnd(request);
    } else {
        // ----------
        // Return the match results.
        // ----------
        unsigned long end_time = get_millisecond_time();
        std::stringstream texts;
        texts << "# Request processed in " << end_time - start_time << " ms.\n";
	if (results.size() > 0) {
	  for (ResultVector::const_iterator r = results.begin(); r != results.end(); r++) {
	    texts << "- {label: " << r->label << ", "
		  << "score: " << r->score << ", "
		  << "percentage: " << r->percentage << "}\n";
	  }
	} else {
	  // No matches.
	  texts << "[]\n";
	}
        std::string text = texts.str();
        server->m_server.set_content_type(request, "text/plain");
        server->m_server.set_content_length(request, text.size());
        ResponseStatus(request, 200);
        ResponseWriteStart(request);
        ResponseWriteBody(request, (char*) text.c_str(), text.size());
        ResponseWriteEnd(request);
    }
    long match_end_time = get_millisecond_time();
    std::cerr << "  --- Match took " << match_end_time - match_start_time << " ms, "
              << "got " << results.size() << " results.\n";
}


// Handles GET /match, for which we just display a simple form.  This is really just to
// make testing by humans easier--you can point your browser at
// http://localhost:5555/match and then submit an image to be matched.

void DBServer::handle_match_get (DBServer *server, HTTPServer::Request *request)
{
    server->m_server.set_status(request, 200);

    std::stringstream htmls;
    htmls << "<html><head><title>Match Image</title></head><body>"
          << "<h1>Match Image</h1>"
          << "<form action=\"/match\" enctype=\"multipart/form-data\" method=\"post\">"
          << "<input type=\"file\" name=\"image\"><p></p>"
          << "<input type=\"submit\"></p></form></body></html>";
    std::string html = htmls.str();
    server->m_server.set_content_type(request, "text/html");
    server->m_server.set_content_length(request, html.size());
    
    ResponseWriteStart(request);
    ResponseWriteBody(request, (char*) html.c_str(), html.size());
    ResponseWriteEnd(request);
}


std::string strtoupper (const std::string& s)
{
    std::string upper(s);
    std::transform(upper.begin(), upper.end(),
                   upper.begin(),
                   toupper);
    return s;
}

std::string filename_extension(const std::string& filename)
{
    int dot_position = filename.rfind('.');
    std::string file_extension = (dot_position >= 0 ) ? strtoupper( filename.substr(dot_position+1) ): "";
    return file_extension;
}

bool do_match(ImageDB *db, const std::string& image_path, int max_nn_chks, bool exhaustive_match,
              ResultVector *results_p)
{
    IplImage *image = cvLoadImage(image_path.c_str(), 1);
    if (!image) {
        std::cerr << "Unable to open image '" << image_path << "': "
                  << strerror(errno) << "\n";
        return false;
    }

    struct feature *features;
    int num_features;
    num_features = sift_features(image, &features);

    MatchResults results;
    if (exhaustive_match) {
        results = db->exhaustive_match(features, num_features);
    } else {
        results = db->match(features, num_features, max_nn_chks);
    }

    ResultVector sorted_results;
    for (MatchResults::const_iterator i = results.begin();
         i != results.end();
         i++) {
        sorted_results.push_back(i->second);
    }
    std::sort(sorted_results.begin(), sorted_results.end(), result_score_cmp);

    *results_p = sorted_results;

    return true;
}

bool result_score_cmp(const MatchResult& r1, const MatchResult& r2)
{
    return r1.score > r2.score;
}

unsigned long get_millisecond_time ()
{
    struct timeval tv;
    gettimeofday (&tv, NULL);
    return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
}




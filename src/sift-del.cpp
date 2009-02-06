/*

*/

#include "SIFT.hpp"

#include <iostream>

#include <sys/stat.h>
#include <unistd.h>

extern "C" {
#include "sift.h"
#include "imgfeatures.h"
#include "kdtree.h"
#include "utils.h"
#include "xform.h"

#include <cv.h>
#include <cxcore.h>
#include <highgui.h>
}

using namespace SIFT;


void usage(const char* progname)
{
    std::cerr << progname << " removes an image from the database.\n\n";
    std::cerr << "Usage: " << progname << " [-v] <db file> <label>\n";
}

int main( int argc, char** argv )
{
    bool verbose = false;
    const char *progname = argv[0];

    int ch;
    while ((ch = getopt(argc, argv, "v")) != -1) {
        switch (ch) {
        case 'v':
            verbose = true;
            break;
        default:
            usage(progname);
            return 1;
        }
    }
    argc -= optind;
    argv += optind;

    if (argc != 2) {
        usage(progname);
        return 1;
    }

    const char *db_path = argv[0];
    const char *label = argv[1];

    if (verbose) {
        std::cout << "Removing image '" << label << "' from database '"
                  << db_path << "'\n";
    }
    
    // Add to DB.
    Database db;
    if (!db.load(db_path)) {
        return 2;
    }

    bool removed = db.remove_image(label);
    if (!removed && verbose) {
        std::cout << "Database does not contain label.\n";
    }

    if (!db.save(db_path)) {
        return 3;
    }

    if (verbose) {
        std::cout << "Database now has " << db.image_count() << " entries.\n";
    }

    return 0;
}

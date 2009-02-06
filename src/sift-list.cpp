/*

*/

#include "SIFT.hpp"

#include <iostream>
#include <errno.h>

#include <sys/stat.h>
#include <unistd.h>

using namespace SIFT;


void usage(const char* progname)
{
    std::cerr << progname << " lists the images in a database.\n\n";
    std::cerr << "Usage: " << progname << " <db file>\n";
}

typedef std::list<std::string> StringList;

int main( int argc, char** argv )
{
    if (argc != 2) {
        usage(argv[0]);
        return 1;
    }

    Database db;
    if (!db.load(argv[1])) {
        std::cerr << "Error loading db '" << argv[1] << "' ("
                  << strerror(errno) << ")\n";
        return 2;
    }

    printf("Database %s has %d entries", argv[1], db.image_count());
    StringList labels = db.all_labels();
    if (labels.begin() != labels.end()) {
        printf(":\nLabel                                                             # Features\n");
        printf("----------------------------------------------------------------------------\n");
    } else {
        printf("\n");
    }
    for (StringList::const_iterator i = labels.begin();
         i != labels.end(); i++) {
        const FeatureInfo *fi;
        fi = db.get_image_info(*i);
        if (!fi) {
            std::cerr << "Internal error: unable to get info for '" << *i << "'\n";
        } else {
            printf("%-65s %10d\n", i->c_str(), fi->num_features);
        }
    }
    
    return 0;
}

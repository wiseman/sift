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
    std::cerr << progname << " adds an image to a database.\n\n";
    std::cerr << "Usage: " << progname << " [-v] <db file> <image file> <label>\n";
    std::cerr << "       " << progname << " -l <db file>\n";
}

int features_from_img(const char *path, struct feature** feat)
{
    IplImage *img = cvLoadImage(path, 1);
    if (img) {
        return sift_features(img, feat);
    } else {
        return -1;
    }
    cvReleaseImage(&img);
}


int add_image(Database *db, const char *image_path, const char *label, bool verbose)
{
    if (verbose) {
        std::cout << "Adding image '" << image_path << "' to database.\n";
    }
    
    // Extract features.
    struct feature *features;
    int num_features;
    num_features = features_from_img(image_path, &features);
    if (num_features < 0) {
        perror("Unable to load image or extract features.");
        return 2;
    }
    if (verbose) {
        std::cout << "Image has " << num_features << " features.\n";
    }

    // Add to ImageDB.
    if (!db->add_image_features(features, num_features, label)) {
        std::cerr << "Unable to add image (duplicate label?)\n";
        return 5;
    }

    return 0;
}

int main( int argc, char** argv )
{
    bool verbose = false;
    bool use_list = false;
    const char *progname = argv[0];

    int ch;
    while ((ch = getopt(argc, argv, "vl")) != -1) {
        switch (ch) {
        case 'v':
            verbose = true;
            break;
        case 'l':
            use_list = true;
            break;
        default:
            usage(progname);
            return 1;
        }
    }
    argc -= optind;
    argv += optind;

    if ((!use_list && argc != 3) || (use_list && argc != 1)) {
        usage(progname);
        return 1;
    }
    
    const char *db_path = argv[0];
    const char *image_path, *label;

    // Load ImageDB.
    Database db;
    struct stat stat_buf;
    if (stat(db_path, &stat_buf) == 0) {
        if (!db.load(db_path)) {
            return 3;
        }
    } else {
        if (verbose) {
            std::cout << "Creating new database file '" << db_path << "'\n";
        }
    }

    if (!use_list) {
        image_path = argv[1];
        label = argv[2];
        int result = add_image(&db, image_path, label, verbose);
        if (result) {
            return result;
        }
    } else {
        char line[16384];
        while (fgets(line, sizeof line, stdin) != NULL) {
            // Get rid of any trailing newline.
            if (line[strlen(line) - 1] == '\n') {
                line[strlen(line) - 1] = '\0';
            }

            image_path = strtok(line, " \t");
            label = strtok(NULL, " \t");
            int result = add_image(&db, image_path, label, verbose);
            if (result) {
                return result;
            }
        }
    }
    

    if (!db.save(db_path)) {
        return 4;
    }

    if (verbose) {
        std::cout << "Database now has " << db.image_count() << " entries.\n";
    }

    return 0;
}



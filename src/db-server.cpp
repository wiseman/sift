#include "DBServer.hpp"
#include "SIFTDB.hpp"

#include <iostream>


void usage(const char* progname)
{
    std::cerr << progname << " is an image database server.\n\n";
    std::cerr << "Usage: " << progname << " <port> <db file>\n";
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        usage(argv[0]);
        return 1;
    }

    int port = strtol(argv[1], NULL, 10);
    const char *db_path = argv[2];

    ImageDB db;
    std::cout << "Loading DB " << db_path << ".\n";
    db.load(db_path);
    db.coalesce();

    std::cout << "Starting server on port " << port << ".\n";
    DBServer server(&db, port);
    server.start();
    while (true) {
        sleep(30000);
    }

    return 0;
}

/**
 * @assignment1
 * @author  Team Members <ubitname@buffalo.edu>
 * @version 1.0
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * @section DESCRIPTION
 *
 * This contains the main function. Add further description here....
 */
#include <iostream>
#include <stdio.h>

#include "../include/global.h"
#include "../include/logger.h"

using namespace std;

/**
 * main function
 *
 * @param  argc Number of arguments
 * @param  argv The argument list
 * @return 0 EXIT_SUCCESS
 */
void run_server(int port);
void run_client(int port);

int main(int argc, char **argv)
{
    if (argc != 3) {
        // Don't access argv[2] before checking this!
        cerr << "Usage: " << argv[0] << " <s/c> <port>" << endl;
        return -1;
    }

    // ✅ Now safe to access argv[2]
    cse4589_init_log(argv[2]);
    fclose(fopen(LOGFILE, "w"));

    std::string mode = argv[1];
    int port_number = atoi(argv[2]);

    if (mode == "s") {
        run_server(port_number);
    } else if (mode == "c") {
        run_client(port_number);
    } else {
        cerr << "Invalid mode. Use 's' for server or 'c' for client." << endl;
        return -1;
    }

    return 0;
}

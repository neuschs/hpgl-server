#include <iostream>

#include <CLI/App.hpp>
#include <CLI/Formatter.hpp>
#include <CLI/Config.hpp>
#include <CLI/Validators.hpp>

#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>
#include <iostream>
//#include <spdlog/spdlog.h>

#include <thread>

#include "job.hpp"

#define BUFSIZE 256

int main(int argc, char** argv) 
{
    CLI::App app { "HPGL Server" };

    std::string configstr;
    CLI::Option* port_option { app.add_option("--device", configstr, "Device string") };
    port_option->required();

    std::string directory;
    CLI::Option* directory_option { app.add_option("--directory", directory, "Device string") };
    directory_option->check(CLI::ExistingDirectory);
    


    /* allow configuration by ini file */
    CLI::Option* config { app.set_config("--config") };

    config->description("Configuration by ini-File");

    /* parse cli arguments */
    CLI11_PARSE(app, argc, argv);
    
    Job j { configstr };
    j.setDirectory(directory);
    j.execute();

    return 0;
}

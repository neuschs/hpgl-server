#ifndef __JOB_H
#define __JOB_H

#include <iostream>
#include <fstream>
#include <string>
#include <boost/asio.hpp>
#include <boost/asio/serial_port.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/bind/bind.hpp>
#include <functional>
#include <boost/filesystem.hpp>
//#include <spdlog/spdlog.h>
//#include <spdlog/sinks/stdout_color_sinks.h>
//#include <fmt/core.h>

/* This configuration string has '6' parts */
/* TEK11801C:COM1:9600:8:N:1 */
constexpr int argument_size { 6 };
constexpr int buffer_size { 1000000 };

using sp_base = boost::asio::serial_port_base;

struct Configuration
{
    std::string port_name { "" };
    std::string job_name { "" };
    sp_base::baud_rate baud_rate { 9600 };
    sp_base::character_size data_bits { 8 };
    sp_base::stop_bits stop_bits { sp_base::stop_bits::one };
    sp_base::parity parity { sp_base::parity::none };
};

enum class States
{
    stop,
    transmission_idle,
    waiting_for_data_timer_start,
    waiting_for_data_timer_ended,
    transmission_ended,
    write_file
};

class Job
{
private:
    /* variables */
    boost::asio::io_service io_service;
    boost::asio::serial_port serial{io_service};
    char buffer[buffer_size];
    boost::filesystem::path directory;
    Configuration config;
    
    boost::filesystem::path subpath;
    bool is_alive { true };

    /* functions */
    Configuration parseArgs(std::string& configstr);
    size_t read_data(char* buffer, size_t chunk_size, int timeout);
    
    States current_state { States::stop };
    /* logger */
    //std::shared_ptr<spdlog::logger> logger;
    
public:
    /* functions */
    Job(std::string configstr);
    ~Job();
    void setDirectory(std::string& path);
    void init();
    void execute();

};



#endif /* __JOB_H */


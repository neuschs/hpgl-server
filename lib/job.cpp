#include "job.hpp"

Job::Job(std::string configstr)
{
    Configuration config {};

    /* parse configuration given by string */
    this->config = this->parseArgs(configstr);

    /* initialize directory path with local working directory */
    this->subpath = boost::filesystem::path(this->config.job_name);
    this->directory = boost::filesystem::current_path() / subpath;

    /* set up logger */
    //this->logger = spdlog::stdout_color_mt(this->config.job_name);
    //spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%t] [%n] [%^%l%$]  %v");
    this->init();
}

void Job::init()
{
    /* open serial port */
    this->serial.open(this->config.port_name);

    /* configure serial_port with attributes */
    this->serial.set_option(this->config.baud_rate);
    this->serial.set_option(this->config.data_bits);
    this->serial.set_option(this->config.parity);
    this->serial.set_option(this->config.stop_bits);

    /* create subdirectory if it doesn't exist */
    if(!boost::filesystem::is_directory(this->directory))
    {
        /* create subdirectory */
        boost::filesystem::create_directory(this->directory);
    }
    std::cout << "Devicejob configured" << std::endl;

    /* init state of 'statemachine' */
    this->current_state = States::transmission_idle;
}

Job::~Job()
{
    ;
}

/// @brief This function may be called by an additional thread to allow parallel execution of multiple devices instances
void Job::execute()
{   
    /* 'stop' is the break condition */
    while(this->is_alive)
    {
        /* offset for buffer */
        static int data_offset { 0 };

        switch(this->current_state)
        {
            case States::transmission_idle:
            {
                char tempBuffer[8192];
                size_t data_received { 0 };

                /* detector flag if a transmission started - crude - */
                static bool first_data_received = false;

                /* try to read data */
                data_received = this->read_data(tempBuffer, 8192, 1000);

                /* if we received a first chunk and then no second chunk, we land in the else if 
                 * if we receive no data just repeat by letting the current_state still
                 * if we receive consecutive chunks, the method will always enter the first structure
                 */
                if(data_received)
                {
                    
                    /* iterator to end of chunk */
                    std::vector<char>::iterator end { tempBuffer + data_received };

                    /* get pointer to offset */
                    char* bufferPointer = this->buffer + data_offset;

                    /* copy chunk into 'global' buffer */
                    std::memcpy(bufferPointer, tempBuffer, data_received);

                    /* update offset */
                    data_offset += static_cast<int>(data_received);
                    
                    if(!first_data_received)
                    {
                        std::cout << "Transmission started" << std::endl;
                    }

                    first_data_received = true;
                }
                else if(first_data_received)
                {
                    /* no consecutive chunk followed, start timeout procedure */
                    this->current_state = States::waiting_for_data_timer_start;
                    first_data_received = false;
                }
            }
            break;
            case States::waiting_for_data_timer_start:
            {
                /* initiate deadline timer for finished criteria */
                boost::asio::deadline_timer timer {this->io_service};

                /* lambda as timeout callback */
                std::function<void(States&, const boost::system::error_code&)> timeout_callback {
                    [&](States& state, const boost::system::error_code& e)
                    {
                        this->current_state = States::waiting_for_data_timer_ended;
                        return;
                    }
                };

                /* reset boost asio service */
                this->io_service.reset();

                /* set deadline timer to 1000ms */
                timer.expires_from_now(boost::posix_time::millisec(5000));
                
                /* configure timer with callback */
                timer.async_wait(boost::bind(timeout_callback, boost::ref(this->current_state), boost::asio::placeholders::error()));
                
                /* start timer */
                this->io_service.run();
            }
            break;
            case States::waiting_for_data_timer_ended:
            {
                char tempBuffer[8192];
                size_t data_received { 0 };

                /* detector flag if a transmission started - crude - */
                static bool first_data_received = false;

                /* try to read data */
                data_received = this->read_data(tempBuffer, 8192, 1000);

                /* if there is data after the timeout, continue to read data */
                if(data_received)
                {
                    /* iterator to end of chunk */
                    //std::vector<char>::iterator end { tempBuffer.begin() + data_received };

                    /* get pointer to offset */
                    char* bufferPointer = this->buffer + data_offset;

                    /* copy chunk into 'global' buffer */
                    std::memcpy(bufferPointer, tempBuffer, data_received);

                    /* update offset */
                    data_offset += static_cast<int>(data_received);
                    
                    first_data_received = true;

                    /* change state back to idle */
                    this->current_state = States::transmission_idle;
                }
                else
                {
                    /* if there's no additional data, end the transmission */
                    this->current_state = States::transmission_ended;
                }
            }
            break;
            case States::transmission_ended:
            {
                std::cout << "Transmission ended, filesize: " << data_offset << " bytes" << std::endl;
                
                /* switch to the write file state */
                this->current_state = States::write_file;
            }
            break;
            case States::write_file:
            {
                std::ofstream fileHandle;

                time_t rawtime = time(0);
                struct tm * timeinfo = localtime(&rawtime);

                char date_time_buf[80];

                time (&rawtime);
                timeinfo = localtime(&rawtime);

                /* stringify time */
                strftime(date_time_buf, 80, "%d-%m-%Y %H-%M-%S", timeinfo);

                std::string filename { date_time_buf };
                filename = "Screenshot_" + filename + ".plt";

                /* path where to save file */
                boost::filesystem::path path { this->directory / boost::filesystem::path(filename) };

                /* open file in binary mode */
                fileHandle.open(path, std::ios::out | std::ios::binary);

                /* check file handle */
                if(fileHandle)
                {
                    /* write buffer data into file */
                    fileHandle.write(this->buffer, data_offset);

                    fileHandle.flush();
                    std::cout << "File saved to " << path.string().c_str() << std::endl;
                    fileHandle.close();
                }
                else
                {
                    std::cout << "Couldn't save file !" << std::endl;
                }
                
                /* reset offset so cycle can start at the beginning */
                data_offset = 0;
                this->current_state = States::transmission_idle;
            }
            break;

            default:
                return;
            break;
        }
    }
}


void Job::setDirectory(std::string& directoryPath)
{
    /* new file path '/' concatenates two paths from boost::filesystem */
    boost::filesystem::path new_path {directoryPath / this->subpath};

    /* create subdirectory if it doesn't exist */
    if(!boost::filesystem::is_directory(this->directory))
    {
        /* create subdirectory */
        boost::filesystem::create_directory(this->directory);
    }

    std::cout << "Reconfigured destination directory from '" << this->directory.string().c_str() << " to '"<< new_path.string().c_str()<<"'" << std::endl;

    this->directory = new_path;
}

/// @brief 
/// @param buffer Reference to the buffer to fill by read_data
/// @param timeout time until method returns without reading data in milliseconds
/// @return size_t with the size of the real read data (zero if no data has been read)
size_t Job::read_data(char* buffer, size_t chunk_size, int timeout)
{
    /* initialize timeout for read buffer so data can be read in a non-blocking way */
    boost::asio::deadline_timer serial_timeout(this->io_service);

    /* return value, either 0 for timeout or > 0 for received data */
    std::size_t data_transfered { 0 };

    /* read callback function which returns if the timeout kicked (via error) or not bytes transfered */
    std::function<void(const boost::system::error_code&, std::size_t)> read_callback { 
        /* pass serial_timeout by reference so timer can be accessed */
        [&serial_timeout, &data_transfered](const boost::system::error_code& error, std::size_t bytes_transferred)
        {
            /* kill timeout deadline timer if data was received */
            if(bytes_transferred)
            {
                serial_timeout.cancel();
                data_transfered = bytes_transferred;
            }
            return;
        }
    };

    std::function<void(boost::asio::serial_port&, const boost::system::error_code&)> timeout_callback {
        /* argument serial_inst is the same as Job::serial and allows to cancel the async read */
        [](boost::asio::serial_port& serial_inst, const boost::system::error_code& error)
        {
            /* if no error happens but the timeout comes, cancel the async_read_some */
            if(!error)
            {
                serial_inst.cancel();
            }
            return;
        }
    };

    /* start async read */
    this->serial.async_read_some(boost::asio::buffer(buffer, chunk_size), boost::bind(read_callback, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));

    /* configures the timeout timer with the given timeout in milliseconds */
    serial_timeout.expires_from_now(boost::posix_time::milliseconds(timeout));

    /* configure deadline timer to call 'timeout_callback' with serial */
    serial_timeout.async_wait(boost::bind(timeout_callback, boost::ref(this->serial), boost::asio::placeholders::error));

    /* blocking instruction: this will block until one of the above callbacks returns */
    this->io_service.run();  

    /* reset the async io service so I can reuse it next time */
    this->io_service.reset();

    /* function returns simply the amount of data readback in the given timeout */
    return data_transfered;
}



Configuration Job::parseArgs(std::string& configstr)
{
    std::vector<std::string> argument_list;
    boost::split(argument_list, configstr, boost::is_any_of(":"));
    Configuration config {};

    if(argument_list.size() == argument_size)
    {
        /* TEK11801C:COM1:9600:8:N:1 */
        config.job_name = argument_list.at(0);
        config.port_name = argument_list.at(1);

        /* TODO: catch exceptions */
        config.baud_rate = sp_base::baud_rate(std::stoi(argument_list.at(2)));
        config.data_bits = sp_base::character_size(std::stoi(argument_list.at(3)));
        
        /* fetch parity information */
        if(argument_list.at(4) == "N")       { config.parity = sp_base::parity { sp_base::parity::none }; }
        else if (argument_list.at(4) == "O") { config.parity = sp_base::parity { sp_base::parity::odd  }; }
        else if (argument_list.at(4) == "E") { config.parity = sp_base::parity { sp_base::parity::even }; }
        else { throw std::invalid_argument( "wrong parity information in configuration string"); }

        /* fetch stop bit size */
        if(argument_list.at(5) == "1")       { config.stop_bits = sp_base::stop_bits { sp_base::stop_bits::one }; }
        else if (argument_list.at(4) == "1.5") { config.stop_bits = sp_base::stop_bits { sp_base::stop_bits::onepointfive }; }
        else if (argument_list.at(4) == "2") { config.stop_bits = sp_base::stop_bits { sp_base::stop_bits::two }; }
        else { throw std::invalid_argument( "wrong stop bit size in configuration string"); }
    }
    else
    {
        throw std::invalid_argument( "Configuration string has not all required elements '" + configstr + "'"  );
    }
    return config;
}
#include "device.hpp"

Device::Device(std::string configstr, int _timeout, bool end_on_eoj)
{
    this->parse_args(configstr);
    this->timeout = _timeout;
    
    /* default state is idle */
    this->current_state = Device::states::transmission_idle;

    std::cout << this->device_name << "\n";
    
    /* configure timeout timer */
    this->timer.expires_from_now(boost::posix_time::millisec(this->timeout));
    
    /*
    this->timer.async_wait(boost::bind(&Device::tick, this, boost::asio::placeholders::error()));
    this->io_service.run();*/

    /* setup serial port */
    this->serial.open(this->port_name);
    this->serial.set_option(baud_rate);
    this->serial.set_option(data_bits);
    this->serial.set_option(stop_bits);
    this->serial.set_option(parity);
    
    //while(true) {
    //    char data[BUFSIZE];
    //    size_t n = serial.read_some(boost::asio::buffer(data, BUFSIZE));
        // Write data to stdout
    //    std::cout.write(data, n);
    //}
}


void Device::read_callback(bool& data_available, size_t& size, boost::asio::deadline_timer& timeout, const boost::system::error_code& error, std::size_t bytes_transferred)
{
  if (error || !bytes_transferred)
  {
    // No data was read!
    data_available = false;
    return;
  }

  timeout.cancel();  // will cause wait_callback to fire with an error
  size = bytes_transferred;
  data_available = true;
}

void Device::wait_callback(boost::asio::serial_port& ser_port, const boost::system::error_code& error)
{
  if (error)
  {
    // Data was read and this timeout was canceled
    return;
  }

  ser_port.cancel();  // will cause read_callback to fire with an error
}

bool Device::read_data(char* data, size_t& size)
{
    char tempBuffer[4096];
    //size_t size = this->serial.read_some(boost::asio::buffer(tempBuffer));
    boost::asio::deadline_timer _timeout(this->io_service);
    bool data_received = false;

    this->serial.async_read_some(boost::asio::buffer(tempBuffer), boost::bind(&Device::read_callback, this, boost::ref(data_received), boost::ref(size), boost::ref(_timeout), 
                boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    _timeout.expires_from_now(boost::posix_time::milliseconds(100));
    _timeout.async_wait(boost::bind(&Device::wait_callback, this, boost::ref(this->serial), boost::asio::placeholders::error));

    this->io_service.run();  // will block until async callbacks are finished
    this->io_service.reset();

    memcpy(data, tempBuffer, size);

    return data_received;
}

void Device::execute()
{
    /* 'stop' is the break condition */
    while(this->current_state != Device::states::stop)
    {
        static int data_offset { 0 };
        static int real_size { 0 };

        switch(this->current_state)
        {
            case Device::states::transmission_idle:
            {
                //std::cout << "idle"<< std::endl;
                char tempBuffer[4096];
                bool data_received = false;
                size_t size {0};
                static bool first_data_received = false;

                data_received = this->read_data(tempBuffer, size);

                if(data_received)
                {
                    std::cout << "idle: received data of size: " << size << std::endl;
                    memcpy(this->buffer + data_offset, tempBuffer, size);
                    data_offset += static_cast<int>(size);
                    std::cout << "idle: data offset: "<< data_offset << std::endl;
                    first_data_received = true;
                }
                else if(first_data_received)
                {
                    this->current_state = Device::states::waiting_for_data_timer_start;
                    first_data_received = false;
                }
            }
            break;
            case Device::states::transmission_started:
            {
                std::cout << "transmission_started"<< std::endl;
                ;
            }
            break;
            case Device::states::waiting_for_data_timer_start:
            {
                std::cout << "waiting_for_data_timer_start"<< std::endl;
                /*this->timer.async_wait(boost::bind(&Device::tick, this, boost::asio::placeholders::error()));*/
                this->timer.async_wait(boost::bind(&Device::tick, this, boost::asio::placeholders::error()));
                this->io_service.run();
            }
            break;
            case Device::states::waiting_for_data_timer_ended:
            {
                std::cout << "waiting_for_data_timer_ended"<< std::endl;
                char tempBuffer[4096];
                bool data_received = false;
                size_t size {0};

                data_received = read_data(tempBuffer, size);
                if(data_received)
                {
                    std::cout << "idle: received data of size: " << size << std::endl;
                    memcpy(this->buffer + data_offset, tempBuffer, size);
                    data_offset += static_cast<int>(size);
                    std::cout << "idle: data offset: "<< data_offset << std::endl;
                    this->current_state = Device::states::transmission_idle;
                }
                else
                {
                    this->current_state = Device::states::transmission_ended;
                }
            }
            break;
            case Device::states::transmission_ongoing:
            {
                ;
            }
            break;
            case Device::states::transmission_ended:
            {
                std::cout << "transmission_ended" << std::endl;
                
                this->current_state = Device::states::write_file;
            }
            break;
            case Device::states::write_file:
            {
                std::cout << "write_file" << std::endl;
                std::ofstream wf("student.dat", std::ios::out | std::ios::binary);
                wf.write(this->buffer, data_offset);
                wf.close();

                data_offset = 0;
                this->current_state = Device::states::transmission_idle;
            }
            break;

            default:
                return;
            break;
        }
    }
}

void Device::tick(const boost::system::error_code& e) 
{
    std::cout << "tick" << std::endl;
    this->current_state = Device::states::waiting_for_data_timer_ended;
    // Reschedule the timer for 1 second in the future:
    //this->timer.expires_at(this->timer.expires_at() + boost::posix_time::seconds(1));
    // Posts the timer event
    //this->timer.async_wait(boost::bind(&Device::tick, this, boost::asio::placeholders::error()));
}


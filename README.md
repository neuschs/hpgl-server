# hpgl-server
This is a little piece of software to dump all arriving HPGL plots on a specified serial port to a directory.


If the software is started without a directory parameter the local working directory with the 'name' as subdirectory.

The syntax per command line is simple:

--device TEK11801C:COM10:19200:8:N:1

  Name:SerialPort:BaudRate:Bits:Parity:StopBits
  
--help Showing Help

--directory Directory where the hpgl dumps shall be located 


Currently there is one binary for windows but none for unix. The source code is built on the boost librarys and some msys2 packages so it should be compilable on unix systems.

# Disclaimer
I wrote this just for personal use and documented it only for myself.

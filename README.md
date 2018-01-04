ampc-server
====

Server part of ampc written in C, utilizing Websockets.
Based on ympd http://www.ympd.org

For the frontend code see: https://github.com/derhorst/ampc-client
![ScreenShot](https://raw.githubusercontent.com/derhorst/ampc-client/master/screenshot.png)

Dependencies
------------
 - libmpdclient 2: http://www.musicpd.org/libs/libmpdclient/
 - cmake 2.6: http://cmake.org/

Unix Build Instructions
-----------------------

1. install dependencies, cmake and libmpdclient are available from all major distributions.
2. create build directory ```cd /path/to/src; mkdir build; cd build```
3. create makefile ```cmake ..  -DCMAKE_INSTALL_PREFIX:PATH=/usr```
4. build ```make```
5. install ```sudo make install``` or just run with ```./ampc```

Run flags
---------
```
Usage: ./ampc [OPTION]...

 -h, --host <host>           connect to mpd at host [localhost]
 -p, --port <port>           connect to mpd at port [6600]
 -d, --music-directory <dir> Used to look up cover-art. Covers may be in
                             the same folder as the song and named
                             'folder' or 'cover' or embedded in the mp3 file
 -w, --webport [ip:]<port>   listen interface/port for webserver [8080]
 -u, --user <username>       drop priviliges to user after socket bind
 -V, --version               get version
 --help                      this help
```

SSL Support
-----------
To run ampc with SSL support:

- create a certificate (key and cert in the same file), example:
```
# openssl req -x509 -newkey rsa:2048 -keyout key.pem -out cert.pem -days 1000 -nodes
# cat key.pem cert.pem > ssl.pem
```
- tell ampc to use a webport using SSL and where to find the certificate:
```
# ./ampc -w "ssl://8081:/path/to/ssl.pem"
```

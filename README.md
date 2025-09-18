# HTTP Server

This is an extremely barebones `HTTP/1.1` web server built in C, using `sockets.h` and pretty much nothing else. 
The server binary runs on an [AWS EC2 VPS](https://aws.amazon.com/ec2/), running Ubuntu, which handles routing for me. For more details check [here](#aws-ec2-instance)


- It can serve `GET` requests for any files that exist in the working directory, and plans to support `HEAD` very soon (tm).
- It currently only supports `200 OK` and `404 ERROR` responses, the latter for anything that cannot be found in the working directory.
- It will soon have some css/javascript, but right now just serves a very plain `index.html`.

[IPv4 Address](http://3.105.0.153/)  // 
[URL (waiting on DNS to update)](http://lmeldrum.dev)


### STATUS:  
**DOWN.** Working on deployment/build process to the server. Will perhaps have a permanent binary running in the background soon.


# AWS EC2 INSTANCE:
**VPS Information:**
---

```python
Operating System: Ubuntu 24.04.3 LTS
          Kernel: Linux 6.14.0-1012-aws
    Architecture: arm64
 Hardware Vendor: Amazon EC2
  Hardware Model: t4g.small
Firmware Version: 1.0
   Firmware Date: Thu 2018-11-01
    Firmware Age: 6y 10month 2w 3d
```

Im currently building out the deployment, but it will most likely just be a makefile which runs some commands through ssh to build and deploy the binary on the server.


Credit to [this article](https://dev.to/jeffreythecoder/how-i-built-a-simple-http-server-from-scratch-using-c-739) for the inspiration.

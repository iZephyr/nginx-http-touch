nginx-http-touch
================

Description:
Nginx module,  that dynamic change the upstream's config

The Nginx Touch module enhances the standard round-robin load balancer provided
with dynamically change the configuration of round-robin upstream server, Include 
weight, max_fails, fail_timeout, down of the server
(backup is not available due to the ngx_http_upstream_rr_peers_t struct definition). 

Installation:
--

You'll need to re-compile Nginx from source to include this module.
Modify your compile of Nginx by adding the following directive
(modified to suit your path of course):

./configure --add-module=/absolute/path/to/nginx-http-touch 
make
make install


Usage:
--

Original config of nginx.conf

upstream upstreamname {

     server 10.1.3.1   weight=10 max_fails=2  fail_timeout=30s;
     
     server 10.1.3.2   weight=5 max_fails=2  fail_timeout=30s;
     
}

server{

		location /touch {
        
		    touch;
            
		}
        
}

1. Browse your Nginx config file's upstream block through url:
http://hostname/touch

Content of http response:
Worker id: pid
upstream name: upstreamname
10.1.3.1:80 weight=10, max_fails=2, fail_timeout=30, down=0, backup=0
10.1.3.2:80 weight=5, max_fails=2, fail_timeout=30, down=0, backup=0

2.Change your Nginx config file's upstream block through url:

http://hostname/touch?upstream=upstreamname&server=10.1.3.1:80&weight=79&max_fails=20&fail_timeout=38&down=1

Content of http response:
Worker id: pid
upstream name: upstreamname
10.1.3.1:80 weight=79, max_fails=20, fail_timeout=38, down=1, backup=0
10.1.3.2:80 weight=5, max_fails=2, fail_timeout=30, down=0, backup=0

If you encounter any issues, mail me : junsuilin@gmail.com

Contributing:
--
Git source repositories:
https://github.com/iZephyr/nginx-http-touch.git

Please feel free to fork the project at GitHub and submit pull requests.

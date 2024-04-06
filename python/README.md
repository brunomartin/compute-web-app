# Compute Web App in python


install necessary packages:
```
$ apt install -y libhdf5-dev libatlas-base-dev libopenjp2-7 libtiff5 python3-pip python3-venv
$ python3 -m pip install --user --upgrade pip
$ python3 -m pip install --user virtualenv
```

install necessary packages for CentOS 8:
```
$ yum install --enablerepo=PowerTools -y hdf5-devel atlas-devel openjpeg2 libtiff-devel python3 python3-pip
$ python3 -m pip install --user --upgrade pip
```

- clone this repos:
```
$ git clone https://framagit.org/nob/compute-web-app.git
```

run it:

```
$ python3 -m venv .venv
$ source .venv/bin/activate
$ pip install -r requirements.txt
$ python app.py
```

- package a standalone executable
```
$ pip install pyinstaller
$ pyinstaller app.spec
```

- quit virtual environment
```
$ deactivate
```

- create cwa user for cloning sources, build and run service
```
$ adduser --gecos "" cwa
```

- create a systemd service file:
```
$ nano /etc/systemd/system/cwa.service
$ cat /etc/systemd/system/cwa.service
[Unit]
Description=Compute Web App
After=network.target mnt-wibble.mount

[Service]
Type=simple
SyslogIdentifier=cwa
PermissionsStartOnly=true
User=cwa
WorkingDirectory=/home/cwa/compute-web-app/
ExecStart=/home/cwa/compute-web-app/.venv/bin/gunicorn -c etc/gunicorn.py 'cwa_app:create_app()' -e CWA_APP_DATA_PATH=/home/cwa/data
StandardOutput=journal+console
TimeoutSec=5
KillSignal=SIGINT

[Install]
WantedBy=multi-user.target
```

- enable and start service
```
$ systemctl enable cwa.service
$ systemctl start cwa.service
```

- if using different application data path, you will need to create a virtual environment .venv in that directory
```
$ cd /home/cwa/data/process_definition
$ python3 -m venv .venv
$ pip install -r requirement.txt
```


- create a nginx configuration for serving CWA
```
$ nano /etc/nginx/site-available/10.7.0.3.conf
$ cat /etc/nginx/site-available/10.7.0.3.conf
server {
    listen       443 ssl http2 default_server;
    listen       [::]:443 ssl http2 default_server;
    server_name  10.7.0.3;
    
    location / {
        proxy_pass http://127.0.0.1:8042;
    }

    ssl_certificate "/etc/letsencrypt/live/vps01.brunocsmartin.fr/cert.pem";
    ssl_certificate_key "/etc/letsencrypt/live/vps01.brunocsmartin.fr/privkey.pem";
    ssl_session_cache shared:SSL:1m;
    ssl_session_timeout  10m;
    ssl_ciphers PROFILE=SYSTEM;
    ssl_prefer_server_ciphers on;
}
```

- enable it and restart nginx
```
$ ln -s /etc/nginx/site-available/10.7.0.3.conf /etc/nginx/site-enabled/
$ systemctl restart nginx
```

- generated h5 file are in general in v110. Some application can only read v180. To convert them, use h5tool in result directory :
```
$ for f in *.h5;  do h5format_convert ${f}; done;
```
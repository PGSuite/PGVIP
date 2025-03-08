file_config  = /etc/pgsuite/pgvip.conf
file_service = /etc/systemd/system/pgvip.service
file_pgvip   = /usr/bin/pgvip

all: 
	gcc -w -std=gnu99 -pthread *.c util/*.c	-o pgvip

install:
	# Configuration
	if [ -z "$(subnet_mask)" ] ; then echo -e '\nUsage with parameters: make install ip_master=[ip_master] ip_standby=[ip_standby] ip_virtual=[ip_virtual] subnet_mask=[subnet_mask]\nExample: make install ip_master=192.168.56.1 ip_standby=192.168.56.2 ip_virtual=192.168.56.10 subnet_mask=255.255.255.0' ; exit 1; fi
	if [ -e $(file_config) ]   ; then echo -e '\nConfiguration file $(file_config) already exists, change it' ; exit 1; fi
	mkdir -p /etc/pgsuite
	cp pgvip.conf $(file_config)
	sed -i "s/\[ip_master\]/$(ip_master)/;s/\[ip_standby\]/$(ip_standby)/;s/\[ip_virtual\]/$(ip_virtual)/;s/\[subnet_mask\]/$(subnet_mask)/" $(file_config)
	# Log
	mkdir -p /var/log/pgsuite
	# Service
	if [ -e $(file_service) ] ; then echo -e '\nService file $(file_service) already exists, change it' ; exit 1; fi	
	cp pgvip.service $(file_service)
	rm -f $(file_pgvip)
	cp pgvip $(file_pgvip)
	systemctl daemon-reload
	echo -e '\npgvip service has been successfully created.\nUse command "systemctl --now enable pgvip" to enable and start service,\n"journalctl -fu pgvip" to view the journal'

uninstall:
	systemctl --now disable pgvip ; true
	rm -f $(file_config) $(file_service) $(file_pgvip)
	rm -f /var/log/pgsuite/pgvip.log*
	echo -e '\npgvip service has been successfully removed'

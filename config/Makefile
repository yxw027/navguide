install:
	if [ -d /etc/udev/rules.d ]; then \
	  cp -vf 10-fishbot.rules /etc/udev/rules.d; \
	  mkdir -p /etc/udev/scripts; \
	  cp -vf set_ftdi_low_latency.sh /etc/udev/scripts; \
	fi; \
	if [ -x /sbin/pam_console_apply ]; then \
	  /sbin/pam_console_apply; \
	fi;

clean:
	@echo "udev clean TODO"

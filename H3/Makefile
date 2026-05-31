CC = gcc
CFLAGS = -Wall -Wextra
LDFLAGS = -pthread -lmosquitto

TARGET = mqtt_bridge

all: $(TARGET)

$(TARGET): mqtt/mqtt_bridge.c
	$(CC) $(CFLAGS) -o mqtt/$(TARGET) mqtt/$(TARGET).c $(LDFLAGS)

cgi: cgi-bin/sensor_data.c cgi-bin/device_mgmt.c
	$(CC) $(CFLAGS) -o cgi-bin/sensor_data.cgi cgi-bin/sensor_data.c
	$(CC) $(CFLAGS) -o cgi-bin/device_mgmt.cgi cgi-bin/device_mgmt.c

install: all cgi
	install -d /usr/local/bin
	install mqtt/$(TARGET) /usr/local/bin/
	install -d /usr/lib/cgi-bin
	install cgi-bin/*.cgi /usr/lib/cgi-bin/
	install -d /var/www/html
	install html/* /var/www/html/

clean:
	rm -f mqtt/$(TARGET)
	rm -f cgi-bin/*.cgi

.PHONY: all cgi install clean

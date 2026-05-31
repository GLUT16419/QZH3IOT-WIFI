#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <json-c/json.h>

#define FRAME_HEADER_1 0xAA
#define FRAME_HEADER_2 0xBB
#define FRAME_TAIL_1 0xCC
#define FRAME_TAIL_2 0xDD

typedef struct {
    uint32_t device_id;
    uint8_t data_type;
    uint16_t data_length;
    uint8_t *data;
    uint16_t crc;
} SensorFrame;

uint16_t crc16(const uint8_t *data, int length) {
    uint16_t crc = 0xFFFF;
    for (int i = 0; i < length; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ (crc & 1 ? 0xA001 : 0);
        }
    }
    return crc;
}

int parse_frame(const uint8_t *buffer, int length, SensorFrame *frame) {
    if (length < 11) return -1;
    
    if (buffer[0] != FRAME_HEADER_1 || buffer[1] != FRAME_HEADER_2) {
        return -2;
    }
    
    if (buffer[length - 2] != FRAME_TAIL_1 || buffer[length - 1] != FRAME_TAIL_2) {
        return -3;
    }
    
    frame->device_id = (buffer[2] << 24) | (buffer[3] << 16) | (buffer[4] << 8) | buffer[5];
    frame->data_type = buffer[6];
    frame->data_length = (buffer[7] << 8) | buffer[8];
    
    uint16_t expected_crc = (buffer[length - 4] << 8) | buffer[length - 3];
    uint16_t actual_crc = crc16(buffer + 2, length - 6);
    
    if (expected_crc != actual_crc) {
        return -4;
    }
    
    frame->data = malloc(frame->data_length);
    memcpy(frame->data, buffer + 9, frame->data_length);
    
    return 0;
}

char *frame_to_json(const SensorFrame *frame) {
    struct json_object *root = json_object_new_object();
    struct json_object *sensors = json_object_new_array();
    
    char device_id_str[20];
    snprintf(device_id_str, sizeof(device_id_str), "STM32_%08X", frame->device_id);
    json_object_object_add(root, "device_id", json_object_new_string(device_id_str));
    json_object_object_add(root, "timestamp", json_object_new_int64(time(NULL)));
    
    struct json_object *sensor;
    switch (frame->data_type) {
        case 0x01: {
            float temp = *(float *)&frame->data[0];
            float humidity = *(float *)&frame->data[4];
            
            sensor = json_object_new_object();
            json_object_object_add(sensor, "type", json_object_new_string("temperature"));
            json_object_object_add(sensor, "value", json_object_new_double(temp));
            json_object_object_add(sensor, "unit", json_object_new_string("°C"));
            json_object_array_add(sensors, sensor);
            
            sensor = json_object_new_object();
            json_object_object_add(sensor, "type", json_object_new_string("humidity"));
            json_object_object_add(sensor, "value", json_object_new_double(humidity));
            json_object_object_add(sensor, "unit", json_object_new_string("%"));
            json_object_array_add(sensors, sensor);
            break;
        }
        case 0x02: {
            uint32_t lux = *(uint32_t *)frame->data;
            sensor = json_object_new_object();
            json_object_object_add(sensor, "type", json_object_new_string("light"));
            json_object_object_add(sensor, "value", json_object_new_int(lux));
            json_object_object_add(sensor, "unit", json_object_new_string("lx"));
            json_object_array_add(sensors, sensor);
            break;
        }
        case 0x03: {
            float hpa = *(float *)frame->data;
            sensor = json_object_new_object();
            json_object_object_add(sensor, "type", json_object_new_string("pressure"));
            json_object_object_add(sensor, "value", json_object_new_double(hpa));
            json_object_object_add(sensor, "unit", json_object_new_string("hPa"));
            json_object_array_add(sensors, sensor);
            break;
        }
        case 0x04: {
            uint8_t level = frame->data[0];
            sensor = json_object_new_object();
            json_object_object_add(sensor, "type", json_object_new_string("air_quality"));
            json_object_object_add(sensor, "value", json_object_new_int(level));
            json_object_object_add(sensor, "unit", json_object_new_string("level"));
            json_object_array_add(sensors, sensor);
            break;
        }
        default:
            sensor = json_object_new_object();
            json_object_object_add(sensor, "type", json_object_new_string("unknown"));
            json_object_object_add(sensor, "value", json_object_new_int(0));
            json_object_object_add(sensor, "unit", json_object_new_string(""));
            json_object_array_add(sensors, sensor);
            break;
    }
    
    json_object_object_add(root, "sensors", sensors);
    
    const char *json_str = json_object_to_json_string(root);
    char *result = strdup(json_str);
    json_object_put(root);
    
    return result;
}

void free_frame(SensorFrame *frame) {
    if (frame->data) {
        free(frame->data);
        frame->data = NULL;
    }
}
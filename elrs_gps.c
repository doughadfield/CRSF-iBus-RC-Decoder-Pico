
/*
 * elrs_gps.c
 *
 * Receives GPS data from a GPS module connected to the RP2040
 * and processes it for use in an ELRS (ExpressLRS) compatible telemetry system.
 */

#include "hardware/uart.h"
#include <stdio.h>
#include "crsf.h"
#include "pico/time.h"

#include <time.h>
#include "minmea.h"


// Global storage for the latest GPS data
struct
{
    int32_t lat;
    int32_t lon;
    uint16_t speed;
    uint16_t alt;
    bool has_fix;
} gps_data;

// Called when a full NMEA sentence is received
void parse_nmea_sentence(char *line)
{
    struct minmea_sentence_gga gga;
    if (minmea_parse_gga(&gga, line))
    {
        // Lat/Lon conversion
        gps_data.lat = (int32_t)(minmea_tocoord(&gga.latitude) * 10000000);
        gps_data.lon = (int32_t)(minmea_tocoord(&gga.longitude) * 10000000);

        // Altitude: gga.altitude is in meters
        gps_data.alt = (uint16_t)(minmea_tofloat(&gga.altitude) + 1000);
        gps_data.has_fix = (gga.fix_quality > 0);
    }

    struct minmea_sentence_rmc rmc;
    if (minmea_parse_rmc(&rmc, line))
    {
        // Speed: RMC returns knots, convert to km/h * 10
        // knots * 1.852 = km/h. * 10 for CRSF = * 18.52
        float speed_kph = minmea_tofloat(&rmc.speed) * 1.852f;
        gps_data.speed = (uint16_t)(speed_kph * 10);
    }
}

void handle_gps_uart()
{
    static char line_buffer[128];
    static int idx = 0;

    while (uart_is_readable(CRSF_UART_RX))  // Assuming GPS is connected to UART1 (CRSF_UART_TX)
    {
        char c = uart_getc(CRSF_UART_RX);
        if (c == '\n' || idx >= 127)
        {
            line_buffer[idx] = '\0';
            parse_nmea_sentence(line_buffer);
            idx = 0;
        }
        else
        {
            line_buffer[idx++] = c;
        }
    }
}


// CRC-8 calculation using polynomial 0xD5 (standard for CRSF)
uint8_t crc8_crsf(const uint8_t *data, uint8_t len) {
    uint8_t crc = 0;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x80) crc = (crc << 1) ^ 0xD5;
            else crc <<= 1;
        }
    }
    return crc;
}

void send_gps_telemetry(double lat, double lon, uint16_t speed_kph_10, uint16_t heading, uint16_t alt_m) {
    uint8_t buffer[18];
    int32_t lat_int = (int32_t)(lat * 10000000);
    int32_t lon_int = (int32_t)(lon * 10000000);

    // Frame Header
    buffer[0] = 0xC8; // Sync
    buffer[1] = 0x0F; // Length (Type + Payload = 1 + 14 = 15 bytes)
    buffer[2] = 0x02; // Type: GPS

    // Lat (4 bytes, Big Endian)
    buffer[3] = (lat_int >> 24) & 0xFF;
    buffer[4] = (lat_int >> 16) & 0xFF;
    buffer[5] = (lat_int >> 8) & 0xFF;
    buffer[6] = lat_int & 0xFF;

    // Lon (4 bytes, Big Endian)
    buffer[7] = (lon_int >> 24) & 0xFF;
    buffer[8] = (lon_int >> 16) & 0xFF;
    buffer[9] = (lon_int >> 8) & 0xFF;
    buffer[10] = lon_int & 0xFF;

    // Ground Speed (2 bytes, Big Endian)
    buffer[11] = (speed_kph_10 >> 8) & 0xFF;
    buffer[12] = speed_kph_10 & 0xFF;

    // Heading (2 bytes, Big Endian)
    buffer[13] = (heading >> 8) & 0xFF;
    buffer[14] = heading & 0xFF;

    // Altitude (2 bytes, Big Endian, 1000m offset)
    uint16_t alt_enc = alt_m + 1000;
    buffer[15] = (alt_enc >> 8) & 0xFF;
    buffer[16] = alt_enc & 0xFF;

    // CRC (Calculated on Type byte + Payload bytes)
    buffer[17] = crc8_crsf(&buffer[2], 15);

    // Transmit
    uart_write_blocking(uart1, buffer, 18);
}

void gps_send()
{
    static uint32_t last_telemetry_time = 0;
    uint32_t now = to_ms_since_boot(get_absolute_time());

    if (gps_data.has_fix && (now - last_telemetry_time > 200))
    { // 5Hz
        send_gps_telemetry(
        (double)gps_data.lat / 10000000.0, 
        (double)gps_data.lon / 10000000.0, 
        gps_data.speed, 
        0, // Heading (if not parsing RMC course, send 0)
        gps_data.alt - 1000 // Send raw meters back to function
                                    );
        last_telemetry_time = now;
    }
}

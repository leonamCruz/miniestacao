#include "l_dht22.h"
#include <stdio.h>
#include <wiringPi.h>

int read_dht22(int *temperature, int *humidity) {
    unsigned char data[5] = {0,0,0,0,0};
    int i, j;

    // Inicializa comunicação
    pinMode(DHT_PIN, OUTPUT);
    digitalWrite(DHT_PIN, LOW);
    delay(18);
    digitalWrite(DHT_PIN, HIGH);
    delayMicroseconds(40);

    pinMode(DHT_PIN, INPUT);

    // Espera resposta do sensor
    unsigned int timeout = 10000;
    while (digitalRead(DHT_PIN) == LOW && timeout--) if (timeout == 0) return -1;
    timeout = 10000;
    while (digitalRead(DHT_PIN) == HIGH && timeout--) if (timeout == 0) return -1;

    // Lê 5 bytes
    for (i = 0; i < 5; i++) {
        for (j = 0; j < 8; j++) {
            timeout = 10000;
            while (digitalRead(DHT_PIN) == LOW && timeout--) if (timeout == 0) return -1;

            unsigned int t = 0;
            while (digitalRead(DHT_PIN) == HIGH && t < 200) { delayMicroseconds(1); t++; }

            if (t > 40) data[i] |= (1 << (7 - j));
        }
    }

    // Verifica checksum
    if (data[4] != ((data[0]+data[1]+data[2]+data[3]) & 0xFF)) return -1;

    // Converte para int (multiplicado por 10 para precisão)
    *humidity = ((data[0] << 8) | data[1]);
    *temperature = ((data[2] & 0x7F) << 8 | data[3]);
    if (data[2] & 0x80) *temperature = -(*temperature);

    return 0;
}

#ifndef L_DHT22
#define L_DHT22

#define DHT_PIN 0  // GPIO 27 no WiringPi

// Leitura do DHT22 (em int, preserva manipulação de bits)
int read_dht22(int *temperature, int *humidity);

#endif

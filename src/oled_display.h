#ifndef OLEDDISPLAY_H
#define OLEDDISPLAY_H

#include <wiringPi.h>
#include <wiringPiI2C.h>

#define enderecoI2C 0x3C
#define comandoDisplay 0x00
#define dadoDisplay 0x40

// Funções para controle do display
void enviaComando(int fileDescriptor, unsigned char comando);
void enviaDado(int fileDescriptor, unsigned char dado);
void inicializaDisplay(int fileDescriptor);  // Recebe o fileDescriptor
void limpaDisplay(int fileDescriptor);  // Recebe o fileDescriptor
void escreveTexto(int fileDescriptor, const char *texto, int linha);  // Recebe o fileDescriptor

#endif

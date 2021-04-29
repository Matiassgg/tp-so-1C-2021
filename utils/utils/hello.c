#include "hello.h"

t_log* iniciar_logger(char* archivo_log, char* archivo_c) {
	return log_create(archivo_log, archivo_c, 1, LOG_LEVEL_INFO);
}

int contar_elementos_array(char** array) {
	int contador = 0;
	while (array[contador] != NULL) {
		contador++;
	}
	return contador;
}

int sonIguales(char* cadenaA, char* cadenaB) {
	return strcmp(cadenaA ,cadenaB) == 0;
}

// El buen helloWorld, nada le gana
void helloWorld() {
    puts("Hello world with shared!\n!");
}

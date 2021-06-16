#include "consola_mongo.h"

void leer_consola() {
	char* leido = readline(">");
	while (!son_iguales(leido, "\0")) {
		procesar_mensajes_en_consola_discordiador(string_split(leido, " "));
		free(leido);
		leido = readline(">");
	}
	free(leido);
}

void procesar_mensajes_en_consola_mongo(char** palabras_del_mensaje, int cantidad_argumentos) {

	//////////////////////////////////////////////////////////////////////////////////////

	if(son_iguales(palabras_del_mensaje[0] ,"ESTAS_ON")) {
		uint32_t socket_conexion;
		log_info(logger, "MONGO :: Preguntamos si esta on %s", palabras_del_mensaje[1]);

		if (son_iguales(palabras_del_mensaje[1] ,"Mi-RAM-HQ"))
			socket_conexion = crear_conexion(ip_Mi_RAM_HQ, puerto_Mi_RAM_HQ);
		else if(son_iguales(palabras_del_mensaje[1] ,"discordiador"))
			socket_conexion = crear_conexion(ip_discordiador, puerto_discordiador);

		t_paquete* paquete_a_enviar = crear_paquete(ESTA_ON);


		t_buffer* buffer = serializar_paquete(paquete_a_enviar);

		send(socket_conexion, buffer->stream, (size_t) buffer->size, 0);

		liberar_conexion(&socket_conexion);

		eliminar_paquete(paquete_a_enviar);
		eliminar_buffer(buffer);
	}

	//////////////////////////////////////////////////////////////////////////////////////

	if(son_iguales(palabras_del_mensaje[0] ,"OTRO")) {
		// NADA
	}

	//////////////////////////////////////////////////////////////////////////////////////

	if(son_iguales(palabras_del_mensaje[0] ,"MAS CASOS")) {
		// NADA
	}



}



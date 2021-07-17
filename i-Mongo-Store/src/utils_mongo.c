#include "utils_mongo.h"

void arrancar_servidor(void){
	pthread_t thread;
	uint32_t socket_servidor = iniciar_servidor(logger, ip_Mongo_Store, puerto_escucha);
	while(1){
		uint32_t socket_cliente = esperar_cliente(logger,socket_servidor);

		int *socket_para_pasar = malloc(sizeof(int));
		*socket_para_pasar = socket_cliente;

		pthread_create(&thread, NULL, (void*) serve_client, socket_para_pasar);
		pthread_detach(thread);
	}
}


void serve_client(int* socket_que_nos_pasan) {
	uint32_t socket = *socket_que_nos_pasan;
	free(socket_que_nos_pasan);

	int cod_op;

	while (1) {
		int i = recv(socket, &cod_op, sizeof(op_code), MSG_WAITALL);

		if (i != 4) {
			cod_op = -1;
			liberar_conexion(&socket);
			pthread_exit(NULL);
		}
		else {
			procesar_mensaje_recibido(cod_op, socket);
		}
	}

}


void procesar_mensaje_recibido(int cod_op, int cliente_fd) {

	uint32_t buffer_size;
	recv(cliente_fd, &buffer_size, sizeof(uint32_t), MSG_WAITALL);

	// logguear quien se me conecto: quiza hay que agregarle a los paquetes el nombre del modulo que envió el paquete, no lo sé
	// ES NECESARIO ALLOCAR MEMORIA A TODO?
	t_patota* patota/* = malloc(sizeof(t_patota))*/;
	t_tripulante* tripulante;
	t_tarea_Mongo* tarea/* = malloc(sizeof(tarea_Mongo))*/;
	t_mover_hacia* posicion/* = malloc(sizeof(t_mover_hacia))*/;
	t_bitacora_tarea_Mongo* tarea_bitacora;
	char* string_bitacora = string_new();
	//Procesar mensajes recibidos
	switch (cod_op) {
		case ESTA_ON:
			log_info(logger, "Estamos on");
			enviar_respuesta(OK,cliente_fd);
		break;
		case INICIAR_PATOTA:
			patota = deserializar_iniciar_patota(cliente_fd);

			log_info(logger, "Nos llego INICIAR_PATOTA de la patota %i", patota->id_patota);

//			crear_bitacoras_de_tripulantes(patota->cant_tripulantes);
//			GUARDAR EN FS Y HACER LAS TARES CORRESPONDIENTES
			list_destroy(patota->posiciones);
			free(patota->path_tareas);
			free(patota);
		break;
		case INICIAR_TRIPULANTE:
			tripulante = deserializar_iniciar_tripulante(cliente_fd);

			log_info(logger, "Nos llego INICIAR_TRIPULANTE del tripulante %i", tripulante->id);

			crear_archivo_bitacora(tripulante->id);

			free(tripulante);
		break;
		case TAREA_E_S:
			tarea = deserializar_tarea(cliente_fd);

			log_info(logger, "Nos llego TAREA_E_S del tripulante %i", tarea->id);

			verificar_archivo_tarea(tarea);

			free(tripulante);
		break;
		case MOVER_HACIA:
			posicion = deserializar_mover_hacia_posicion(cliente_fd);

			string_append_with_format(&string_bitacora, "El tripulante %i se mueve de %i|%i a %i|%i\n",posicion->id_tripulante, posicion->posicion_origen->pos_x, posicion->posicion_origen->pos_y, posicion->posicion_destino->pos_x, posicion->posicion_destino->pos_y);
			subir_a_bitacora(string_bitacora, posicion->id_tripulante);
		break;
		case EJECUTAR_TAREA:
			tarea_bitacora = deserializar_bitacora_tarea(cliente_fd);

			string_append_with_format(&string_bitacora, "El tripulante %i comienza ejecución de tarea %s\n", tarea_bitacora->id, tarea_bitacora->tarea);
			subir_a_bitacora(string_bitacora, tarea_bitacora->id);
		break;
		case FINALIZAR_TAREA:
			tarea_bitacora = deserializar_bitacora_tarea(cliente_fd);

			string_append_with_format(&string_bitacora, "El tripulante %i finaliza la tarea %s\n", tarea_bitacora->id, tarea_bitacora->tarea);
			subir_a_bitacora(string_bitacora, tarea_bitacora->id);
		break;
		case RESOLVER_SABOTAJE:

			string_append(&string_bitacora, "Se corre en pánico hacia la ubicación del sabotaje\n");
			subir_a_bitacora(string_bitacora, tarea_bitacora->id);
		break;
		case FINALIZAR_SABOTAJE:

			string_append(&string_bitacora, "Se resuelve el sabotaje\n");
			subir_a_bitacora(string_bitacora, tarea_bitacora->id);
		break;
	}

	free(string_bitacora);

}

void subir_a_bitacora(char* informacion,uint32_t id_tripulante){
	char* path = path_bitacora_tripulante(id_tripulante);
	if(!archivo_existe(path))
		log_error(logger,"No se pudo abrir el archivo %s para su escritura", path);
	else{
		log_info(logger,"%s", informacion);
		subir_FS(informacion, path, false);
	}

	free(path);
}

void verificar_archivo_tarea(t_tarea_Mongo* tarea){
	if(son_iguales(tarea->tarea,"GENERAR_OXIGENO") || son_iguales(tarea->tarea,"CONSUMIR_OXIGENO")){
		if(archivo_recursos_existe("Oxigeno.ims"))
			procesar_tarea(tarea);
		else{
			procesar_falta_archivo(tarea,"Oxigeno.ims");
			procesar_tarea(tarea);
		}
	} else if(son_iguales(tarea->tarea,"GENERAR_COMIDA") || son_iguales(tarea->tarea,"CONSUMIR_COMIDA")){
		if(archivo_recursos_existe("Comida.ims"))
			procesar_tarea(tarea);
		else
			procesar_falta_archivo(tarea,"Comida.ims");
	} else if(son_iguales(tarea->tarea,"GENERAR_BASURA") || son_iguales(tarea->tarea,"DESCARTAR_BASURA")){
		if(archivo_recursos_existe("Basura.ims"))
			procesar_tarea(tarea);
		else
			procesar_falta_archivo(tarea,"Basura.ims");
	}

}

void procesar_tarea(t_tarea_Mongo* tarea){
	char* nombre_tarea = tarea->tarea;
	uint32_t cantidad = tarea->parametro; // [Consulta]¿Este parametro es la cantidad que menciona el enunciado, o es otra cosa?
	if(string_contains(nombre_tarea,"GENERAR")){
		agregar_caracteres_llenado_segun_tarea(nombre_tarea,cantidad);
	}
	else if(string_contains(nombre_tarea,"CONSUMIR")){
		eliminar_caracteres_llenado_segun_tarea(nombre_tarea,cantidad);
	}
	else if(son_iguales(nombre_tarea,"DESCARTAR_BASURA")){
		eliminar_archivo("Basura.ims");
	}
}

void agregar_caracteres_llenado_segun_tarea(char* nombre_tarea, uint32_t cantidad){
	if(string_contains(nombre_tarea,"OXIGENO")){
		agregar_caracteres_llenado_a_archivo('O',cantidad,"Oxigeno.ims");
	}
	else if(string_contains(nombre_tarea,"COMIDA")){
		agregar_caracteres_llenado_a_archivo('C',cantidad,"Comida.ims");
	}
	else if(string_contains(nombre_tarea,"BASURA")){
		agregar_caracteres_llenado_a_archivo('B',cantidad,"Basura.ims");
	}
}

char obtener_caracter_llenado(char* nombre_tarea){
	if(string_contains(nombre_tarea,"OXIGENO")){
		return 'O';
	}
	else if(string_contains(nombre_tarea,"COMIDA")){
		return 'C';
	}
	else if(string_contains(nombre_tarea,"BASURA")){
		return 'B';
	}
}

void agregar_caracteres_llenado_a_archivo(char caracter, uint32_t cantidad, char* archivo){
	char* ruta_archivo = obtener_path_files(archivo);
	if(!archivo_existe(ruta_archivo)){
		log_error(logger,"No se pudo abrir el archivo %s para su escritura", archivo);
	}else{
		char* caracteres = string_repeat(caracter,cantidad);
		log_info(logger,"Se va a llenar el archivo %s con %s", ruta_archivo, caracteres);
		subir_FS(caracteres, ruta_archivo, true);
//		fwrite(caracteres,string_length(caracteres)+1,1,file);
	}


//	subir_tarea(tarea, fd);
}

void eliminar_caracteres_llenado_segun_tarea(char* nombre_tarea, uint32_t cantidad){
	if(string_contains(nombre_tarea,"OXIGENO")){
		eliminar_caracteres_llenado_a_archivo('O',cantidad,"Oxigeno.ims");
	}
	else if(string_contains(nombre_tarea,"COMIDA")){
		eliminar_caracteres_llenado_a_archivo('C',cantidad,"Comida.ims");
	}
}

void eliminar_caracteres_llenado_a_archivo(char caracter, uint32_t cantidad, char* archivo){
	uint32_t cantidad_caracteres = cantidad_caracteres_archivo(caracter, archivo);
	if (cantidad >= cantidad_caracteres){
		vaciar_archivo(archivo);
		log_warning(logger,"Se quisieron eliminar mas caracteres de los existentes en %s",archivo);
	}
	else{
		//Eliminar caracteres del archivo.
		quitar_caracteres_a_archivo(caracter,cantidad_caracteres,cantidad,archivo);
	}
}

uint32_t cantidad_caracteres_archivo(char caracter, char* archivo){

	char* ruta_completa_archivo = obtener_path_files(archivo);
	FILE * file = fopen(ruta_completa_archivo, "rb");
	if(file==NULL)
		return -1;
	uint32_t cantidad = 0;
	char c;
	while(!feof(file)){
		fread(&c,sizeof(char),1,file);
		if(c==caracter)
			cantidad++;
	}
	fclose(file);
	log_info(logger, "El archivo %s tiene %d veces el caracter de llenado %c",archivo,cantidad,caracter);

	return cantidad;
}

int quitar_caracteres_a_archivo(char caracter, uint32_t cantidad_actual, uint32_t cantidad_a_sacar,char* archivo){
	char* ruta_completa_archivo = obtener_path_files(archivo);
	FILE * file = fopen(ruta_completa_archivo, "wb");
	if(file==NULL){
		log_warning(logger, "Error al abrir archivo %s cuando se quiso quitarle caracteres %c", archivo, caracter);
		return -1;
	}
	int nueva_cantidad = cantidad_actual-cantidad_a_sacar;
	char* caracteres = string_repeat(caracter,nueva_cantidad);
	fwrite(caracteres,string_length(caracteres)+1,1,file);
	fclose(file);
	log_info(logger, "El archivo %s se le quitaron %d caracteres %c. De %d paso a %d",archivo,cantidad_a_sacar,caracter,cantidad_actual,nueva_cantidad);
	return 1;
}


void procesar_falta_archivo(t_tarea_Mongo* tarea,char* archivo){
	char* nombre_tarea = tarea->tarea;
	if(string_contains(nombre_tarea,"GENERAR")){
		if(crear_archivo_recursos(archivo, obtener_caracter_llenado(tarea->tarea))){
			log_info(logger, "El archivo %s se creo correctamente",archivo);
		}
	}
	else if(string_contains(nombre_tarea,"CONSUMIR")){
		informar_falta_archivo(tarea, archivo);
	}
	else if(son_iguales(nombre_tarea,"DESCARTAR_BASURA")){
		log_error(logger, "El archivo Basuras.ims no existe. Finalizando tarea DESCARTAR_BASURA");
	}
}

void informar_falta_archivo(t_tarea_Mongo* tarea, char* archivo){
	//ACA HAY QUE AVISARLE AL TRIPULANTE A CARGO DE LA TAREA Y FINALIZAR TAREA. TIENE QUE ESPERAR EN COLA DE BLOQUEADO.
	log_error(logger, "El archivo %s no existe. Finalizando tarea %s", archivo, tarea->tarea);

}



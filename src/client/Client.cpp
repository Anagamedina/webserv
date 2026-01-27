#include "CLient.hpp"


//MOTOR DE ENTRADA  
//ESTE METODO SE LLAMARA CUANDO EPOLL NOS AVISE CUANDO HAY EPOLLIN

void Client::handleRead()
{
    char buffer; //buffer temporal

    //1) Recibir datos crudos
    //2)actualizar tiempo para evitar timeout 
    //3) anadir al buffer de procesamiento 
    //4) invocar al parser (la logia que ya esta hecha)
    //5)verificar si el parser termino 

}

//MOTOR DE SALIDA
//ES EL PUNTO CRITICO DE LOS SERVIDORES NO BLOQUEANTES 
//SEND() NO VA ENVAIR Todo EL VECTOR DE UNA VEZ

void Client::handleWrite() {



    //1)Intentar enviar lo que queda en el buffer
    //2)hemos terminado de enviar todo?
    //2.1)respuesta completa? cerrar conexion o keep alive
    


}


void Client::buildResponse() {
    //depende del status que tengas respondemos una cosa u otra...
    //serializar a raw bytes para el envio 
}


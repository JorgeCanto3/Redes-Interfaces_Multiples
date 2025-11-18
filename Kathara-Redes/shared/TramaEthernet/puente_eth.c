/*Escucha todos los paquetes ethernet que llegan, pero se*/
/*desea el que le corresponde*/
#include "eth.h"

 int main(int argc, char *argv[])
 {
   int sockfd, i, iTramaLen;
   ssize_t numbytes;
   byte sbBufferEtherSend[BUF_SIZ], sbMac[6];
   byte sbBufferEther[BUF_SIZ];
   /*La cabecera Ethernet (eh) y sbBufferEther apuntan a lo mismo*/
 
   struct ether_header *psehHeaderEther = (struct ether_header *)sbBufferEther;
   int iLen, iLenHeader, iLenTotal, iIndex, iIndex2;
    struct sockaddr_ll socket_address;
   int saddr_size; 
   
   struct sockaddr saddr;    
   struct ifreq sirDatos, sirDatos_Ad;
   int iEtherType;
   char mv_name[3];
   char scMsj[] = "Ahi te va mi mac :) No te pases de alberca!!!";

   if (argc<3)
   {
     printf ("Error en argumentos.\n\n");
     printf ("ethdump INTERFACE\n");
     printf ("Ejemplo: recv_eth eth0 pcX\n\n");
     exit (1);
   }
   /*Apartir de este este punto, argv[1] = Nombre de la interfaz.          */
   
   /*Podriamos recibir tramas de nuestro "protocolo" o de cualquier protocolo*/
   /*sin embargo, evitaremos esto y recibiremos de todo. Hay que tener cuidado*/
   /*if ((sockfd = socket(PF_PACKET, SOCK_RAW, htons(ETHER_TYPE))) == -1)*/
   /*Se abre el socket para "escuchar" todo sin pasar por al CAR*/
   if ((sockfd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) == -1)  // sockdf tiene la ubicacion del socket
   {
     perror("Listener: socket"); 
     return -1;
   }


   /*Ahora obtenemos la MAC de la interface del host*/
   memset(&sirDatos, 0, sizeof(struct ifreq));

   for (i=0; argv[1][i]; i++) sirDatos.ifr_name[i] = argv[1][i];

   for (i=0; argv[2][i]; i++) sirDatos_Ad.ifr_name[i] = argv[2][i];

   for (i=0; argv[3][i]; i++) mv_name[i] = argv[3][i];
   
   /* Get interface index */
   if (ioctl(sockfd, SIOCGIFINDEX, &sirDatos) < 0) perror("SIOCGIFINDEX");
   iIndex = sirDatos.ifr_ifindex;

   if (ioctl(sockfd, SIOCGIFHWADDR, &sirDatos_Ad) < 0) perror("SIOCGIFHWADDR");
   iIndex2 = sirDatos_Ad.ifr_ifindex;
   
   /*Se imprime la MAC del host*/
   printf ("Direccion MAC de la interfaz de entrada: %02x:%02x:%02x:%02x:%02x:%02x\n",
           (byte)(sirDatos.ifr_hwaddr.sa_data[0]), (byte)(sirDatos.ifr_hwaddr.sa_data[1]),
           (byte)(sirDatos.ifr_hwaddr.sa_data[2]), (byte)(sirDatos.ifr_hwaddr.sa_data[3]),
           (byte)(sirDatos.ifr_hwaddr.sa_data[4]), (byte)(sirDatos.ifr_hwaddr.sa_data[5])); 
   
   /*Se mantiene en escucha*/ 
   do 
   { /*Capturando todos los paquetes*/   
       saddr_size = sizeof saddr;
       iTramaLen = recvfrom(sockfd, sbBufferEther, BUF_SIZ, 0, &saddr, (socklen_t *)(&saddr_size));
     
      char* mac = iSacarMAC(sbBufferEther);

      if (isBroadcast(sbBufferEther) && isParaMi(sbBufferEther, mv_name)) {
        printf("\nRecibido broadcast solicitando MAC para %s\n", mv_name);
        
        // Guardamos la MAC origen del broadcast
        struct ether_header *ehRecibido = (struct ether_header *)sbBufferEther;
        byte *macOrigen[6];
        memcpy(macOrigen, ehRecibido->ether_shost, 6);
        
        // Limpiamos el buffer para la respuesta
        memset(sbBufferEther, 0, BUF_SIZ);  // <-- pa q te hago funciones si no las usas? corazon

        // Configuramos nuestra MAC como origen

        //---------------------- Mismo chow aqui, te hice como 4 funciones de COnfiguraciones ogeis-------------
        /*
        psehHeaderEther->ether_shost[0] = (byte)(sirDatos.ifr_hwaddr.sa_data[0]);
        psehHeaderEther->ether_shost[1] = (byte)(sirDatos.ifr_hwaddr.sa_data[1]);
        psehHeaderEther->ether_shost[2] = (byte)(sirDatos.ifr_hwaddr.sa_data[2]);
        psehHeaderEther->ether_shost[3] = (byte)(sirDatos.ifr_hwaddr.sa_data[3]);
        psehHeaderEther->ether_shost[4] = (byte)(sirDatos.ifr_hwaddr.sa_data[4]);
        psehHeaderEther->ether_shost[5] = (byte)(sirDatos.ifr_hwaddr.sa_data[5]);

        // Configuramos la MAC destino
        memcpy(psehHeaderEther->ether_dhost, macOrigen, 6); // <--------------------- fakin sameee  
        */
        configurarOrigen_Ether(psehHeaderEther, &sirDatos);
        configurarDestino_Ether(psehHeaderEther, macOrigen);
        // Configuramos el socket_address una sola vez

        // ---------------- Pero falta agregar el broadcast como  destino, no? pq tas mandando de ti pal otro buei  --------------

        memset(&socket_address, 0, sizeof(struct sockaddr_ll));
        socket_address.sll_family = AF_PACKET;
        socket_address.sll_protocol = htons(ETH_P_ALL);
        socket_address.sll_ifindex = iIndex;
        socket_address.sll_halen = ETH_ALEN;
       
        //memcpy(socket_address.sll_addr, macOrigen, 6);
        configurarDestino_Socket(&socket_address, macOrigen);

        // Configuramos el payload
        iLenHeader = sizeof(struct ether_header);
        for (i=0; ((scMsj[i])&&(i<ETHER_TYPE)); i++) 
            sbBufferEther[iLenHeader+i] = scMsj[i];

        // Rellenamos con espacios si es necesario
        while (i<ETHER_TYPE) {
            sbBufferEther[iLenHeader+i] = ' ';
            i++;
        }

        iLenHeader = iLenHeader + i;   
        psehHeaderEther->ether_type = htons(ETHER_TYPE); 

        // FCS
        for (i=0; i<4; i++) 
            sbBufferEther[iLenHeader+i] = 0;
        iLenTotal = iLenHeader + 4;

        printf("MAC origen (nuestra): %02x:%02x:%02x:%02x:%02x:%02x\n",
          (byte)(sirDatos.ifr_hwaddr.sa_data[0]),
          (byte)(sirDatos.ifr_hwaddr.sa_data[1]),
          (byte)(sirDatos.ifr_hwaddr.sa_data[2]),
          (byte)(sirDatos.ifr_hwaddr.sa_data[3]),
          (byte)(sirDatos.ifr_hwaddr.sa_data[4]),
          (byte)(sirDatos.ifr_hwaddr.sa_data[5]));

        printf("Enviando respuesta a MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
          macOrigen[0], macOrigen[1], macOrigen[2],
          macOrigen[3], macOrigen[4], macOrigen[5]);

        iLen = sendto(sockfd, sbBufferEther, iLenTotal, 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll));
        
            if (iLen < 0) {
                perror("Error enviando respuesta");
            } else {
                printf("Respuesta enviada correctamente\n");
            }
        }

     /*Recibe todo lo que llegue. Llegara el paquete a otras capas dentro del host?*/   
     if (iLaTramaEsParaMi(sbBufferEther, &sirDatos))
     {
       printf ("\nContenido de la trama recibida:\n");    
       vImprimeTrama (sbBufferEther);
     }


     if (!isParaMi(sbBufferEther, mv_name) && !isBroadcast(sbBufferEther))
     {
        
        byte sbBufferEtherR[BUF_SIZ];
        struct ether_header *psehHeaderEtherR = (struct ether_header *)sbBufferEtherR;
        sbBufferEtherR = sbBufferEther;

        configurarBroadcast_Ether(psehHeaderEther);
        configurarBroadcast_Socket(&socket_address);
        
        char *mensaje = "pc5";
        configurarTrama(sbBufferEther, psehHeaderEther, &sirDatos_Ad, sbMac, mensaje ,&iLenHeader, iIndex2, &iLenTotal,sockfd, &socket_address);
        printf("\nBROADCAST buscando pc5...\n");
        iLen = sendto(sockfd, sbBufferEther, iLenTotal, 0, (struct sockaddr *)&socket_address, sizeof(struct sockaddr_ll));
        if (iLen < 0)
        {
          perror("sendto");
          exit(1);
        }
        
        printf("Trama reenviada como broadcast.\n");

        printf("Esperando MAC pc5...\n");

        memset(sbBufferEther, 0, BUF_SIZ);// <--- Pa eso hice la de ReiniciarTrama
        
        memset(&socket_address, 0, sizeof(struct sockaddr_ll)); // <--- Pa q es?

        iTramaLen = recvfrom(sockfd, sbBufferEther, BUF_SIZ, 0, &saddr, (socklen_t *)(&saddr_size));
        if (iTramaLen < 0) {
          perror("recvfrom");
          exit(1);
        }

        mac = iSacarMAC_Trama(sbBufferEther);
        printf("MAC pc5 recibida: %s\n", mac);

        configurarDestino_Ether(psehHeaderEtherR, sbMac);
        configurarDestino_Socket(&socket_address, mac);
        configurarOrigen_Ether(psehHeaderEther, &sirDatos);

        configurarTrama(sbBufferEtherR, psehHeaderEther, &sirDatos, sbMac, scMsj,&iLenHeader,  iIndex, &iLenTotal, sockfd, &socket_address);

        printf("Reenviando trama a pc5...\n");
        iLen = sendto(sockfd, sbBufferEtherR, iLenTotal, 0, (struct sockaddr *)&socket_address, sizeof(struct sockaddr_ll));
        
        if (iLen < 0)
        {
          perror("sendto");
          exit(1);
        }

        printf("Trama reenviada a pc5.\n");

        free(mac);


     }
     
   } while (1);


   
   close(sockfd); 
   return (0);
 }
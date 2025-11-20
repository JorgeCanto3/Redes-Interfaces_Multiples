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
   char scMsj[100] ;
   char *mac;

   if (argc<4)
   {
     printf ("Error en argumentos.\n\n");
     printf ("ethdump INTERFACE\n");
     printf ("Ejemplo: puente_eth eth0 eht1 pcX \n\n");
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

  memset(&sirDatos, 0, sizeof(sirDatos));
  memset(&sirDatos_Ad, 0, sizeof(sirDatos_Ad));

   /*Ahora obtenemos la MAC de la interface del host*/
   memset(&sirDatos, 0, sizeof(struct ifreq));

   for (i=0; argv[1][i]; i++) sirDatos.ifr_name[i] = argv[1][i];

   for (i=0; argv[2][i]; i++) sirDatos_Ad.ifr_name[i] = argv[2][i];

   for (i=0; argv[3][i]; i++) mv_name[i] = argv[3][i];
   


   /* Obtenemos los Indices de ambas Interfaces junto con la MAC de cada interfaz */
   if (ioctl(sockfd, SIOCGIFINDEX, &sirDatos) < 0) perror("SIOCGIFINDEX");
   iIndex = sirDatos.ifr_ifindex;

   if (ioctl(sockfd, SIOCGIFHWADDR, &sirDatos) < 0) perror("SIOCGIFHWADDR");

   if (ioctl(sockfd, SIOCGIFINDEX, &sirDatos_Ad) < 0) perror("SIOCGIFINDEX");
   iIndex2 = sirDatos_Ad.ifr_ifindex;

   if (ioctl(sockfd, SIOCGIFHWADDR, &sirDatos_Ad) < 0) perror("SIOCGIFHWADDR");
   
   /*Se imprime la MAC del host*/
   printf ("Direccion MAC de la interfaz 1 de entrada: %02x:%02x:%02x:%02x:%02x:%02x\n",
           (byte)(sirDatos.ifr_hwaddr.sa_data[0]), (byte)(sirDatos.ifr_hwaddr.sa_data[1]),
           (byte)(sirDatos.ifr_hwaddr.sa_data[2]), (byte)(sirDatos.ifr_hwaddr.sa_data[3]),
           (byte)(sirDatos.ifr_hwaddr.sa_data[4]), (byte)(sirDatos.ifr_hwaddr.sa_data[5])); 
   

   printf ("Direccion MAC de la interfaz 2 de entrada: %02x:%02x:%02x:%02x:%02x:%02x\n",
           (byte)(sirDatos_Ad.ifr_hwaddr.sa_data[0]), (byte)(sirDatos_Ad.ifr_hwaddr.sa_data[1]),
           (byte)(sirDatos_Ad.ifr_hwaddr.sa_data[2]), (byte)(sirDatos_Ad.ifr_hwaddr.sa_data[3]),
           (byte)(sirDatos_Ad.ifr_hwaddr.sa_data[4]), (byte)(sirDatos_Ad.ifr_hwaddr.sa_data[5])); 
   
   /*Se mantiene en escucha*/ 
   do 
   { /*Capturando todos los paquetes*/   
       saddr_size = sizeof saddr;
      memset(sbBufferEther, 0, BUF_SIZ);  

       iTramaLen = recvfrom(sockfd, sbBufferEther, BUF_SIZ, 0, &saddr, (socklen_t *)(&saddr_size));
       
       
       if (isBroadcast(sbBufferEther) && isParaMi(sbBufferEther, mv_name)) {
        printf("\n\n---------- Nuevo Broadcast Recibido --------------\n" );      
        printf("\n\nRecibido broadcast solicitando MAC para %s\n", mv_name);
        mac = iSacarMAC(sbBufferEther);
        
        // Guardamos la MAC origen del broadcast
        struct ether_header *ehRecibido = (struct ether_header *)sbBufferEther;
        byte macOrigen[LEN_MAC];
        memcpy(macOrigen, ehRecibido->ether_shost, 6);
        
        // Limpiamos el buffer para la respuesta
        memset(sbBufferEther, 0, BUF_SIZ);  

        // Configuramos nuestra MAC como origen

        /*
        psehHeaderEther->ether_shost[0] = (byte)(sirDatos.ifr_hwaddr.sa_data[0]);
        psehHeaderEther->ether_shost[1] = (byte)(sirDatos.ifr_hwaddr.sa_data[1]);
        psehHeaderEther->ether_shost[2] = (byte)(sirDatos.ifr_hwaddr.sa_data[2]);
        psehHeaderEther->ether_shost[3] = (byte)(sirDatos.ifr_hwaddr.sa_data[3]);
        psehHeaderEther->ether_shost[4] = (byte)(sirDatos.ifr_hwaddr.sa_data[4]);
        psehHeaderEther->ether_shost[5] = (byte)(sirDatos.ifr_hwaddr.sa_data[5]);

        // Configuramos la MAC destino
        memcpy(psehHeaderEther->ether_dhost, macOrigen, 6);   
        */

        configurarOrigen_Ether(psehHeaderEther, &sirDatos);
        configurarDestino_Ether(psehHeaderEther, macOrigen);
        // Configuramos el socket_address una sola vez

        // Limpiamos el mensaje para evitar basura 
        memset(scMsj, 0, sizeof(scMsj));
        strcpy(scMsj,"Te mando mi MAC!" );


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

        printf("Enviando mensaje de respuesta ante el broadcast... ");

        iLen = sendto(sockfd, sbBufferEther, iLenTotal, 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll));
        printf("\n\n---------- Nuevo Mensaje Enviado a la Interfaz ETH0 --------------\n" );      

        
            if (iLen < 0) {
                perror("Error enviando respuesta");
            } else {
                printf("Respuesta enviada correctamente\n");
            }
        }

     /*Recibe todo lo que llegue. Llegara el paquete a otras capas dentro del host?*/   
     if (iLaTramaEsParaMi(sbBufferEther, &sirDatos))
     {
      printf("\n\n---------- Nuevo Mensaje Recibido --------------\n" );      

       printf ("\nContenido de la trama recibida: \n");    
       vImprimeTrama (sbBufferEther);
     }

     
     if (!isParaMi(sbBufferEther, mv_name) && iLaTramaEsParaMi(sbBufferEther, &sirDatos))     
     {
       printf("\n\n---------- Nuevo Mensaje para ETH1 --------------\n" );      
        
        byte sbBufferEtherR[BUF_SIZ];
        struct ether_header *psehHeaderEtherR = (struct ether_header *)sbBufferEtherR;
        for (int i = 0; i < BUF_SIZ; i++)
        {
          sbBufferEtherR[i] = sbBufferEther[i];
        }

        memset(scMsj, 0, sizeof(scMsj));
        strcpy(scMsj,sacarNombreInterfaz(sbBufferEther) );
        char NoInterfazOrigen = sbBufferEther[TRAMA_PAYLOAD+4];

        char NoInterfaz =scMsj[2];

        reinciarTrama(sbBufferEther);        
        psehHeaderEther = (struct ether_header *)sbBufferEther;
        


        configurarTrama_Broadcast(sbBufferEther, psehHeaderEther, &sirDatos_Ad, scMsj ,&iLenHeader, iIndex2, &iLenTotal,sockfd, &socket_address);
        printf("\nBROADCAST buscando pc%c...\n", scMsj[2]);

        iLen = sendto(sockfd, sbBufferEther, iLenTotal, 0, (struct sockaddr *)&socket_address, sizeof(struct sockaddr_ll));
        
        printf("\n\n----------  Mensaje Enviado a la Interfaz ETH1 --------------\n" );      
        
        if (iLen < 0)
        {
          perror("sendto");
          exit(1);
        }
        
        printf("Trama reenviada como broadcast.\n");

        printf("Esperando MAC pc%c...\n",scMsj[2]);

        reinciarTrama (sbBufferEther);
        psehHeaderEther = (struct ether_header *)sbBufferEther;

        
        memset(&socket_address, 0, sizeof(struct sockaddr_ll));

        iTramaLen = recvfrom(sockfd, sbBufferEther, BUF_SIZ, 0, &saddr, (socklen_t *)(&saddr_size));
        printf("\n\n---------- Nuevo Mensaje Recibido de la Interfaz ETH1 --------------\n" );      

        if (iTramaLen < 0) {
          perror("recvfrom");
          exit(1);
        }

        memset(mac, 0, sizeof(mac));
        mac = iSacarMAC(sbBufferEther);

        char busqueda[] = "Te habla pcX";
        busqueda[11] = NoInterfazOrigen;

        memset(scMsj, 0, sizeof(scMsj));
        strcpy(scMsj,busqueda);
        configurarTrama(sbBufferEtherR, psehHeaderEtherR, &sirDatos_Ad, mac, scMsj,&iLenHeader,  iIndex2, &iLenTotal, sockfd, &socket_address);

        printf("Reenviando trama a pc%c...\n", NoInterfaz);
        iLen = sendto(sockfd, sbBufferEtherR, iLenTotal, 0, (struct sockaddr *)&socket_address, sizeof(struct sockaddr_ll));
        printf("\n\n----------  Mensaje Enviado a la Interfaz ETH1 --------------\n" );      
        
        if (iLen < 0)
        {
          perror("sendto");
          exit(1);
        }

        printf("Trama reenviada a pc%c.\n", NoInterfaz);

        free(mac);


     }
     
   } while (1);


   
   close(sockfd); 
   return (0);
 }  
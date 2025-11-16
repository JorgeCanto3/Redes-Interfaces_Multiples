#include "eth.h"

 int main(int argc, char *argv[])
 {
   int sockfd;
   /*Esta estructura permite solicitar datos a la interface*/
   struct ifreq sirDatos;
   int i, iLen, iLenHeader, iLenTotal, iIndex;
   int iTramaLen;
   /*Buffer en donde estara almacenada la trama*/
   byte sbBufferEther[BUF_SIZ], sbMac[6];
   /*Para facilitar, hay una estructura para la cabecera Ethernet*/
   /*La cabecera Ethernet (psehHeaderEther) y sbBufferEther apuntan a lo mismo*/
   
   struct ether_header *psehHeaderEther = (struct ether_header *)sbBufferEther;
   struct sockaddr_ll socket_address;
   struct sockaddr saddr;    
   int saddr_size = sizeof(saddr);

   char mv_name[3]; 
   /*Mensaje a enviar*/

   char scMsj[] = "Ya jalo tu";


   if (argc!=3)
   {
     printf ("Error en argumentos.\n\n");
     printf ("send_eth INTERFACE NOMBRE-PC-DESTINO\n");
     printf ("Ejemplo: send_eth eth0 pc1\n");
     printf ("Donde pc1 es el nombre del PC destino (pc1, pc2, pc3, etc)\n\n");
     exit (1);
   }
   /*Apartir de este este punto, argv[1] = Nombre de la interfaz y argv[2] */
   /*contiene la MAC destino, la MAC origen ya se conoce.                  */ 
   /*Abre el socket. Para que sirven los parametros empleados?*/
   if ((sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) == -1) perror("socket");
   
   /* Mediante el nombre de la interface (i.e. eth0), se obtiene su indice */
   memset (&sirDatos, 0, sizeof(struct ifreq)); 
   for (i=0; argv[1][i]; i++) sirDatos.ifr_name[i] = argv[1][i];
   if (ioctl(sockfd, SIOCGIFINDEX, &sirDatos) < 0) perror("SIOCGIFINDEX");
   iIndex = sirDatos.ifr_ifindex;
   

   /*Obtenemos el nombre de la interfaz a la cual se desea comunicar*/
   for (i=0; argv[2][i]; i++) mv_name[i] = argv[2][i];
   
   
   
   /*Ahora obtenemos la MAC de la interface por donde saldran los datos */
   memset(&sirDatos, 0, sizeof(struct ifreq));
   for (i=0; argv[1][i]; i++) sirDatos.ifr_name[i] = argv[1][i];
   if (ioctl(sockfd, SIOCGIFHWADDR, &sirDatos) < 0) perror("SIOCGIFHWADDR");
   
   /*Se imprime la MAC del host*/
   printf ("Iterface de salida: %u, con MAC: %02x:%02x:%02x:%02x:%02x:%02x\n", 
           (byte)(iIndex), 
           (byte)(sirDatos.ifr_hwaddr.sa_data[0]), (byte)(sirDatos.ifr_hwaddr.sa_data[1]), 
           (byte)(sirDatos.ifr_hwaddr.sa_data[2]), (byte)(sirDatos.ifr_hwaddr.sa_data[3]), 
           (byte)(sirDatos.ifr_hwaddr.sa_data[4]), (byte)(sirDatos.ifr_hwaddr.sa_data[5]));



           
   /*Ahora se construye la trama Ethernet empezando por su encabezado. El   */
   /*formato, para la trama IEEE 802.3 version 2, es:                       */ 
   /*6         bytes de MAC Origen                                          */
   /*6         bytes de MAC Destino                                         */
   /*2         bytes para longitud de la trama o Ether_Type                 */ 
   /*46 a 1500 bytes de payload                                             */
   /*4         bytes Frame Check Sequence                                   */
   /*Total sin contar bytes de sincronizacion, va de 64 a 1518 bytes.       */
   

   /*Llenamos con 0 el buffer de datos (payload)*/
  memset(sbBufferEther, 0, BUF_SIZ); 
  
  /*Direccion MAC Origen*/
  configurarBroadcast_Ether(pseheaderEther, sirDatos);

   /*
   psehHeaderEther->ether_shost[0] = ((uint8_t *)&sirDatos.ifr_hwaddr.sa_data)[0];
   psehHeaderEther->ether_shost[1] = ((uint8_t *)&sirDatos.ifr_hwaddr.sa_data)[1];
   psehHeaderEther->ether_shost[2] = ((uint8_t *)&sirDatos.ifr_hwaddr.sa_data)[2];
   psehHeaderEther->ether_shost[3] = ((uint8_t *)&sirDatos.ifr_hwaddr.sa_data)[3];
   psehHeaderEther->ether_shost[4] = ((uint8_t *)&sirDatos.ifr_hwaddr.sa_data)[4];
   psehHeaderEther->ether_shost[5] = ((uint8_t *)&sirDatos.ifr_hwaddr.sa_data)[5];
  */
  //  Broadcaast Factorizado
  //broadcast - asegurarnos de usar 0xFF como unsigned char
  configurarBroadcast_Ether(psehHeaderEther)

      /*
      psehHeaderEther->ether_dhost[0] = (unsigned char)0xFF;
      psehHeaderEther->ether_dhost[1] = (unsigned char)0xFF;
      psehHeaderEther->ether_dhost[2] = (unsigned char)0xFF;
      psehHeaderEther->ether_dhost[3] = (unsigned char)0xFF;
      psehHeaderEther->ether_dhost[4] = (unsigned char)0xFF;
      psehHeaderEther->ether_dhost[5] = (unsigned char)0xFF;
      */

      printf("MAC broadcast configurada: %02x:%02x:%02x:%02x:%02x:%02x\n",
             psehHeaderEther->ether_dhost[0], psehHeaderEther->ether_dhost[1],
             psehHeaderEther->ether_dhost[2], psehHeaderEther->ether_dhost[3],
             psehHeaderEther->ether_dhost[4], psehHeaderEther->ether_dhost[5]);

  //Configuración el mensaje de la trama, y del socket, a si mismo se manda un Broadcast por el Socket

   iLenHeader = sizeof(struct ether_header);


   for (i=0; ((mv_name[i])&&(i<ETHER_TYPE)); i++) sbBufferEther[iLenHeader+i] = mv_name[i];

   if (i<ETHER_TYPE)
   { /*Rellenamos con espacios en blanco*/
     while (i<ETHER_TYPE)
     {
       sbBufferEther[iLenHeader+i] = ' ';  i++;
     }
   }

   iLenHeader = iLenHeader + i;   
   /*Tipo de protocolo o la longitud del paquete*/
   psehHeaderEther->ether_type = htons(ETHER_TYPE); 
   /*Finalmente FCS*/
   for (i=0; i<4; i++) sbBufferEther[iLenHeader+i] = 0;
   iLenTotal = iLenHeader + 4; /*Longitud total*/

   /*Procedemos al envio de la trama*/
   memset(&socket_address, 0, sizeof(struct sockaddr_ll));
   socket_address.sll_family = AF_PACKET;
   socket_address.sll_protocol = htons(ETH_P_ALL);
   socket_address.sll_ifindex = iIndex;
   socket_address.sll_halen = ETH_ALEN;

   configurarBroadcast_Socket(socket_address)
   
   /*
   socket_address.sll_addr[0] = 0xFF;
   socket_address.sll_addr[1] = 0xFF;
   socket_address.sll_addr[2] = 0xFF;
   socket_address.sll_addr[3] = 0xFF;
   socket_address.sll_addr[4] = 0xFF;
   socket_address.sll_addr[5] = 0xFF;
   */

   
   printf("\nEnviando broadcast solicitando MAC de %s...\n", mv_name);
   iLen = sendto(sockfd, sbBufferEther, iLenTotal, 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll));

   if (iLen < 0) {
       perror("sendto");
       exit(1);
   }

   printf("Esperando respuesta...\n");
   memset(sbBufferEther, 0, BUF_SIZ);  


    /* ------------------------------ Esta parte se queda? ------------------------
    
    Esperamos la respuesta implementando un timeout 
    {
      int max_wait_sec = 5; 
      fd_set readfds;
      struct timeval tv;
      int rv;
      
      FD_ZERO(&readfds);
      FD_SET(sockfd, &readfds);
      tv.tv_sec = max_wait_sec;
      tv.tv_usec = 0;
      
      rv = select(sockfd + 1, &readfds, NULL, NULL, &tv);
      if (rv == -1) {
        perror("select");
        exit(1);
      } else if (rv == 0) {
        printf("Timeout esperando respuesta (%d s)\n", max_wait_sec);
        exit(1);
      } else {
        iTramaLen = recvfrom(sockfd, sbBufferEther, BUF_SIZ, 0, &saddr, (socklen_t *)(&saddr_size));
        if (iTramaLen < 0) {
          perror("recvfrom");
          exit(1);
        }
        
        //Direccion MAC destino: iSacarMAC devuelve los bytes (ether_shost) 
        char* mac_recibida = iSacarMAC(sbBufferEther);
        //Copiamos directamente los 6 bytes de la MAC recibida
        memcpy(sbMac, mac_recibida, LEN_MAC);
        printf("MAC recibida: %02x:%02x:%02x:%02x:%02x:%02x\n",
          (unsigned char)sbMac[0], (unsigned char)sbMac[1], (unsigned char)sbMac[2],
          (unsigned char)sbMac[3], (unsigned char)sbMac[4], (unsigned char)sbMac[5]);
        free(mac_recibida);
      }
    }
  */


 // Configuramos la MAC destino para el mensaje final
// Reiniciamos la trama
   configurarTrama(sbBuferr, psehHeaderEther, iLenHeader, scMsj, iIndex, iLenTotal,sbBufferEther,socket_address,sirDatos);

   // ReinciarTrama(sbBufferEther, psehHeaderEther);

   /*
     memset(sbBufferEther, 0, BUF_SIZ);
     psehHeaderEther = (struct ether_header *)sbBufferEther;

      // Configuramos la MAC origen (la de la interfaz)
      psehHeaderEther->ether_shost[0] = (byte)(sirDatos.ifr_hwaddr.sa_data[0]);
      psehHeaderEther->ether_shost[1] = (byte)(sirDatos.ifr_hwaddr.sa_data[1]);
      psehHeaderEther->ether_shost[2] = (byte)(sirDatos.ifr_hwaddr.sa_data[2]);
      psehHeaderEther->ether_shost[3] = (byte)(sirDatos.ifr_hwaddr.sa_data[3]);
      psehHeaderEther->ether_shost[4] = (byte)(sirDatos.ifr_hwaddr.sa_data[4]);
      psehHeaderEther->ether_shost[5] = (byte)(sirDatos.ifr_hwaddr.sa_data[5]);
   */

   // Configuramos la MAC destino (la que recibimos)
   //memcpy(psehHeaderEther->ether_dhost, sbMac, 6);
   /*Antes de colocar la longitud de la trama o ETHER_TYPE, colocamos*/
   /*el payload que basicamente es rellenar de letras el mensaje*/
  /*
   iLenHeader = sizeof(struct ether_header);
  
   if (strlen(scMsj) > ETHER_TYPE)
   {
     printf("El mensaje debe ser mas corto o incremente ETHER_TYPE\n");
     close(sockfd);
     exit(1);
   }
   
   for (i=0; ((scMsj[i])&&(i<ETHER_TYPE)); i++) sbBufferEther[iLenHeader+i] = scMsj[i];

   if (i<ETHER_TYPE)
   { //Rellenamos con espacios en blanco
     while (i<ETHER_TYPE)
     {
       sbBufferEther[iLenHeader+i] = ' ';  i++;
     }
   }
   iLenHeader = iLenHeader + i;   
  */
   /*
   //Tipo de protocolo o la longitud del paquete
   psehHeaderEther->ether_type = htons(ETHER_TYPE);
   //Finalmente FCS
   for (i=0; i<4; i++) sbBufferEther[iLenHeader+i] = 0;
   iLenTotal = iLenHeader + 4; //Longitud total

   //Procedemos al envio de la trama
   socket_address.sll_ifindex = iIndex;
   socket_address.sll_halen = ETH_ALEN;

   */
   // ConfigurarDestino(socket_address,sbmac)

    /*
    socket_address.sll_addr[0] = sbMac[0];
    socket_address.sll_addr[1] = sbMac[1];
    socket_address.sll_addr[2] = sbMac[2];
    socket_address.sll_addr[3] = sbMac[3];
    socket_address.sll_addr[4] = sbMac[4];
    socket_address.sll_addr[5] = sbMac[5];
    */

   // Una vez que recibe la MAC, enviamos el mensaje chisyo

   iLen = sendto(sockfd, sbBufferEther, iLenTotal, 0, (struct sockaddr *)&socket_address, sizeof(struct sockaddr_ll));
   if (iLen<0) printf("Send failed\n");


   printf ("\nContenido de la trama enviada:\n\n");
   vImprimeTrama (sbBufferEther);
   
   
   /*Cerramos*/
   close (sockfd);
   return 0;
   
 }

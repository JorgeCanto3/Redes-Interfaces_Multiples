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
  char noInterfaz[1];

  char mv_name[3];
  /*Mensaje a enviar*/

  char scMsj[100];

  if (argc != 4)
  {
    printf("Error en argumentos.\n\n");
    printf("send_eth INTERFACE NOMBRE-PC-DESTINO\n");
    printf("Ejemplo: send_eth eth0 pc1 #\n");
    printf("Donde pc1 es el nombre del PC destino (pc1, pc2, pc3, etc)  y # es numero de tu interfaz \n\n");
    exit(1);
  }

  /*Apartir de este este punto, argv[1] = Nombre de la interfaz y argv[2] */
  /*contiene la MAC destino, la MAC origen ya se conoce.                  */
  /*Abre el socket. Para que sirven los parametros empleados?*/
  if ((sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) == -1)
    perror("socket");

  /* Mediante el nombre de la interface (i.e. eth0), se obtiene su indice */
  memset(&sirDatos, 0, sizeof(struct ifreq));
  for (i = 0; argv[1][i]; i++)
    sirDatos.ifr_name[i] = argv[1][i];
  if (ioctl(sockfd, SIOCGIFINDEX, &sirDatos) < 0)
    perror("SIOCGIFINDEX");
  iIndex = sirDatos.ifr_ifindex;

  /*Obtenemos el nombre de la interfaz a la cual se desea comunicar*/
  for (i = 0; argv[2][i]; i++)
    mv_name[i] = argv[2][i];

  noInterfaz[0] = argv[3][0];

  /*Ahora obtenemos la MAC de la interface por donde saldran los datos */
  memset(&sirDatos, 0, sizeof(struct ifreq));
  for (i = 0; argv[1][i]; i++)
    sirDatos.ifr_name[i] = argv[1][i];
  if (ioctl(sockfd, SIOCGIFHWADDR, &sirDatos) < 0)
    perror("SIOCGIFHWADDR");

  /*Se imprime la MAC del host*/
  printf("Dirreción de la Interfaz de Entrada: %02x:%02x:%02x:%02x:%02x:%02x\n",
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

  //reinciarTrama(sbBufferEther);
  memset(sbBufferEther, 0, BUF_SIZ);
  psehHeaderEther = (struct ether_header *)sbBufferEther;



/////// Broadcast para solicitar MAC de pcX ///////

  memset(scMsj, 0, sizeof(scMsj));
\
  if (strcmp(mv_name, "pc4") == 0 || strcmp(mv_name, "pc5") == 0)
  {
    
    strcpy(scMsj, "pc3");
    configurarTrama_Broadcast(sbBufferEther, psehHeaderEther, &sirDatos,  scMsj,&iLenHeader, iIndex, &iLenTotal, sockfd, &socket_address);
    printf("\nBuscando a pc3! :\n\n");
    
  }else{
    printf("okzion 2");
    strcpy(scMsj, mv_name);
    configurarTrama_Broadcast(sbBufferEther, psehHeaderEther, &sirDatos,  scMsj,&iLenHeader, iIndex, &iLenTotal, sockfd, &socket_address);
    printf("\nBuscando a %c%c%c! :\n\n", mv_name[0], mv_name[1],mv_name[2]);
  }
  
  
  printf("MAC broadcast configurada: %02x:%02x:%02x:%02x:%02x:%02x\n\n",
         psehHeaderEther->ether_dhost[0], psehHeaderEther->ether_dhost[1],
         psehHeaderEther->ether_dhost[2], psehHeaderEther->ether_dhost[3],
         psehHeaderEther->ether_dhost[4], psehHeaderEther->ether_dhost[5]);

// Configuración el mensaje de la trama, y del socket, a si mismo se manda un Broadcast por el Socket


  /*Procedemos al envio de la trama*/
  memset(&socket_address, 0, sizeof(struct sockaddr_ll));
  socket_address.sll_family = AF_PACKET;
  socket_address.sll_protocol = htons(ETH_P_ALL);
  socket_address.sll_ifindex = iIndex;
  socket_address.sll_halen = ETH_ALEN;

  configurarBroadcast_Socket(&socket_address);  

      printf("\nEnviando broadcast solicitando MAC de %s...\n", scMsj);
  iLen = sendto(sockfd, sbBufferEther, iLenTotal, 0, (struct sockaddr *)&socket_address, sizeof(struct sockaddr_ll));

  if (iLen < 0)
  {
    perror("sendto");
    exit(1);
  }

  printf("Esperando respuesta...\n");
  memset(sbBufferEther, 0, BUF_SIZ);

  int max_wait_sec = 5;
  fd_set readfds;
  struct timeval tv;
  int rv;

  FD_ZERO(&readfds);
  FD_SET(sockfd, &readfds);
  tv.tv_sec = max_wait_sec;
  tv.tv_usec = 0;

  rv = select(sockfd + 1, &readfds, NULL, NULL, &tv);
  if (rv == -1)
  {
    perror("select");
    exit(1);
  }
  else if (rv == 0)
  {
    printf("Timeout esperando respuesta (%d s)\n", max_wait_sec);
    exit(1);
  } /////////////////// fin broadcast ///////////////////////
  else
  {
    iTramaLen = recvfrom(sockfd, sbBufferEther, BUF_SIZ, 0, &saddr, (socklen_t *)(&saddr_size));
    if (iTramaLen < 0)
    {
      perror("recvfrom");
      exit(1);
    }


    // Direccion MAC destino: iSacarMAC devuelve los bytes (ether_shost)
    char *mac_recibida = iSacarMAC(sbBufferEther);
    // Copiamos directamente los 6 bytes de la MAC recibida
    for (int i = 0; i < LEN_MAC; i++)
    {
      sbMac[i] = mac_recibida[i];
    }
    
    //memcpy(sbMac, mac_recibida, LEN_MAC);
    printf("MAC recibida: %02x:%02x:%02x:%02x:%02x:%02x\n",
           sbMac[0],sbMac[1], sbMac[2],
           sbMac[3],sbMac[4], sbMac[5]);
    free(mac_recibida);
  }
  
  memset(sbBufferEther, 0, BUF_SIZ);
  //reinciarTrama(sbBufferEther);
  psehHeaderEther = (struct ether_header *)sbBufferEther;
  
  memset(scMsj, 0, sizeof(scMsj));  
  char mensaje[] = "XXX X te busco ";
  mensaje[0] = mv_name[0];
  mensaje[1] = mv_name[1];
  mensaje[2] = mv_name[2];
  mensaje[4] = noInterfaz[0];
  
  strcpy(scMsj, mensaje);

  configurarTrama(sbBufferEther, psehHeaderEther, &sirDatos, sbMac, scMsj, &iLenHeader, iIndex, &iLenTotal, sockfd, &socket_address);


  iLen = sendto(sockfd, sbBufferEther, iLenTotal, 0, (struct sockaddr *)&socket_address, sizeof(struct sockaddr_ll));
  if (iLen < 0)
    printf("Send failed\n");

  printf("\nContenido de la trama enviada:\n\n");

  /*Cerramos*/
  close(sockfd);
  return 0;
}

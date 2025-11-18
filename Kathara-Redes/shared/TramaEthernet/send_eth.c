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

  if (argc != 3)
  {
    printf("Error en argumentos.\n\n");
    printf("send_eth INTERFACE NOMBRE-PC-DESTINO\n");
    printf("Ejemplo: send_eth eth0 pc1\n");
    printf("Donde pc1 es el nombre del PC destino (pc1, pc2, pc3, etc)\n\n");
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

  /*Ahora obtenemos la MAC de la interface por donde saldran los datos */
  memset(&sirDatos, 0, sizeof(struct ifreq));
  for (i = 0; argv[1][i]; i++)
    sirDatos.ifr_name[i] = argv[1][i];
  if (ioctl(sockfd, SIOCGIFHWADDR, &sirDatos) < 0)
    perror("SIOCGIFHWADDR");

  /*Se imprime la MAC del host*/
  printf("Iterface de salida: %u, con MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
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



/////// Broadcast para solicitar MAC de pcX ///////


  /*Direccion MAC Origen*/
  configurarOrigen_Ether(psehHeaderEther, &sirDatos);

  // broadcast - asegurarnos de usar 0xFF como unsigned char
  configurarBroadcast_Ether(psehHeaderEther);

  printf("MAC broadcast configurada: %02x:%02x:%02x:%02x:%02x:%02x\n",
         psehHeaderEther->ether_dhost[0], psehHeaderEther->ether_dhost[1],
         psehHeaderEther->ether_dhost[2], psehHeaderEther->ether_dhost[3],
         psehHeaderEther->ether_dhost[4], psehHeaderEther->ether_dhost[5]);

// Configuración el mensaje de la trama, y del socket, a si mismo se manda un Broadcast por el Socket


  scMsj = "pc3";

  configurarTrama(sbBufferEther, psehHeaderEther, &sirDatos, sbMac,  scMsj,&iLenHeader, iIndex, &iLenTotal, sockfd, socket_address);
  /*Procedemos al envio de la trama*/
  memset(&socket_address, 0, sizeof(struct sockaddr_ll));
  socket_address.sll_family = AF_PACKET;
  socket_address.sll_protocol = htons(ETH_P_ALL);
  socket_address.sll_ifindex = iIndex;
  socket_address.sll_halen = ETH_ALEN;

  configurarBroadcast_Socket(&socket_address);

      printf("\nEnviando broadcast solicitando MAC de %s...\n", mv_name);
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
    char *mac_recibida = iSacarMAC_Trama(sbBufferEther);
    // Copiamos directamente los 6 bytes de la MAC recibida
    memcpy(sbMac, mac_recibida, LEN_MAC);
    printf("MAC recibida: %02x:%02x:%02x:%02x:%02x:%02x\n",
           (unsigned char)sbMac[0], (unsigned char)sbMac[1], (unsigned char)sbMac[2],
           (unsigned char)sbMac[3], (unsigned char)sbMac[4], (unsigned char)sbMac[5]);
    free(mac_recibida);
  }

  scMsj = "pc5";

  reinciarTrama(sbBufferEther);                                                             
  psehHeaderEther = (struct ether_header *)sbBufferEther;


  configurarTrama(sbBufferEther, psehHeaderEther, &sirDatos, sbMac, scMsj, &iLenHeader, iIndex, &iLenTotal, sockfd, socket_address);


  iLen = sendto(sockfd, sbBufferEther, iLenTotal, 0, (struct sockaddr *)&socket_address, sizeof(struct sockaddr_ll));
  if (iLen < 0)
    printf("Send failed\n");

  printf("\nContenido de la trama enviada:\n\n");
  vImprimeTrama(sbBufferEther);

  /*Cerramos*/
  close(sockfd);
  return 0;
}

#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/ether.h>
#include <unistd.h>

 
#define BUF_SIZ              2000  /*Con 2000 bytes son suficientes para la trama, ya que va de 64 a 1518*/
#define TRAMA_DESTINATION    0
#define TRAMA_SOURCE         6   
#define TRAMA_ETHER_TYPE     (6+6)
#define TRAMA_PAYLOAD        (6+6+2)  
#define LEN_MAC              6
#define LEN_NAME             3
#define LEN_ARP             3



/*En realidad podriamos enviar cualquier trama con payload entre 43 y 1500, */
/*sin embargo, habria que calcular el FCS y por el momento no es importante.*/
/*Entonces en lugar de "escuchar" cualquier trama, escucharemos todas las   */
/*tramas sin que estas sean filtradas por la CAR y nuestros programas haran */
/*el filtrado. Ver codigo en recv_eth.c                                     */
#define ETHER_TYPE           0x0100 /*Nuestro "protocolo"*/

 /*Tipo de dato sin signo*/
typedef unsigned char byte;


 void vConvierteMAC (char *psMac, char *psOrg)
 { /*En lugar de caracteres, se requiere el numero*/
   int i, j, iAux, iAcu;
   for (i=0, j=0, iAcu=0; i<12; i++)
   {
     if ((psOrg[i]>47)&&(psOrg[i]<58))  iAux = psOrg[i] - 48;  /*0:9*/
     if ((psOrg[i]>64)&&(psOrg[i]<71))  iAux = psOrg[i] - 55;  /*A:F*/
     if ((psOrg[i]>96)&&(psOrg[i]<103)) iAux = psOrg[i] - 87;  /*a:f*/ 
     if ((i%2)==0) iAcu = iAux * 16;
     else 
     { /*Obtiene el byte*/
       psMac[j] = iAcu + iAux;  j++;
     }
   }
 }
 
 void vImprimeTrama (char *psTrama)
 { /*Imprime el contenido de la trama, supone bien construida*/
   short int *piEtherType;
   int i, iLen;

   piEtherType = (short int *)(psTrama + TRAMA_ETHER_TYPE);
   iLen = (int)(htons(*piEtherType));
   
   printf ("Destino:  ");
   for (i=0; i<LEN_MAC; i++) 
     printf ("%02x ", (unsigned char)psTrama[TRAMA_DESTINATION+i]);
   
     printf ("\nFuente:   ");
   for (i=0; i<LEN_MAC; i++) 
     printf ("%02x ", (unsigned char)psTrama[TRAMA_SOURCE+i]);  
   
     printf ("\nLongitud: %d\n", iLen);
   
     printf ("Payload:\n");
   
     for (i=0; i<iLen; i++)
       printf ("%c", (char)psTrama[i+TRAMA_PAYLOAD]);
   
       piEtherType = (short int *)(psTrama+i+TRAMA_PAYLOAD);
   
       printf ("\nFCS:      %d\n\n", *piEtherType); 
 }
 
 int iLaTramaEsParaMi (char *psTrama, struct ifreq *psirDatos)
 {
   int i, iFlag;
   for (i=0, iFlag=1; i<LEN_MAC; i++)
     if (psirDatos->ifr_hwaddr.sa_data[i]!=psTrama[TRAMA_DESTINATION+i]) iFlag = 0;
   return (iFlag);
 }

 char *iSacarMAC(char *psTrama)
 {
    struct ether_header *eh = (struct ether_header *)psTrama;
    char* mac = malloc(LEN_MAC * sizeof(char));
    printf("Extrayendo MAC origen: %02x:%02x:%02x:%02x:%02x:%02x\n",
         (unsigned char)eh->ether_shost[0],
         (unsigned char)eh->ether_shost[1],
         (unsigned char)eh->ether_shost[2],
         (unsigned char)eh->ether_shost[3],
         (unsigned char)eh->ether_shost[4],
         (unsigned char)eh->ether_shost[5]);
   
    for(int i = 0; i < LEN_MAC; i++) {
       mac[i] = eh->ether_shost[i];
    }
    return mac;
 }

 void CrearBroadcastEther(struct ether_header *psehHeaderEther)
 {

   for (int i = 0; i < LEN_MAC; i++)
   {
     psehHeaderEther->ether_dhost[i] = (unsigned char)0xFF;
   }

   printf("Broadcast Asignado a la MAC de destino!");


  }


  void ConfigurarOrigenEther(struct ether_header *psehHeaderEther, struct ifreq *sirDatos)
  {
   for (int i = 0; i < LEN_MAC; i++)
   {
     psehHeaderEther->ether_shost[i] = ((uint8_t *)&sirDatos.ifr_hwaddr.sa_data)[i];
   }
 
 }

 void ConfigurarBroadcastD_Sock(struct sockaddr_ll *socket_address)
 {

   for (int i = 0; i < LEN_MAC; i++)
   {

     socket_address.sll_addr[i] = (unsigned char)0xFF;
   }
 }

 void ConfigurarDestinoSock(struct sockaddr_ll *socket_address, byte sbmac)
 {

   for (int i = 0; i < LEN_MAC; i++){

     socket_address.sll_addr[i] = sbMac[i];
   }

 }


void ReinciarTrama(byte * sbBufferEther[BUF_SIZ], struct ether_header * psehHeaderEther){
 
  {

    memset(sbBufferEther, 0, BUF_SIZ);
    psehHeaderEther =  (struct ether_header *)sbBufferEther
  }
 }



 int isParaMi(char *psTrama, char *pcName){
  int i, iFlag=1;
  printf("Verificando mensaje para %s. Contenido del payload: ", pcName);
  for(i=0; i<3; i++) {
    printf("%c", psTrama[TRAMA_PAYLOAD+i]);
  }
  printf("\n");
  
  for (i=0; i<3; i++) {
    if(psTrama[TRAMA_PAYLOAD+i] != pcName[i]) {
      iFlag = 0;
      printf("No coincide en posición %d: recibido '%c' esperado '%c'\n", 
             i, psTrama[TRAMA_PAYLOAD+i], pcName[i]);
    }
  }
  return iFlag;
}

void ArmarTrama(){
  
}

int isBroadcast(char *psTrama){
  int i, iFlag=1;
  printf("Verificando si es broadcast. MAC destino: ");
  for (i=0; i<LEN_MAC; i++){
    unsigned char byte = (unsigned char)psTrama[TRAMA_DESTINATION+i];
    printf("%02x ", byte);
    if (byte != 0xFF) {
      iFlag = 0;
    }
  }
  printf(" -> %s\n", iFlag ? "SI ES BROADCAST" : "NO ES BROADCAST");
  return iFlag;
}
cd
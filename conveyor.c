#INCLUDE <16f887.h>
#device ADC=8  
#fuses /*XT,*/NOWDT,NOPROTECT,NOLVP,PUT,BROWNOUT  
#use delay(clock=/*1843200 2000000*/4000000)  
#INCLUDE <stdlib.h>  
#INCLUDE <math.h>
#include <string.h>
#use rs232(baud=9600, xmit=pin_c6, rcv=pin_c7, bits=8, parity=N)



#define sensor1 PIN_A0 
#define sensor2 PIN_A1
#define sensor3 PIN_A2
#define panic PIN_A3
#define motor PIN_B5// el valor que debes poner es el tiempo que tu deseas entre .0655
#define constante_de_tiempo 25 // este es el tiempo determina si el buffer esta lleno al activarse el s1 y s2
#define tiempo_de_inicio 150  // tiempo que dura el motor encendido despues de encender el conveyor
#define tiempo_para_llegar_al_final 95 // tiempo que el motor se mantiene encendido para que una pieze llegue al final despues de haber sido sensada por el s2
#define timeout_del_motor2 40 // tiempo que espera el conveyor para sensar la pieza al final, antes de que se apague  
#define timeout_motor1 120 // tiempo que espera el conveyor para sensar la pieza con s2, antes de que se apague 


int s1=0;
int s2=0;
int pausa=0;
int tiempo_pausa;
int s3=0;
int s1_ant=0;
int s2_ant=0;
int s3_ant=0;
int voltaje_presente=0;
int buffer_sw1=0;
int EN_timersw1=0;
int timer_sw1=0;
char estado_sw1[20]="inactivo";
char estado_sw2[20]="inactivo";
int buffer_sw2=0;
int EN_timersw2=0;
int timer_sw2=0;
int buffer_total=0;
int motor_uno=0;
int motor_dos=0;
int timer_init=0;
int iniciar=1;
int EN_timeout2=0;
int timeout2=0;
int EN_descarga=0;
int descarga=0;

#INT_TIMER0    

void TIMR0_ISR(){
if(EN_timersw1==1){ //timer para el timeout
timer_sw1++;
}
if(EN_timersw2==1){
timer_sw2++;
}
if(pausa==1){
tiempo_pausa++;
}
if(iniciar==1 && input(panic)==1){
timer_init++;
}
if(EN_timeout2==1 && buffer_sw2==0){
timeout2++;
}
else{
timeout2=0;
}
if(EN_descarga==1){
descarga++;
}
else{
descarga=0;
}
}




void motor_off(){
motor_uno=0;
}


void motor_on(){
motor_uno=1;
}


void motor_off2(){
motor_dos=0;
EN_timeout2=0;
}


void motor_on2(){
motor_dos=1;
EN_timeout2=1;
}



void checar_s1(){
 if(s1==1){
 buffer_sw1++;
 EN_timersw1=1;
 timer_sw1=0;
 printf("checar s1\n\r");
 }
}



void checar_s2(){
 if(s2==1 && buffer_sw1>0){
 
 buffer_sw1--;
 timer_sw1=0;
 estado_sw2="entro_pieza";
 buffer_sw2++;
 buffer_total++;
 EN_timersw2=1;
 }
 printf("checar s2\n\r");
}



/*void checar_s2_sw2(){
 if(s2==1){
 buffer_sw2++;
 buffer_total++;
 EN_timersw2=1;
 }
}*/




void checar_s3(){
if(s3==1 && buffer_total>0){
buffer_total--;
}
if(s3==0){
EN_descarga=1;
}
}



void checar_timersw2(){
if (timer_sw2 > tiempo_para_llegar_al_final * buffer_sw2){
buffer_sw2=0;
EN_timersw2=0;
timer_sw2=0;
}
}


void checar_buffers(){
if(timeout2>timeout_del_motor2){
buffer_total=0;}

if(buffer_sw2>0){
motor_on2();
}
else{ 
  if((buffer_total>0 && s3==0) || (descarga<16 && EN_descarga==1)){
  motor_on2();
  }
  else{
  motor_off2();
  if(descarga>16){ 
  EN_descarga=0;
  descarga=0;
  }
  }
}
}




void checar_buffer(){
if(buffer_sw1>0){
motor_on();
}
else{
motor_off();
}
}




void checar_timer(){
 if(timer_sw1>timeout_motor1){
 buffer_sw1=0;
 EN_timersw1=0;
 }
 checar_buffer();
}



void seq_inicial(){
while (timer_init< tiempo_de_inicio && input(panic)==1){
output_high(motor);
printf("%i\n\r",timer_init);
}
output_Low(motor);
iniciar=0;
timer_init=0;
}



void main (){
setup_timer_0(RTCC_INTERNAL | RTCC_DIV_256);
set_timer0(0);
enable_interrupts(INT_RTCC);
enable_interrupts(GLOBAL);

while (true){

s1=input(sensor1);
s2=input(sensor2);
s3=input(sensor3);
voltaje_presente=input(panic);


if(iniciar==1&&voltaje_presente==1){seq_inicial();}

if(s2==1&&s3==1){
 if(tiempo_pausa<constante_de_tiempo){pausa=1;}
}
else {
tiempo_pausa=0;
pausa=0;
}


printf("%i\n\r",descarga);

if(tiempo_pausa<constante_de_tiempo){
// este switch case controla los procesos que corresponden a la primera parte del conveyor
switch (estado_sw1){
     case "inactivo":
     
       if(voltaje_presente==1){
        estado_sw1="lectura";
       }
       else{
       motor_off();
       buffer_sw1=0;
       iniciar=1;
       //s1=0;
       }
     break;
     
     case "lectura":
     
       if(voltaje_presente==0){
        estado_sw1="inactivo";
        iniciar=1;
       }
       if(s1!=s1_ant){
       checar_s1();
       }
       if(s2!=s2_ant){
       checar_s2();
       }
       checar_timer();
     break;  

}

// switch para la segunda parte del programa
switch (estado_sw2){
     case "inactivo":
       motor_off2();
       buffer_sw2=0;
       buffer_total=0;
       
     break;
     
     case "entro_pieza":
     
       if(voltaje_presente==0){
        estado_sw2="inactivo";
        iniciar=1;
       }
       
       if(s3!=s3_ant){
       checar_s3();
       }
       checar_timersw2();
       checar_buffers();
       
     break;  

}
}
else{
s1=0;
pausa=0;
motor_off2();
motor_off();
}


s1_ant=s1;
s2_ant=s2;
s3_ant=s3;
if(motor_uno==1 ||motor_dos==1){
output_high(motor);
}
else{
output_low(motor);
}
}
}

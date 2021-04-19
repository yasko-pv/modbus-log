#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <modbus.h>
#include <fcntl.h>
#include <linux/serial.h>
#include <sys/ioctl.h>
#include <curl/curl.h>

#define NREG			125
#define START_REG		35100
#define METER_PWR		36008
#define PV_PWR			35105
#define BATT_PWR		35182
#define TOTAL_INV_POWER	35138
#define TOTAL_LOAD_POWER	35172

struct pwr_inverter{
        uint32_t pv;
        int32_t batt;
        int32_t meter;
	int32_t out;
	int32_t load;
};

struct pwr_inverter pwr1;

modbus_t *ctx;
uint16_t reg[NREG];

void print_data();
void print_pwr();
void read_pwr();
int16_t read_wreg(uint16_t addr);
int32_t read_dwreg(uint16_t addr);
void print_batt_data();
int post_data(CURL *curl, struct pwr_inverter *pw);


int main()
{

	CURL *curl;
  	CURLcode res;

	curl_global_init(CURL_GLOBAL_ALL);
	  /* get a curl handle */ 
  	curl = curl_easy_init();
	if (curl == 0){
		exit(3);
	}

	ctx = modbus_new_rtu("/dev/ttyUSB0", 9600, 'N', 8, 1);
	if (!ctx) {
    		fprintf(stderr, "Failed to create the context: %s\n", modbus_strerror(errno));
    		exit(1);
	}

	//modbus_rtu_set_serial_mode(ctx, MODBUS_RTU_RS485);
	//modbus_rtu_set_rts(ctx, MODBUS_RTU_RTS_UP);
	//modbus_rtu_set_rts_delay(ctx,0);
	modbus_set_slave(ctx, 247);


	if (modbus_connect(ctx) == -1) {
    		fprintf(stderr, "Unable to connect: %s\n", modbus_strerror(errno));
    		modbus_free(ctx);
    		exit(1);
	}


	//Set the Modbus address of the remote slave (to 3)
	modbus_set_response_timeout(ctx, 0, 50000);




	for(;;){
		//print_batt_data();
		//print_data();
		read_pwr();
		print_pwr();

		
		 if (curl){
                        post_data(curl, &pwr1);
                }
		usleep(1000000);
	}

	modbus_close(ctx);
	modbus_free(ctx);
}

void print_batt_data()
{
	int num;
	int32_t pw;

	num = modbus_read_registers(ctx, 35180, 4, (uint16_t *)reg);
	if (num != 4){
		fprintf(stderr, "Failed to read: %s\n", modbus_strerror(errno));
		return;
	}
	fprintf(stdout,"%.1f,%.1f,",(float)reg[0]/10.0, ((int16_t)reg[1])/10.0);
	pw = MODBUS_GET_INT32_FROM_INT16(reg,2);
	//pw = (int32_t)reg[2]<<16 | (int32_t)reg[3];
	fprintf(stdout,"%d\r\n",pw);

}


void print_pwr()
{
	fprintf(stdout,"%d,%d,%d,%d,%d\r\n",pwr1.pv, pwr1.batt, pwr1.meter, pwr1.out, pwr1.load);
}

void read_pwr()
{
	pwr1.meter = read_wreg(METER_PWR);
	pwr1.pv = read_dwreg(PV_PWR);
	pwr1.batt = read_dwreg(BATT_PWR);
	pwr1.out = read_wreg(TOTAL_INV_POWER);
	pwr1.load = read_wreg(TOTAL_LOAD_POWER);
}


void print_data()
{
	int num, i;
	uint16_t start_reg = START_REG;


	//Read 5 holding registers starting from address 35180
	num = modbus_read_registers(ctx, start_reg, NREG, (uint16_t *)reg);

		if (num != NREG) {// number of read registers is not the one expected
    			fprintf(stderr, "Failed to read: %s\n", modbus_strerror(errno));
		}

		else {
			for (i = 0; i < NREG; i++){
				fprintf(stdout,"%d,",(int16_t)reg[i]);
			}
			fprintf(stdout,"\r\n");

		}

}


int16_t read_wreg(uint16_t addr)
{
	int num;
	uint16_t r;

	num = modbus_read_registers(ctx, addr, 1, &r);
	if (num != 1) {// number of read registers is not the one expected
        	fprintf(stderr, "Failed to read: %s\n", modbus_strerror(errno));
		return 0;

        }
	return r;
}


int32_t read_dwreg(uint16_t addr)
{
        int num;
        uint16_t r[2];

        num = modbus_read_registers(ctx, addr, 2, r);
        if (num != 2) {// number of read registers is not the one expected
                fprintf(stderr, "Failed to read: %s\n", modbus_strerror(errno));
                return 0;

        }
        return MODBUS_GET_INT32_FROM_INT16(r,0);
}


int post_data(CURL *curl, struct pwr_inverter *pw)
{
	 CURLcode res;

	char post_data[128];
	char sval[64];

	strcpy(post_data, "gw6000eh,param=power "); 
	sprintf(sval,"pv=%d,batt=%d,meter=%d,out=%d,load=%d", pw->pv,pw->batt,pw->meter,pw->out,pw->load);
	strcat(post_data,sval);

	curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:8086/write?db=pv1");	
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);

	res = curl_easy_perform(curl);	

	if(res != CURLE_OK)
      		fprintf(stderr, "curl_easy_perform() failed: %s\n",
              		curl_easy_strerror(res));

	return 0;
}

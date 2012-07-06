/***************************************************************
*
* observe.c
* 
* SMAsh command for tracking an astronomical source
* NAP 
* 1 March 2000
*
* 5 June 2001: added inputs for source specs and offsets
* This version is for the PowerPC
* Command line arguments are now parsed using the smapopt library.
* Revised on 26 Jun 2001 to command only those antennas which
*  are specified by the project command- if no antenna switch is given.
* 22 oct 2003: added additional command line arguments for input
* of source name and coordinates- allowing non-catalog sources.
****************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <rpc/rpc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <smapopt.h>
#include <ctype.h>
#include <time.h>
#include "dsm.h"
#include "commonLib.h"

#define OK     0
#define ERROR -1
#define RUNNING 1

#define DSM_HOST "gltacc"

void usage(int exitcode, char *error, char *addl) {
 
        if (error) fprintf(stderr, "\n%s: %s\n\n", error, addl);
        fprintf(stderr, "Usage: observe [options] --source <sourcename>\n"
                "[options] include:\n"
		"[--ra or -r <hh:mm:ss.sss> --dec or -d <+/-dd:mm:dd.ddd>\n"
		"--epoch -e <1950./2000.>\n"
		"--velocity or -v <velocity km/s (only vlsr for now)>]\n"
                "  -h or --help    this help\n");
        exit(exitcode);
}

int main(int argc, char *argv[])  {

	char c,*source, Source[34], command_n[30], *nameIt, fileSource[34], oldSource[34];
	short i, pmac_command_flag=0; 
	short  sourcelength;
	int gotsource=0;
	int gotpmra=0,gotpmdec=0;
	int gotra=0,gotdec=0,gotepoch=0,gotvel=0;
	int newSourceFlag=0;
	int dsm_status;
	int sourceType=0;
	smapoptContext optCon;	
	int tracktimestamp,timestamp;
	int trackStatus=0;
	double ra=0.0,dec=0.0,velocity=0.0,epoch=2000.;
	double pmra=0.0,pmdec=0.0;
	time_t timeStamp;

	 struct  smapoptOption optionsTable[] = {
                {"source",'s',SMAPOPT_ARG_STRING,&source,'s',
		"source name."},
                {"name_it",'n',SMAPOPT_ARG_STRING,&nameIt,'n',
		"use this name in data file."},
                {"ra",'r',SMAPOPT_ARG_TIME,&ra,'r',
		"RA in hours  (decimal or HH:MM:SS.SSS format"},
                {"dec",'d',SMAPOPT_ARG_DEC,&dec,'d',
		"DEC in degrees (decimal or +-DD:MM:SS.SSS"},
                {"velocity",'v',SMAPOPT_ARG_DOUBLE,&velocity,'v',
		"source velocity in km/s, VLSR"},
                {"epoch",'e',SMAPOPT_ARG_DOUBLE,&epoch,'e',
		"Epoch of input coordinates 1950 or 2000."},
                {"pmra",'p',SMAPOPT_ARG_DOUBLE,&pmra,'p',
		"Proper motion along RA in mas/yr."},
                {"pmdec",'q',SMAPOPT_ARG_DOUBLE,&pmdec,'q',
		"Proper motion along DEC in mas/yr."},
		SMAPOPT_AUTOHELP	
                {NULL,0,0,NULL,0}
        };

 if(argc<2) usage(-1,"Insufficient number of arguments","At least source- name required.");
 
        optCon = smapoptGetContext("observe", argc, argv, optionsTable,0);
 
        while ((c = smapoptGetNextOpt(optCon)) >= 0) {
        switch(c) {
                case 'h':
                usage(0,NULL,NULL);
                break;
 
                case 's':
                gotsource=1;
                break;

                case 'r':
                gotra=1;
                break;

                case 'd':
                gotdec=1;
                break;

                case 'p':
                gotpmra=1;
                break;

                case 'q':
                gotpmdec=1;
                break;

                case 'e':
                gotepoch=1;
                break;

                case 'v':
                gotvel=1;
                break;
 
                }
 
        }
 
        if(gotsource!=1) usage(-2,"No source specified","Source name is required .\n");

	if((gotpmra==1)||(gotpmdec==1)) {
		if(!((gotpmra==1)&&(gotpmdec==1))) {
		usage(-5,"Insufficient arguments."," when giving proper motion values, both ra and dec pm are required.\n");
		}
	}

	if((gotra==1)||(gotdec==1)) {
		if(!((gotra==1)&&(gotdec==1)&&(gotsource==1))) {
		usage(-4,"Insufficient arguments.",
	"if coordinates are specified, source-name, ra, dec, epoch and propermotions  are all required; velocity is optional.\n");
		}
		else  {
		newSourceFlag=1;
		if(gotepoch==0) epoch=2000.;
		}
	}

 
        if(c<-1) {
        fprintf(stderr, "%s: %s\n",
                smapoptBadOption(optCon, SMAPOPT_BADOPTION_NOALIAS),
                smapoptStrerror(c));
        }
	/* initialize dsm */
	  dsm_status = dsm_open();
	  if(dsm_status!= DSM_SUCCESS) {
	    dsm_error_message(dsm_status, "dsm_open");
	    exit(-1);
	  }

	smapoptFreeContext(optCon);


	sourcelength=strlen(source);
	strcpy(Source,source);
	for(i=0;i<30;i++) {
	command_n[i]=0x0;
	}
	command_n[0]='n';/*0x6e;*/ /* send 'n' command */
	command_n[1]=0x0;
	pmac_command_flag=0;


#if 0

	  /* check if track is running on this antenna */
        
        rm_status=rm_read(antenna,"DSM_UNIX_TIME_L",&timestamp);
	rm_status=rm_read(antenna,"DSM_TRACK_TIMESTAMP_L",&tracktimestamp);
        if((abs(tracktimestamp-timestamp)>3L) && (antenna < 9)) {
	trackStatus=0;
	printf("Track is not running on antenna %d.\n",antenna);
	}
        if((abs(tracktimestamp-timestamp)<=3L) || (antenna > 8)) trackStatus=1;

	if(trackStatus==1) {
#endif

	
	dsm_status=dsm_write(DSM_HOST,"DSM_CMD_SOURCE_FLAG_L",
				&newSourceFlag);
        if(dsm_status != DSM_SUCCESS) {
                dsm_error_message(dsm_status,"dsm_write()");
                exit(1);
        }
	
	if(newSourceFlag==1) {

	dsm_status=dsm_write(DSM_HOST,"DSM_CMD_SOURCE_C34",
				&Source);
        if(dsm_status != DSM_SUCCESS) {
                dsm_error_message(dsm_status,"dsm_write()");
                exit(1);
        }

	dsm_status=dsm_write(DSM_HOST,"DSM_CMD_RA_HOURS_D", &ra);
	dsm_status=dsm_write(DSM_HOST,"DSM_CMD_DEC_DEG_D", &dec);
	dsm_status=dsm_write(DSM_HOST,"DSM_CMD_EPOCH_YEAR_D", &epoch);
	dsm_status=dsm_write(DSM_HOST,"DSM_CMD_PMRA_MASPYEAR_D", &pmra);
	dsm_status=dsm_write(DSM_HOST,"DSM_CMD_PMDEC_MASPYEAR_D", &pmdec);
	if(gotvel==0) velocity=0.0;
	dsm_status=dsm_write(DSM_HOST,"DSM_CMD_SVEL_KMPS_D", &velocity);
	} /* if new source flag =1 */

	dsm_status=dsm_write(DSM_HOST,"DSM_SOURCE_LENGTH_S",
				&sourcelength);
        if(dsm_status != DSM_SUCCESS) {
                dsm_error_message(dsm_status,"dsm_write()");
                exit(1);
        }

	dsm_status=dsm_write(DSM_HOST,"DSM_SOURCE_C34",Source);
        if(dsm_status != DSM_SUCCESS) {
                dsm_error_message(dsm_status,"dsm_write()");
                exit(1);
        }


	dsm_status=dsm_write(DSM_HOST,"DSM_COMMANDED_TRACK_COMMAND_C30",
					command_n);
        if(dsm_status != DSM_SUCCESS) {
                dsm_error_message(dsm_status,"dsm_write()");
                exit(1);
        }
	
	dsm_status=dsm_write(DSM_HOST,"DSM_COMMAND_FLAG_S",
					&pmac_command_flag);
        if(dsm_status != DSM_SUCCESS) {
                dsm_error_message(dsm_status,"dsm_write(DSM_COMMAND_FLAG)");
                exit(1);
        }


#if 0
	} /* if track is running */
#endif
	
return(0);
}

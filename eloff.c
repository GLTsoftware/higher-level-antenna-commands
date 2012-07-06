/***************************************************************
*
* eloff.c
*
*
* for GLT, DSM version. 21 June 2012. NAP
*
*
* Original notes:
* 
* NAP 18 FEB 99.
*
* Revised on 2 March 2000 for PowerPC and smapopt.
* Revised on 27 Feb 2001 to command all antennas by default .
* Revised on 26 Jun 2001 to command only those antennas which
*  are specified by the project command- if no antenna switch is given.
****************************************************************/

#include <stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include <smapopt.h>
#include "dsm.h"
#include "commonLib.h"

#define DSM_HOST "gltacc"

#define OK     0
#define ERROR -1
#define RUNNING 1


void usage(int exitcode, char *error, char *addl) {

        if (error) fprintf(stderr, "\n%s: %s\n\n", error, addl);
        fprintf(stderr, "Usage: eloff [options] --arcseconds (or -s) <arcseconds>\n"
                "[options] include:\n"
                "  -h or --help    this help\n");
        exit(exitcode);
}

int main(int argc, char *argv[])  
{

	char c,*arcseconds,command_n[30];
	short pmac_command_flag=0;
	int goteloff=0,dsm_status;
	double eloff;
	int i ;
	smapoptContext optCon;
        int trackStatus=0;
        int tracktimestamp,timestamp;
	time_t dsmtimestamp;
 
        struct  smapoptOption optionsTable[] = {
                {"help",'h',SMAPOPT_ARG_NONE,0,'h'},
                {"arcseconds",'s',SMAPOPT_ARG_STRING,&arcseconds,'s'},
                {NULL,0,0,NULL,0}
        };

 if(argc<2) usage(-1,"Insufficient number of arguments","At least eloff(arcseconds) required.");

        optCon = smapoptGetContext("eloff", argc, argv, optionsTable,0);

        while ((c = smapoptGetNextOpt(optCon)) >= 0) {

        switch(c) {
                case 'h':
                usage(0,NULL,NULL);
                break;

                case 's':
                goteloff=1;
		eloff = atof(arcseconds);
                break;

                }


        }
	for(i=0;i<30;i++) {
        command_n[i]=0x0;
        };
        command_n[0]='P'; /* send 'O' command */

	 if(goteloff!=1) usage(-2,"No offset specified","Az (arcseconds) is required .\n");

        if(c<-1) {
        fprintf(stderr, "%s: %s\n",
                smapoptBadOption(optCon, SMAPOPT_BADOPTION_NOALIAS),
                smapoptStrerror(c));
        }
         smapoptFreeContext(optCon);

	/* initializing ref. mem. */
        dsm_status=dsm_open();
        if(dsm_status != DSM_SUCCESS) {
                dsm_error_message(dsm_status,"dsm_open()");
                exit(1);
        }

#if 0
          /* check if track is running on this antenna */

       dsm_status=dsm_read(DSM_HOST,"DSM_UNIX_TIME_L",&timestamp,&dsmtimestamp);
        dsm_status=dsm_read(DSM_HOST,"DSM_TRACK_TIMESTAMP_L",&tracktimestamp,&dsmtimestamp);

        if(abs(tracktimestamp-timestamp)>3L) {
        trackStatus=0;
        printf("Track is not running.\n");
        }
        if(abs(tracktimestamp-timestamp)<=3L) trackStatus=1;

      if(trackStatus==1) {
#endif

        dsm_status=dsm_write(DSM_HOST,"DSM_COMMANDED_TRACK_COMMAND_C30",
                                        &command_n);
        if(dsm_status != DSM_SUCCESS) {
                dsm_error_message(dsm_status,"dsm_write()");
                exit(1);
        }

      dsm_status=dsm_write(DSM_HOST, "DSM_COMMANDED_ELOFF_ARCSEC_D",&eloff);
         if(dsm_status != DSM_SUCCESS) {
                dsm_error_message(dsm_status,"dsm_write()");
                exit(1);
        }


/* the following dsm_write should actually be dsm_write_notify- but due
to a bug in gltTrack, dsm_write_notify does not work */
     dsm_status=dsm_write(DSM_HOST,"DSM_COMMAND_FLAG_S",
                                        &pmac_command_flag);

     if(dsm_status != DSM_SUCCESS) {
                dsm_error_message(dsm_status,"dsm_write()");
                exit(1);
        }

	
#if 0
	} /* if track is running */
#endif
return(0);
}

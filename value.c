#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <ctype.h>
#include <smapopt.h>
#include "dsm.h"

#define DSM_HOST "gltacc"

char *lower(char *upper);
int present(char *, char *);

void usage(int exitcode, char *error, char *addl) {
  if (error) {
    fprintf(stderr, "%s: %s\n", error, addl);
  }
  fprintf(stderr,"Usage: value [options] -v <variablename>\n");
  fprintf(stderr,"[options] include:\n");
  fprintf(stderr,"  -h or --help    this help\n");
  fprintf(stderr,"  -l or --list    to print out a list of available variables\n");
  fprintf(stderr,"  -p or --print   to print the age of a timestamp variable\n");
  exit(exitcode);
}

void main(int argc, char *argv[])  {

  dsm_structure ds;
  struct dsm_allocation_list *dal;
  int nhosts;
  int j;

  int dsm_status;
  struct utsname unamebuf;
  size_t sublength;
  int print = 0;
  int ifound,length,C,V;
  char underscore,letterV;
  char *p,*n,*q;
  int iarray, jarray;
  char *arraysizestr;
  int arraysize, arraysize2;
  char datatype[256];
  char variable_name[256], vname[256];
  char c,*command,String[256];
  char **namelist;
  int num_alloc;
  short Short,Shortvector[256];
  char Bit, Bitvector[256];
  long Long, Longvector[256],*Long2vector, UnixTime;
  int status,i,l,v1,inum;
  int vectorflag;
  float Float, Floatvector[256];
  double Double, Doublevector[256];
  smapoptContext optCon;
  int listflag=0,gotvariable=0;
  time_t timestamp;
  struct  smapoptOption optionsTable[] = {
    {"help",'h',SMAPOPT_ARG_NONE,0,'h'},
    {"list",'l',SMAPOPT_ARG_NONE,0,'l'},
    {"print",'p',SMAPOPT_ARG_NONE,&print,'p'},
    {"variable",'v',SMAPOPT_ARG_STRING,&command,'v'},
    {NULL,0,0,NULL,0}
  };
  if (argc<2) {
    usage(-1,"Insufficient number of arguments",
	  "-v variablename is required. See value -h");
  }
  optCon = smapoptGetContext("value", argc, argv, optionsTable,
			     SMAPOPT_CONTEXT_NOLOG);
  while ((c = smapoptGetNextOpt(optCon)) >= 0) {
    
    switch(c) {
    case 'h':
      usage(0,NULL,NULL);
      break;
      
    case 'l':
      listflag=1;
      break;
      
    case 'v':
      gotvariable=1;
      break;
    }
  }


  if ((gotvariable!=1)&&(listflag==0)) {
    usage(-2,"No variable specified","Variable name required; try -l to see the list of available variables.\n");
  }

  if (c<-1) {
    fprintf(stderr, "%s: %s ",
	    smapoptBadOption(optCon, SMAPOPT_BADOPTION_NOALIAS),
	    smapoptStrerror(c));
  }
  smapoptFreeContext(optCon);
  
  
  underscore='_';
  C=(int)underscore;
  
  letterV = 'v';
  V=(int)letterV;
  
  
  for(i=0;i<256;i++) variable_name[i]='\0';
  dsm_status=dsm_open();
  if(dsm_status != DSM_SUCCESS) {
    dsm_error_message(dsm_status,"dsm_open()");
    exit(1);
  }



 /* print allocation list */
  dsm_status = dsm_get_allocation_list(&nhosts, &dal);
  if(dsm_status !=DSM_SUCCESS) {
    dsm_error_message(dsm_status, "dsm_get_allocation_list()");
    exit(1);
  }

/*debug*/
/*  
  for(i=0; i<nhosts; i++) {
    printf("\nSharing %d entries with host %s:\n",
           dal[i].n_entries, dal[i].host_name);
    for(j=0; j<dal[i].n_entries; j++)
      printf("  %s\n", dal[i].alloc_list[j]);
  }
*/

  num_alloc = dal[0].n_entries;
/*
printf("number of entries: %d\n",num_alloc);
*/


  namelist = (char **)malloc(num_alloc*sizeof(char *));
  for(inum=0; inum<num_alloc; inum++) 
    namelist[inum] = (char *)malloc(DSM_NAME_LENGTH);


#if 0
/* report the list of allocation names */
  status = rm_get_num_alloc(&num_alloc);
  if(status != RM_SUCCESS) {
    rm_error_message(status, "rm_get_num_alloc()");
    exit(1);
  }

  namelist = (char **)malloc(num_alloc*sizeof(char *));
  for(inum=0; inum<num_alloc; inum++) 
    namelist[inum] = (char *)malloc(DSM_NAME_LENGTH);

  status = rm_get_alloc_names(namelist);
  if(status != DSM_SUCCESS) {
    rm_error_message(status, "rm_get_alloc_names()");
    exit(1);
  }
#endif

	ifound=-1;
  for(inum=0; inum<num_alloc; inum++) {
        strcpy(namelist[inum],dal[0].alloc_list[inum]);
	p=strchr(namelist[inum],C);
	p++;
	length=strlen(p);
	/* get the last token after underscore, for datatype */
		q=strrchr(namelist[inum],C);
		/* this includes the underscore, so increment by 1 */
		q++;
		strcpy(datatype,q);
	length=length-strlen(datatype)-1;
	sublength=(size_t)length;
	for(l=0;l<256;l++) variable_name[l]='\0';
	strncpy(variable_name,p,sublength);
	strcpy(variable_name,lower(variable_name));

	if(listflag==1) printf("%s\n",variable_name);

	vectorflag=0;
	/* check if this variable is a vector */
	for(l=0;l<strlen(variable_name);l++) {
	v1=(int)(variable_name[l+1]);
	if((variable_name[l]=='v')&&(isdigit(v1)!=0)) vectorflag++;
	}
	if(vectorflag>0) {
		n=strrchr(variable_name,C);
		n++;
		arraysizestr=strchr(n,V);
		arraysizestr++;
	  	arraysize=atoi(arraysizestr);	
		arraysize2=1;
                if (vectorflag > 1) {
                  arraysize2 = arraysize;
                  sublength = (size_t)(n-variable_name-1);
                  strncpy(vname,variable_name,sublength);
                  n=strrchr(vname,C);
                  if (n != NULL) {
                  n++;
                  arraysizestr=strchr(n,V);
	          if (arraysizestr != NULL) {
                   arraysizestr++;
                   arraysize=atoi(arraysizestr);   
                  }
                  }
/*
            fprintf(stderr,"Found a 2-dim array of size (%dx%d)\n",
                    arraysize,arraysize2);
*/
          }
	}
        
        
        if(listflag==0) {
        if(!strcmp(command,variable_name)) {
                ifound=inum;
                break;
                }
        if(!strcmp(lower(command),variable_name)) {
                ifound=inum;
                break;
                }
          }

	}
	
	if(listflag==0) {

	if(ifound!=-1) {
		if(!strcmp(datatype,"L"))
		{
		  switch(vectorflag) {
		  case 1:
			dsm_status=dsm_read(DSM_HOST,namelist[ifound],Longvector,&timestamp);
        		if(dsm_status != DSM_SUCCESS) {
                	dsm_error_message(dsm_status,"dsm_read()");
                	exit(1);
			}
			for(iarray=0;iarray<arraysize;iarray++) {
			  printf("%ld ",Longvector[iarray]);
			}
			printf("\n");
		        break;
		  case 2:
	            Long2vector = (long *)calloc(arraysize*arraysize2,sizeof(long));
                    dsm_status=dsm_read(DSM_HOST,namelist[ifound],Long2vector,&timestamp);
                    if(dsm_status != DSM_SUCCESS) {
                      dsm_error_message(dsm_status,"dsm_read()");
                      exit(1);
                    }
                    for(iarray=0;iarray<arraysize;iarray++) {
                      for(jarray=0;jarray<arraysize2;jarray++) {
                        printf("%ld ",Long2vector[iarray*arraysize2+jarray]);
                      }
                      printf("\n");
                    }
                    free(Long2vector);
                    break;
                   default:
		    dsm_status=dsm_read(DSM_HOST,namelist[ifound],&Long,&timestamp);
                    if (dsm_status != DSM_SUCCESS) {
                      dsm_error_message(dsm_status,"dsm_read()");
                      exit(1);
		     }
		     printf("%ld\n",Long);
   	             /* see if it is a timestamp variable */
		     if (present(namelist[ifound],"timestamp") ||
                         present(namelist[ifound],"TIMESTAMP")) {
		       dsm_status=dsm_read(DSM_HOST,"DSM_UNIX_TIME_L",&UnixTime,&timestamp);
          	       if (dsm_status != DSM_SUCCESS) {
                         dsm_error_message(dsm_status,"dsm_read()");
                         exit(1);
		       }
		       if (print==1) {
		printf("It is out of date by %ld seconds\n",UnixTime-Long);
		       }
		     }
		   }
		}

		if(!strcmp(datatype,"S")) {
		   if(vectorflag==1) {
           	      dsm_status=dsm_read(DSM_HOST,namelist[ifound],&Shortvector,&timestamp);
        	 	if(dsm_status != DSM_SUCCESS) {
                	dsm_error_message(dsm_status,"dsm_read()");
                	exit(1);
			}
			for(iarray=0;iarray<arraysize;iarray++) 
			printf("%d ",Shortvector[iarray]);
			printf("\n");
		      } else {
		      dsm_status=dsm_read(DSM_HOST,namelist[ifound],&Short,&timestamp);
        	      if(dsm_status != DSM_SUCCESS) {
                      dsm_error_message(dsm_status,"dsm_read()");
                      exit(1);
		      }
		   printf("%d\n",Short);
		   }
		}

		if(!strcmp(datatype,"B")) {
		   if(vectorflag==1) {
           	      dsm_status=dsm_read(DSM_HOST,namelist[ifound],&Bitvector,&timestamp);
        	 	if(dsm_status != DSM_SUCCESS) {
                	dsm_error_message(dsm_status,"dsm_read()");
                	exit(1);
			}
			for(iarray=0;iarray<arraysize;iarray++) 
			printf("%d ",Bitvector[iarray]);
			printf("\n");
		      } else {
		      dsm_status=dsm_read(DSM_HOST,namelist[ifound],&Bit,&timestamp);
        	      if(dsm_status != DSM_SUCCESS) {
                      dsm_error_message(dsm_status,"dsm_read()");
                      exit(1);
		      }
		   printf("%d\n",Bit);
		   }
		}

		if(!strcmp(datatype,"F")) {
		   if(vectorflag==1) {
           	      dsm_status=dsm_read(DSM_HOST,namelist[ifound],&Floatvector,&timestamp);
        	 	if(dsm_status != DSM_SUCCESS) {
                	dsm_error_message(dsm_status,"dsm_read()");
                	exit(1);
			}
			for(iarray=0;iarray<arraysize;iarray++) 
				{
			if(Floatvector[iarray] < 1.e-5) 
				printf("%e ",Floatvector[iarray]);
			else printf("%f ",Floatvector[iarray]);
				}
			printf("\n");
		      } else {
		   dsm_status=dsm_read(DSM_HOST,namelist[ifound],&Float,&timestamp);
        	   if(dsm_status != DSM_SUCCESS) {
                   dsm_error_message(dsm_status,"dsm_read()");
                   exit(1);
		   }
			if(Float < 1.e-5) printf("%e\n",Float);
			else printf("%f\n",Float);
		   }
		}

		if(!strcmp(datatype,"D"))
		{
		   if(vectorflag==1) {
           	      dsm_status=dsm_read(DSM_HOST,namelist[ifound],&Doublevector,&timestamp);
        	 	if(dsm_status != DSM_SUCCESS) {
                	dsm_error_message(dsm_status,"dsm_read()");
                	exit(1);
			}
			for(iarray=0;iarray<arraysize;iarray++) 
				{
			if(Doublevector[iarray] < 1.e-5) 
				printf("%e ",Doublevector[iarray]);
			else printf("%f ",Doublevector[iarray]);
				}
			printf("\n");
		      } else {
		   dsm_status=dsm_read(DSM_HOST,namelist[ifound],&Double,&timestamp);
        	   if(dsm_status != DSM_SUCCESS) {
                   dsm_error_message(dsm_status,"dsm_read()");
                   exit(1);
		   }
			if(Double < 1.e-5) printf("%e\n",Double);
			else printf("%lf\n",Double);
		   }
		}

		if(datatype[0]=='C')
		{
		dsm_status=dsm_read(DSM_HOST,namelist[ifound],String,&timestamp);
        	if(dsm_status != DSM_SUCCESS) {
                dsm_error_message(dsm_status,"dsm_read()");
                exit(1);
		}
		printf("%s\n",String);
		}
	}
	else
	{
	printf("This variable does not exist.\n");
	}
	} /* listflag==0 */

  /* free allocation list memory */
  dsm_destroy_allocation_list(&dal);
  free(namelist);
}

char *lower(char *upper) {
int i,c;
for(i=0;i<strlen(upper);i++) {
c=(int)(upper[i]);
upper[i]=(char)tolower(c);
}
return upper;
}

int present(char *a, char *b) {
  if (strstr(a,b) == NULL) {
    return(0);
  } else {
    return(1);
  }
}

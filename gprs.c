void messageInit() {
	memset(gprs.message,'\0',sizeof(gprs.message));
	gprs.message_waiting=0;
}

void gprsInit() {
	disable_interrupts(INT_RDA2);

	memset(gprs.buff,'\0',sizeof(gprs.buff));
	gprs.buff_length=0;
	gprs.age=65535;

	gprs.checksum_rx_msb=gprs.checksum_rx_lsb=0;

	messageInit();

	enable_interrupts(INT_RDA2);
}


void messageFromBuff() {
	disable_interrupts(INT_RDA2);
	/* make a new message if the last one has been read */
	if ( gprs.message_waiting != 1 ) {
		memcpy(gprs.message,gprs.buff,gprs.buff_length);
		gprs.message[gprs.buff_length+1]='\0'; // always null terminated
		gprs.message_waiting=1;

		/* reset our buffer pointer to beginning */
		gprs.buff_length=0;
	}	
	enable_interrupts(INT_RDA2);
}

void gprsSendChar(int8 c) {
	if ( HARDWARE_TYPE_RDLOGGER == current.hardware_type )
		return;

	fputc(c,stream_gprs);
}

/* returns 0 when connected */
int8 gprsReset(void) {
	static int8 state=0;

	if ( 0 == state ) {
		gprsSendChar(0x03);
		gprs.age_response=0;
		state=1;
	} else if ( 1 == state ) {
		if ( gprs.age_response > 1100 ) {
			gprsSendChar("+++");
			gprs.age_response=0;
			state=2;		
		}
	} else if ( 2 == state ) {
		if ( gprs.age_response > 1100 ) {
			gprsSendChar("ATE0\n");
			gprs.age_response=0;
			state=3;		
		}
	} else if ( 3 == state ) {
		if ( gprs.age_response > 50 ) {
			gprs.age_response=0;
			state=4;		
		}
	} else if ( 4 == state ) {
		/* pause for 100 ms */
		/* discard our buffer */
		gprsInit();

		if ( gprs.age_response > 100 ) {
			state=5;
		}
	} else if ( 5 == state ) {
		gprsSendChar("AT+CFUN=1\n");
		gprs.age_response=0;
		state=6;
	} else if ( 6 == state ) {
		/* pause for 100 ms and discard buffer */
		if ( gprs.age_response > 100 ) {
			state=0;
			gprsInit();
			return 0;
		}
	}

	return 1;
}


void gprsSendCharCont(int8 c) {
	if ( HARDWARE_TYPE_RDLOGGER == current.hardware_type || HARDWARE_TYPE_RDLOGGERUNIVERSAL == current.hardware_type ) 
		return;

	/* if we have a DLE or ETX, preceed by DLE */
	if ( 0x03 == c || 0x010 == c ) {
		fputc(0x10,stream_gprs);
	}
	fputc(c,stream_gprs);
//	fputc(c,stream_wireless);
}

/* returns 0 when connected */
int8 gprsConnect(short reset) {
	int8 search[32];
	int8 s2[32];

#if 0
	static int8 ls=255;

	if ( ls != gprs.connect_state ) {
		wirelessOn(10);
		delay_ms(20);
		fprintf(stream_wireless,"#%c%lu c_state=%u\r\n",
			current.serial_prefix,
			make16(current.serial_msb,current.serial_lsb),
			gprs.connect_state
		);
		ls=gprs.connect_state;
	}
#endif

	gprs.connection_open=0;

	if ( reset ) {
		gprs.connect_state=0;
		return 1;
	}


//	fprintf(stream_wireless,"%u\r\n",gprs.connect_state);

	if ( 0 == gprs.connect_state ) {
		gprsSendChar(0x03);
		gprs.age_response=0;
			gprs.connect_state=1;

		/* keep track of how many times we tried to (re) connect */
		if ( gprs.connects_failed < 255 ) 
			gprs.connects_failed++;
	} else if ( 1 == gprs.connect_state ) {
		if ( gprs.age_response > 1100 ) {
			gprsSendChar("+++");
			gprs.age_response=0;
			gprs.connect_state=2;		
		}
	} else if ( 2 == gprs.connect_state ) {
		if ( gprs.age_response > 1100 ) {
			gprsSendChar("ATE0\n");
			gprs.age_response=0;
			gprs.connect_state=3;		
		}
	} else if ( 3 == gprs.connect_state ) {
		if ( gprs.age_response > 50 ) {
			gprs.age_response=0;
			gprs.connect_state=4;		
		}
	} else if ( 4 == gprs.connect_state ) {
		/* pause for 100 ms */
		/* discard our buffer */
		gprsInit();

		if ( gprs.age_response > 100 ) {
			gprsInit();
			gprs.connect_state=5;
			gprs.connect_retries=0;
		}
	} else if ( 5 == gprs.connect_state ) {
		/* turn on extended error reporting */
		printf(gprsSendChar,"AT+CMEE=1\n");
		gprs.age_response=0;
		gprs.connect_state=6;
	} else if ( 6 == gprs.connect_state ) {
		/* wait for a response to CMEE command */
		
		if ( gprs.connect_retries>20 ) {
			/* too many retries, start script over */
			gprs.connect_state=0;
		}

		if ( gprs.message_waiting ) {
			gprs.message_waiting=0;
			strcpy(search,"OK");
			if ( 0 != strstr(gprs.message,search) ) {
				gprs.connect_state=7;
				gprs.connect_retries=0;
			}	
		} else if ( gprs.age_response > 250 ) {
			gprs.connect_state=5;
			gprs.connect_retries++;
		}
	} else if ( 7 == gprs.connect_state ) {
		/* check to see if we need to open the TCP/IP stack */
		printf(gprsSendChar,"AT+WOPEN?\n");
		gprs.age_response=0;
		gprs.connect_state=8;
	} else if ( 8 == gprs.connect_state ) {
		/* wait for response to AT+WOPEN? */

		if ( gprs.connect_retries>20 ) {
			/* too many retries, start script over */
			gprs.connect_state=0;
		}

		if ( gprs.message_waiting ) {
			gprs.message_waiting=0;
			strcpy(search,"+WOPEN: 1");
			if ( 0 != strstr(gprs.message,search) ) {
				/* TCP/IP stack is open */
				gprs.connect_retries=0;
				gprs.connect_state=11;
			} else {
				gprs.connect_state=9;	
				gprs.connect_retries=0;	
			}
		} else if ( gprs.age_response > 250 ) {
			gprs.connect_state=7;
			gprs.connect_retries++;
		}
	} else if ( 9 == gprs.connect_state ) {
		/* open the TCP/IP stack */
		printf(gprsSendChar,"AT+WOPEN=1\n");
		gprs.age_response=0;
		gprs.connect_state=10;
	} else if ( 10 == gprs.connect_state ) {
		/* wait for a response to AT+WOPEN command */

		if ( gprs.connect_retries>20 ) {
			/* too many retries, start script over */
			gprs.connect_state=0;
		}

		if ( gprs.message_waiting ) {
			gprs.message_waiting=0;
			strcpy(search,"OK");
			if ( 0 != strstr(gprs.message,search) ) {
				gprs.connect_state=11;
				gprs.connect_retries=0;
			} else {
				gprs.connect_retries++;
			}
		} else if ( gprs.age_response > 250 ) {
			gprs.connect_state=7;
			gprs.connect_retries++;
		}
	} else if ( 11 == gprs.connect_state ) {
		/* check to see if the TCP/IP stack is running */
		printf(gprsSendChar,"AT+WIPCFG?\n");
		gprs.age_response=0;
		gprs.connect_state=12;
	} else if ( 12 == gprs.connect_state ) {
		/* wait for a response to WIPCFG? command */

		if ( gprs.connect_retries>20 ) {
			/* too many retries, start script over */
			gprs.connect_state=0;
		}

		if ( gprs.message_waiting ) {
			gprs.message_waiting=0;
			strcpy(search,"ERROR");
			strcpy(s2,"+WIPCFG: ");
			if ( 0 != strstr(gprs.message,search) ) {
				/* error found, so we must start stack */
				gprs.connect_state=13;
				gprs.connect_retries++;
			} else if ( 0 != strstr(gprs.message,s2) ) {
				/* stack running */
				gprs.connect_state=15;
				gprs.connect_retries=0;
			} 
		} else if ( gprs.age_response > 750 ) {
			gprs.connect_state=11;
			gprs.connect_retries++;
		}
	} else if ( 13 == gprs.connect_state ) {
		/* start the IP stack since it isn't running */
		printf(gprsSendChar,"AT+WIPCFG=1\n");
		gprs.age_response=0;
		gprs.connect_state=14;
	} else if ( 14 == gprs.connect_state ) {
		/* make sure the stack comes up */

		if ( gprs.connect_retries>20 ) {
			/* too many retries, start script over */
			gprs.connect_state=0;
		}

		if ( gprs.message_waiting ) {
			gprs.message_waiting=0;
			strcpy(search,"OK");
			if ( 0 != strstr(gprs.message,search) ) {
				gprs.connect_state=15;
				gprs.connect_retries=0;
			} else {
				gprs.connect_retries++;
			}
		} else if ( gprs.age_response > 500 ) {
			gprs.connect_state=11;
			gprs.connect_retries++;
		}
	} else if ( 15 == gprs.connect_state ) {
		/* open the GPRS bearer */
		printf(gprsSendChar,"AT+WIPBR=1,6\n");
		gprs.age_response=0;
		gprs.connect_state=16;
	} else if ( 16 == gprs.connect_state ) {
		/* see if we get an OK or an error that it is already open */

		if ( gprs.connect_retries>20 ) {
			/* too many retries, start script over */
			gprs.connect_state=0;
		}

		if ( gprs.message_waiting ) {
			gprs.message_waiting=0;
			strcpy(search,"OK");
			strcpy(s2,"+CME ERROR: 804");
			if ( strstr(gprs.message,search) || strstr(gprs.message,s2) ) {
				gprs.connect_state=17;
				gprs.connect_retries++;
			} else {
				gprs.connect_retries++;
			}	
		} else if ( gprs.age_response > 1000 ) {
			gprs.connect_state=15;
			gprs.connect_retries++;
		}
	} else if ( 17 == gprs.connect_state ) {
		/* send security settings to get on the internet */
		printf(gprsSendChar,"AT+WIPBR=2,6,11,\"WAP.CINGULAR\"\n");
		gprs.age_response=0;
		gprs.connect_state=18;
	} else if ( 18 == gprs.connect_state ) {
		if ( gprs.connect_retries>20 ) {
			/* too many retries, start script over */
			gprs.connect_state=0;
		}

		if ( gprs.message_waiting ) {
			gprs.message_waiting=0;
			strcpy(search,"OK");
			if ( strstr(gprs.message,search) ) {
				gprs.connect_state=19;
				gprs.connect_retries=0;
			} else {
				gprs.connect_retries++;
			}
		} else if ( gprs.age_response > 200 ) {
			gprs.connect_state=17;
			gprs.connect_retries++;
		}
	} else if ( 19 == gprs.connect_state ) {
		/* send security settings to get on the internet */
		printf(gprsSendChar,"AT+WIPBR=2,6,0,\"WAP@CINGULARGPRS.COM\"\n");
		gprs.age_response=0;
		gprs.connect_state=20;
	} else if ( 20 == gprs.connect_state ) {
		if ( gprs.connect_retries>20 ) {
			/* too many retries, start script over */
			gprs.connect_state=0;
		}

		if ( gprs.message_waiting ) {
			gprs.message_waiting=0;
			strcpy(search,"OK");
			if ( strstr(gprs.message,search) ) {
				gprs.connect_state=21;
				gprs.connect_retries=0;
			} else {
				gprs.connect_retries++;
			}
		} else if ( gprs.age_response > 200 ) {
			gprs.connect_state=17;
			gprs.connect_retries++;
		}
	} else if ( 21 == gprs.connect_state ) {
		/* send security settings to get on the internet */
		printf(gprsSendChar,"AT+WIPBR=2,6,1,\"CINGULAR1\"\n");
		gprs.age_response=0;
		gprs.connect_state=22;
	} else if ( 22 == gprs.connect_state ) {
		if ( gprs.connect_retries>20 ) {
			/* too many retries, start script over */
			gprs.connect_state=0;
		}

		if ( gprs.message_waiting ) {
			gprs.message_waiting=0;
			strcpy(search,"OK");
			if ( strstr(gprs.message,search) ) {
				gprs.connect_state=23;
				gprs.connect_retries=0;
			} else {
				gprs.connect_retries++;
			}
		} else if ( gprs.age_response > 200 ) {
			gprs.connect_state=17;
			gprs.connect_retries++;
		}
	} else if ( 23 == gprs.connect_state ) {
		/* open the bearer and connect to the internet */
		printf(gprsSendChar,"AT+WIPBR=4,6,0\n");
		gprs.age_response=0;
		gprs.connect_state=24;
	} else if ( 24 == gprs.connect_state ) {
		/* wait to connect, or timeout */
		if ( gprs.connect_retries>20 ) {
			/* too many retries, start script over */
			gprs.connect_state=0;
		}

		if ( gprs.message_waiting ) {
			gprs.message_waiting=0;	
			strcpy(search,"OK");
			strcpy(s2,"+CME ERROR: 803");
			if ( strstr(gprs.message,search) || strstr(gprs.message,s2) ) {
				gprs.connect_state=25;
				gprs.connect_retries=0;
			} else {
				gprs.connect_retries++;
			}
		} else if ( gprs.age_response > 20000 ) {
			gprs.connect_state=23;
			gprs.connect_retries++;
		}
	} else if ( 25 == gprs.connect_state ) {
		/* open the bearer and connect to the internet */
		printf(gprsSendChar,"AT+WIPCREATE=2,1,\"data.aprsworld.com\",4012\n");
	//	printf(gprsSendChar,"AT+WIPCREATE=2,1,\"data.aprsworld.com\",4013\n");
		gprs.age_response=0;
		gprs.connect_state=26;
	} else if ( 26 == gprs.connect_state ) {
		/* wait to connect, or timeout */
		if ( gprs.connect_retries>4 ) {
			/* too many retries, start script over */
			gprs.connect_state=0;
		}

		if ( gprs.message_waiting ) {
			gprs.message_waiting=0;
			strcpy(search,"+WIPREADY: 2,1");
			strcpy(s2,"+CME ERROR: 840");			
			if ( strstr(gprs.message,search) ) {
				gprs.connect_state=27;
				gprs.connect_retries=0;
			} else if ( strstr(gprs.message,s2) ) {
				/* socket already open */
				gprs.connect_state=200;
				gprs.connect_retries=0;
			} else {
				gprs.connect_retries++;
			}
		} else if ( gprs.age_response > 20000 ) {
			gprs.connect_state=25;
			gprs.connect_retries++;
		}
	} else if ( 27 == gprs.connect_state ) {
		/* open continuous data stream ... need to escale ETX and DLE */
		printf(gprsSendChar,"AT+WIPDATA=2,1,1\n");
		gprs.age_response=0;
		gprs.connect_state=28;
	} else if ( 28 == gprs.connect_state ) {
		if ( gprs.connect_retries>20 ) {
			/* too many retries, start script over */
			gprs.connect_state=0;
		}

		/* wait to connect, or timeout */
		if ( gprs.message_waiting ) {
			gprs.message_waiting=0;
			strcpy(search,"CONNECT");
			if ( strstr(gprs.message,search) ) {
				gprs.connect_state=29;
				gprs.connect_retries=0;
			} else {
				gprs.connect_retries++;
			}
		} else if ( gprs.age_response > 30000 ) {
			gprs.connect_state=27;
			gprs.connect_retries++;
		}
	} else if ( 200 == gprs.connect_state ) {
		/* close an already open socket */
		printf(gprsSendChar,"AT+WIPCLOSE=2,1\n");
		gprs.age_response=0;
		gprs.connect_state=201;
	} else if ( 201 == gprs.connect_state ) {
		if ( gprs.connect_retries>20 ) {
			/* too many retries, start script over */
			gprs.connect_state=0;
		}

		if ( gprs.message_waiting ) {
			gprs.message_waiting=0;
			strcpy(search,"OK");
			if ( strstr(gprs.message,search) ) {
				gprs.connect_state=25;
				gprs.connect_retries=0;
			} else {
				gprs.connect_retries++;
			}
		} else if ( gprs.age_response > 500 ) {
			gprs.connect_state=0;
			gprs.connect_retries++;
		}
	}
	
	if ( 29 == gprs.connect_state ) {
		gprs.connect_state=0;
		gprs.connect_retries=0;
		gprs.connects_failed=0;
		return 0;
	} 
	
	return 1;
}

/* return non-zero on error */
int8 gprsConnected(void) {
	int8 search[16];
	int8 r;
	int16 lcrc;

	gprs.connection_open=1;

	/* check to see if we shut down */
	if ( gprs.message_waiting ) {
		gprs.message_waiting=0;

		strcpy(search,"SHUTDOWN");
		r = strstr(gprs.message,search);
		strcpy(search,"+CME ERROR:");
		r += strstr(gprs.message,search);

		if ( r ) {
			wirelessOn(1);
			delay_ms(20);
			fprintf(stream_wireless,"# disconnected with: %s\r\n",gprs.message);
			return r;
		}

		if ( '!' == gprs.message[0] ) {
			/* got an ack */
			gprs.checksum_rx_msb=gprs.message[1];
			gprs.checksum_rx_lsb=gprs.message[2];
			gprs.checksum_rx_used=0;
		} else if ( '#' == gprs.message[0] &&
             current.serial_prefix == gprs.message[1] &&
             current.serial_msb == gprs.message[2] &&
             current.serial_lsb == gprs.message[3] &&
             13 == gprs.message[4] &&
             22 == gprs.message[5] 
           ) {
			/* query to us */

		
			lcrc=crc_chk(gprs.message+1,10);

			if ( lcrc != make16(gprs.message[11],gprs.message[12]) ) {
//				fprintf(stream_gprs,"# invalid crc ... we calculated 0x%04lx and got 0x%04lx\r\n",lcrc,make16(gprs.message[11],gprs.message[12]));
				return 0;
			}

			switch ( gprs.message[6] ) {
				case 0: 
					action.now_live_status=1;
					break;
				case 1:
					action.now_log_init=1;
					break;
#if 0
				case 2:
					log.page_requested=make16(gprs.message[7],gprs.message[8]);
					log.n_pages=make16(gprs.message[9],gprs.message[10]);
					break;
#endif
			}
		}
		return r;
	}

	return 0;
}


void gprsTick(void) {
	int8 rv;

	if ( gprs.connects_failed > 4 || GPRS_STATE_ERROR==gprs.state ) {
		/* send a AT+CFUN=1 if we get four or more failed connects */
		rv=gprsReset();
		if ( 0 == rv ) {
			gprs.state=GPRS_STATE_DISCONNECTED;
			gprs.uptime=0;
			gprs.connects_failed=0;
		}
	} else if ( GPRS_STATE_DISCONNECTED==gprs.state )  {
		rv=gprsConnect(0);
		if ( 0 == rv ) {
			gprs.state=GPRS_STATE_READY;
			gprs.missed_acks=0;
			gprs.uptime=0;
		}
	} else if ( GPRS_STATE_READY==gprs.state ) {
		rv=gprsConnected();
		if ( 0 != rv ) {
			/* connection appears to have dropped */
			gprs.state=GPRS_STATE_DISCONNECTED;
			gprsConnect(1); /* reset our connection state machine */
//			fprintf(stream_wireless,"# connection dropped\r\n");
			gprs.uptime=0;
		}
	} 

}
#define		GET_IP		1
#define		ACK_IP		2
#define		GET_TIME	3
#define		ACK_TIME	4
#define		GET_DATE	5
#define		ACK_DATE	6
#define		GET_SSID	7
#define		ACK_SSID	8
#define		GET_CONFIG	9
#define		ACK_CONFIG	10




char cmdStr[56];
extern uint8_t cmdq;

  // get string label for various json cmds
  char* jcmd(uint8_t jcmd) {

	  
	  switch (jcmd) {
		case 1:	  
			strcpy_P(cmdStr, PSTR("{\"get\":\"ip\"}\n"));
			break;
		case 2:
			strcpy_P(cmdStr, PSTR("{\"ack\":\"ip\"}\n"));
			break;
		case 3:	
			strcpy_P(cmdStr, PSTR("{\"get\":\"time\"}\n"));
			break;
		case 4:
			strcpy_P(cmdStr, PSTR("{\"ack\":\"time\"}\n"));
			break;
		case 5:		  
			strcpy_P(cmdStr, PSTR("{\"get\":\"date\"}\n"));
			break;
		case 6:
			strcpy_P(cmdStr, PSTR("{\"ack\":\"date\"}\n"));
			break;
		case 7:			  
			strcpy_P(cmdStr, PSTR("{\"get\":\"ssid\"}\n"));
			break;
		case 8:
			strcpy_P(cmdStr, PSTR("{\"ack\":\"ssid\"}\n"));
			break;
		case 9:
			strcpy_P(cmdStr, PSTR("{\"get\":\"config\"}\n"));
			break;
		case 10:
			strcpy_P(cmdStr, PSTR("{\"ack\":\"config\"}\n"));
			break;			
					  
		  default: break;
	  }
	  return cmdStr;
  }

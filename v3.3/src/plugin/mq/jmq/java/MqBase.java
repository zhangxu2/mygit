package mqjava;
import com.ibm.mq.*;
import java.io.*;
import java.text.SimpleDateFormat;
import java.util.Date;

public class MqBase{
	public static int MSG_STRING = 1;
	public static int msgflag = 1;
	public static int port = 0;
	public static int ccsid = 0;
	public static int fmtFlag = 0;
	public static int procId = 0;
	public static String ip = null;
	public static String qmgr = null;
	public static String ConnChnl = "SYSTEM.DEF.SVRCONN";
	public static String SendQueue = null;
	public static String RecvQueue = null;
	public static MQQueue SendQ = null;
	public static MQQueue RecvQ = null;
	public static MQMessage SendMsg = null;
	public static MQMessage RecvMsg = null;
	public static MQQueueManager SendQmgr = null;
	public static MQQueueManager RecvQmgr = null;
	public static MQPutMessageOptions putOpt = null;
	public static MQGetMessageOptions getOpt = null;
	public static PrintStream psOut = null;
	public static File file = null;

	public int getMsgflag() {
		return msgflag;
	}
	public void setMsgflag(int msgflag) {
		MqBase.msgflag = msgflag;
	}

	public static int getFmtFlag() {
		return fmtFlag;
	}
	public static void setFmtFlag(int fmtFlag) {
		MqBase.fmtFlag = fmtFlag;
	}

	public static String getDateTime(){
		SimpleDateFormat df = new SimpleDateFormat("[yyyy-MM-dd HH:mm:ss]");
		return df.format(new Date());
	}

	public MqBase(int cprocId, int cport, int cccsid, String cip, String cqmgr, String cconChl){
		if (cip == null ||cport == 0 || cccsid == 0 || cqmgr == null){
			System.out.println("input argvment error,ip:" + ip + "port:" + port + "ccsid" + ccsid + "qmgr:" + qmgr);
		}

		ip = cip;
		port = cport;
		procId = cprocId;
		ccsid = cccsid;
		qmgr = cqmgr;
		if (cconChl != null){
			ConnChnl = cconChl;
		}
	}

	private int initLog() {
		String work = System.getenv("SWWORK");
		String path = String.format("%s/log/syslog/java%d.log", work, procId);
		SimpleDateFormat df = new SimpleDateFormat("yyyyMMddhhmmss");  
		String date = df.format(new Date());
		String pathbak = String.format("%s/log/syslog/java%d.log%s", work, procId, date);
		file =new File(path);
		if (file.exists()){
			File dst = new File(pathbak);
			file.renameTo(dst);
			if (psOut != null){
				psOut.close();
			}
		}

		try {
			psOut = new PrintStream(new FileOutputStream(path));
		} catch (FileNotFoundException e) {
			e.printStackTrace();
			return -1;
		} 
		System.setOut(psOut); 
		System.setErr(psOut);
		return 0;
	}

	public void initEnv(){
		MQEnvironment.hostname = ip;
		MQEnvironment.port = port;
		MQEnvironment.channel = ConnChnl;
		MQEnvironment.CCSID = ccsid;
	}

	public int init(){
		initEnv();
		if (initLog() < 0) {
			System.out.println(getDateTime() + " Init Log error!");
			return -1;
		}

		System.out.println(getDateTime() + " init Env ok.");
		return 0;
	}

	public int initSend(String sendQ, String recvQ){
		SendQueue = sendQ;
		RecvQueue = recvQ;
		try{
			SendQmgr = new MQQueueManager(qmgr);
			if (SendQmgr == null){
				System.out.println(getDateTime() + " Create Send MQQueueManger error.");
				return -1;
			}
			int openOptions = MQC.MQOO_OUTPUT | MQC.MQOO_FAIL_IF_QUIESCING;
			SendQ = SendQmgr.accessQueue(SendQueue, openOptions);
			putOpt = new MQPutMessageOptions();
			SendMsg = new MQMessage();
		}catch(MQException e){
			System.out.println(getDateTime() + " initSend error.errcode=[" + e.reasonCode + "]");
			return -1;
		}

		return 0;
	}

	public int initRecv(String recvQ, String sendQ){
		SendQueue = sendQ;
		RecvQueue = recvQ;
		try{
			RecvQmgr = new MQQueueManager(qmgr);
			if (RecvQmgr == null){
				System.out.println(getDateTime() + " Create recv MQQueueManger error.");
				return -1;
			}

			int openOptions = MQC.MQOO_INPUT_AS_Q_DEF | MQC.MQOO_OUTPUT | MQC.MQOO_INQUIRE;
			RecvQ = RecvQmgr.accessQueue(RecvQueue, openOptions);
			getOpt = new MQGetMessageOptions();
			RecvMsg = new MQMessage();
			getOpt.options = getOpt.options + MQC.MQGMO_SYNCPOINT;
			getOpt.options = getOpt.options + MQC.MQGMO_WAIT;
			getOpt.options = getOpt.options + MQC.MQGMO_FAIL_IF_QUIESCING;
			getOpt.waitInterval = 3000;
		}catch(MQException e){
			System.out.println(getDateTime() + " initRecv error,errcode=[" + e.reasonCode + "]");
			return -1;
		}

		return 0;
	}

	public int sendMessage(byte[] sMsg, String msgid, String corrid, int exptime) {
		if (msgid == null || msgid.length() == 0){
			SendMsg.messageId = MQC.MQMI_NONE;
		}else{
			SendMsg.messageId = msgid.getBytes();
		}

		if (corrid == null || corrid.length() == 0){
			SendMsg.correlationId  = MQC.MQCI_NONE;
		}else{
			SendMsg.correlationId = corrid.getBytes();
		}

		if (exptime > 0){
			SendMsg.expiry = exptime;
		}

		if (fmtFlag == MSG_STRING){
			SendMsg.format = MQC.MQFMT_STRING;
		}else{
			SendMsg.format = MQC.MQFMT_NONE;
		}

		SendMsg.messageType = MQC.MQMT_REQUEST;
		SendMsg.replyToQueueName = RecvQueue;
		try {
			/*System.out.println("send len:" + sMsg.length());
			SendMsg.write(sMsg.getBytes());
			*/
			SendMsg.write(sMsg);
			System.out.println("msg byte:" + sMsg.length);
			System.out.println("msg byte:" + sMsg.toString());
			SendQ.put(SendMsg, putOpt);
			SendMsg.clearMessage();

		} catch (MQException mqe) {
			System.out.println(getDateTime() + " send Message MQ error.");
			return -1;

		}catch (IOException e1) {
			System.out.println(getDateTime() + " send Message IO error.");
			return -1;
		}

		return 0;
	}


	public byte[] recvMessage(String msgid, String corrid){
		msgflag = 1;

		if (msgid == null || msgid.length() == 0){
			RecvMsg.messageId = MQC.MQMI_NONE;
		}else{
			RecvMsg.messageId = msgid.getBytes();
		}

		if (corrid == null || corrid.length() == 0){
			RecvMsg.correlationId  = MQC.MQCI_NONE;
		}else{
			RecvMsg.correlationId = corrid.getBytes();
		}

		int depth = 0;
		byte [] b = null;
		String  text = null;
		try {
			depth = RecvQ.getCurrentDepth();
			if (depth == 0){
				msgflag = 0;
				return null;
			}
			RecvQ.get(RecvMsg, getOpt);
			//System.out.println("recv msg len:" + RecvMsg.getMessageLength());
			b = new byte[RecvMsg.getMessageLength()];
			RecvMsg.readFully(b);
			if (msgid != null){
				msgid = RecvMsg.messageId.toString();
			}
			if (corrid != null){
				corrid = RecvMsg.correlationId.toString();
			}
			RecvMsg.clearMessage();
			RecvMsg.messageId = null;
			RecvMsg.correlationId = null;
		}catch(MQException mqe){
			if (mqe.reasonCode == 2033){
				if (file.length() >= 1024 * 1024 * 8){
					initLog();
				}
				msgflag = 0;
				System.out.println(getDateTime() + " no message,errocde[" + mqe.reasonCode + "]");
				return null;
			}
			System.out.println(getDateTime() + " recv message error,errocde[" + mqe.reasonCode + "]");
			return null;
		}catch(IOException e1){
			System.out.println(getDateTime() + " recv message io error.");
			return null;
		}

		try {
			RecvQmgr.commit();
		}catch (MQException e) {
			System.out.println(getDateTime() + " commit send error,errocde[" + e.reasonCode + "]");
			return null;
		}

		return b;
	}

	public void delSend(){
		try {
			SendQ.close();
		} catch (MQException e) {
			System.out.println(getDateTime() + " del Send error,errocde[" + e.reasonCode + "]");
		}

		try {
			SendQmgr.disconnect();
		} catch (MQException e) {
			System.out.println(getDateTime() + " dis send qmgr connect error,errocde[" + e.reasonCode + "]");
		}

		if (psOut != null){
			psOut.close();
		}
	}

	public void delRecv(){
		try {
			RecvQ.close();
		} catch (MQException e) {
			System.out.println(getDateTime() + " del Recv error,errocde[" + e.reasonCode + "]");
		}
		try {
			RecvQmgr.disconnect();
		} catch (MQException e) {
			System.out.println(getDateTime() + " dis Recv qmgr connect error,errocde[" + e.reasonCode + "]");
		}

		if (psOut != null){
			psOut.close();
		}
	}

	public static void main(String args[]){
        if (args.length != 5){
            System.out.println("useage: java MqBase connectip connectport ccsid qmgr filepath");
            System.out.println("example: java MqBase 192.168.101.245 11183 ccsid QMMPC a.txt");
            return;
        }
        MqBase mqbase = new MqBase(0, Integer.parseInt(args[1]), Integer.parseInt(args[2]),
                args[0], args[3], null);
        System.out.println("ready read file:" + args[4]);
        StringBuffer buffer = new StringBuffer();
        String sendbuf = null;
        File file = new File(args[4]);
    	try {
			BufferedReader reader = new BufferedReader(new FileReader(file));
			String tmpStr = null;
			try {
				while((tmpStr = reader.readLine()) != null){
					System.out.println("string:" + tmpStr);
					buffer.append(tmpStr);
				}
				reader.close();
			} catch (IOException e) {
				e.printStackTrace();
			}
		} catch (FileNotFoundException e) {
			e.printStackTrace();
		}
        sendbuf = buffer.toString();
        System.out.println("ready init.");
        mqbase.init();
        System.out.println("ready initsend.");
        String send = new String("LQ_1");
        String recv = new String("LQ_1");
        mqbase.initSend(send, recv);
        System.out.println("ready initrecv.");
        mqbase.initRecv(send,recv);
        System.out.println("ready sendmsg.");
        System.out.println("ready recvmsg.:" + sendbuf);
        int i = 0;
        String str = null;
        while( i < 5){
            System.out.println("recv msg:" + str);
            i++;
            int flag = mqbase.getMsgflag();
            System.out.println("flag:" + flag);
        }
        mqbase.delSend();
        mqbase.delRecv();
    }
}

